#include "iomanager.hpp"
#include "macro.hpp"
#include <unistd.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <memory>
#include <string.h>

namespace sylar{

static Logger::ptr g_logger = SYLAR_LOG_NAME("system");

IOManager::FdContext::FdContext(){
    //SYLAR_LOG_DEBUG(g_logger) << "FdContext()";
}

IOManager::FdContext::~FdContext(){
    //SYLAR_LOG_DEBUG(g_logger) << "~FdContext()";
}

IOManager::FdContext::EventContext &IOManager::FdContext::getContext(Event event){
    SYLAR_ASSERT(events & event);
    switch(event){
        case READ:
            return read;
        case WRITE:
            return write;
        default:
            SYLAR_ASSERT(false);
    }
}

void IOManager::FdContext::triggerContext(Event event){
    SYLAR_ASSERT(events & event);
    EventContext &event_ctx = getContext(event);
    events = Event(events & ~event);
    if(event_ctx.cb){
        event_ctx.scheduler->schedule(&event_ctx.cb);
    } else {
        event_ctx.scheduler->schedule(&event_ctx.fiber);
    }
}

void IOManager::FdContext::resetContext(Event event){
    SYLAR_ASSERT(events & event);
    EventContext &event_ctx = getContext(event);
    event_ctx.cb = nullptr;
    event_ctx.fiber = nullptr;
    event_ctx.scheduler = nullptr;
    events = Event(events & ~event);
}

IOManager* IOManager::GetThis(){
    return dynamic_cast<IOManager*>(Scheduler::GetThis());
}

IOManager::IOManager(std::size_t thread_count, const std::string name)
    :Scheduler(thread_count, name){
    m_epoll_fd = epoll_create(2048);
    SYLAR_ASSERT(m_epoll_fd > 0);

    int ret = pipe(m_tickleFds);
    SYLAR_ASSERT(ret == 0);
    ret = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);
    SYLAR_ASSERT(ret == 0);

    epoll_event epoll_ev;
    epoll_ev.events = EPOLLIN | EPOLLET;
    epoll_ev.data.fd = m_tickleFds[0];
    ret = epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, m_tickleFds[0], &epoll_ev);
    SYLAR_ASSERT(ret == 0);

    resizeFdContexts(8);

    start();
}

void IOManager::resizeFdContexts(std::size_t size){
    m_fd_contexts.resize(size);
    for(std::size_t i = 0; i < m_fd_contexts.size(); i++){
        if(m_fd_contexts[i] == nullptr){
            m_fd_contexts[i] = FdContext::ptr(new FdContext);
            m_fd_contexts[i]->fd = i;
        }
    }
}

IOManager::~IOManager(){
    stop();
    close(m_epoll_fd);
    close(m_tickleFds[0]);
    close(m_tickleFds[1]);
}

int IOManager::addEvent(int fd, Event event, std::function<void(void)> cb){
    FdContext::ptr fd_ctx;
    RWMutexType::ReadLock lock(m_mutex);
    if((int)m_fd_contexts.size() > fd){
        fd_ctx = m_fd_contexts[fd];
        lock.unlock();
    } else {
        lock.unlock();
        RWMutexType::WriteLock lock2(m_mutex);
        resizeFdContexts(fd * 1.5);
        fd_ctx = m_fd_contexts[fd];
    }

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    SYLAR_ASSERT(!(fd_ctx->events & event));
    int op = (fd_ctx->events == NONE) ? EPOLL_CTL_ADD : EPOLL_CTL_MOD;
    fd_ctx->events = (Event)(fd_ctx->events | event);
    SYLAR_ASSERT(fd_ctx->fd == fd);
    epoll_event epoll_ev;
    epoll_ev.events = fd_ctx->events | EPOLLET;
    epoll_ev.data.ptr = fd_ctx.get();
    int ret = epoll_ctl(m_epoll_fd, op, fd_ctx->fd, &epoll_ev);
    if(ret){
        SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epoll_fd << ", "
        << op << ", " << fd << ", " << (EPOLL_EVENTS)epoll_ev.events << "):"
        << ret << " (" << errno << ") (" << strerror(errno) << ")";
        return -1;
    }
    m_pendingEventCount++;

    FdContext::EventContext &event_ctx = fd_ctx->getContext(event);
    if(cb){
        event_ctx.cb = cb;
    } else {
        event_ctx.fiber = Fiber::GetThis();
    }
    event_ctx.scheduler = Scheduler::GetThis();
    return 0;
}

void IOManager::delEvent(int fd, Event event){
    FdContext::ptr fd_ctx;
    {
        RWMutexType::ReadLock lock(m_mutex);
        SYLAR_ASSERT(fd <= (int)m_fd_contexts.size());
        fd_ctx = m_fd_contexts[fd];
    }

    FdContext::MutexType::Lock lock(fd_ctx->mutex);
    Event new_event = (Event)(fd_ctx->events & ~event);
    int op = new_event ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event epoll_ev;
    epoll_ev.events = new_event | EPOLLET;
    epoll_ev.data.ptr = fd_ctx.get();
    int ret = epoll_ctl(m_epoll_fd, op, fd_ctx->fd, &epoll_ev);
    if(ret) {
        SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epoll_fd << ", "
            << op << ", " << fd << ", " << (EPOLL_EVENTS)epoll_ev.events << "):"
            << ret << " (" << errno << ") (" << strerror(errno) << ")";
        return;
    }
    m_pendingEventCount--;

    fd_ctx->resetContext(event);
}

void IOManager::tickle(){
    write(m_tickleFds[1], "t", 1);
}

bool IOManager::stopping(uint64_t& timeout) {
    timeout = getNextTimer();
    return timeout == ~0ull
        && m_pendingEventCount == 0
        && Scheduler::stopping();
}

bool IOManager::stopping(){
    uint64_t timeout = 0;
    return stopping(timeout);
}

void IOManager::idle(){
    const std::size_t MAX_EVNETS = 512;
    epoll_event *epoll_events = new epoll_event[MAX_EVNETS];
    //自动析构
    std::unique_ptr<epoll_event, void(*)(epoll_event*)> epoll_events_ptr(epoll_events, [](epoll_event *ptr){
        delete[] ptr;
    });

    while(!stopping()){
        uint64_t next_timeout = 0;
        if(SYLAR_UNLIKELY(stopping(next_timeout))) {
            SYLAR_LOG_INFO(g_logger) << "name=" << getName()
                                     << " idle stopping exit";
            break;
        }
        
        int nready;
        do {
            static const int MAX_TIMEOUT = 3000;
            if(next_timeout != ~0ull) {
                next_timeout = (int)next_timeout > MAX_TIMEOUT
                                ? MAX_TIMEOUT : next_timeout;
            } else {
                next_timeout = MAX_TIMEOUT;
            }
            nready = epoll_wait(m_epoll_fd, epoll_events, MAX_EVNETS, (int)next_timeout);
            if(nready < 0 && errno == EINTR) {
            } else {
                break;
            }
        } while(true);

        std::vector<std::function<void()> > cbs;
        listExpiredCb(cbs);
        if(!cbs.empty()) {
            //SYLAR_LOG_DEBUG(g_logger) << "on timer cbs.size=" << cbs.size();
            schedule(cbs.begin(), cbs.end());
            cbs.clear();
        }
        
        for(int i = 0; i < nready; i++){
            epoll_event event = epoll_events[i];
            if(event.data.fd == m_tickleFds[0]){
                char temp[64];
                while(read(event.data.fd, temp, sizeof(temp)) > 0);
                continue;
            }

            FdContext *fd_context = (FdContext *)event.data.ptr;
            FdContext::MutexType::Lock lock(fd_context->mutex);

            if(!((event.events & (READ | WRITE)) & fd_context->events)){
                continue;
            }

            Event left_event = (Event)(fd_context->events & ~event.events);
            int op = left_event ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
            epoll_event new_event;
            new_event.events = left_event | EPOLLET;
            new_event.data.ptr = event.data.ptr;
            int ret = epoll_ctl(m_epoll_fd, op, fd_context->fd, &new_event);
            if(ret){
                // SYLAR_ASSERT(ret == 0);
                SYLAR_LOG_DEBUG(g_logger) << "epoll_ctl error: " << strerror(errno);
                return;
            }

            if(event.events & EPOLLIN){
                fd_context->triggerContext(READ);
                m_pendingEventCount--;
            }
            if(event.events & EPOLLOUT){
                fd_context->triggerContext(WRITE);
                m_pendingEventCount--;
            }
        }
        Fiber::YieldToHold();
    }
}

void IOManager::onTimerInsertedAtFront() {
    tickle();
}

}