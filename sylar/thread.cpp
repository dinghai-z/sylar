#include "thread.hpp"
#include "log.hpp"

namespace sylar{

static thread_local Thread* t_thread = nullptr;
static thread_local std::string t_thread_name = "MAIN";

static Logger::ptr g_logger = SYLAR_LOG_NAME("system");

Thread::Thread(std::function<void()> cb, const std::string &name)
    :m_cb(cb)
    ,m_name(name)
    ,m_sem(0){
    int ret = pthread_create(&m_thread, nullptr, &Run, this);
    if(ret){
        SYLAR_LOG_ERROR(g_logger) << "pthread_create error, ret = " << ret << "name = " << m_name;
        throw std::logic_error("pthread_create error");
    }
    m_sem.wait();
}

Thread::~Thread(){
    if(m_thread){
        pthread_detach(m_thread);
    }
}

void Thread::join(){
    if(m_thread){
        int ret = pthread_join(m_thread, nullptr);
        if(ret){
            SYLAR_LOG_ERROR(g_logger) << "pthread_join error, ret = " << ret << "name = " << m_name;
            throw std::logic_error("pthread_join error");
        }
        m_thread = 0;
    }
}

Thread* Thread::GetThis(){
    return t_thread;
}

std::string Thread::GetName(){
    return t_thread_name;
}

void* Thread::Run(void *arg){
    Thread* thread = (Thread*)arg;
    t_thread = thread;
    t_thread_name = thread->m_name;
    thread->m_id = GetThreadId();
    pthread_setname_np(pthread_self(), thread->m_name.substr(0, 15).c_str());
    std::function<void()> cb;
    cb.swap(thread->m_cb);
    thread->m_sem.notify();
    cb();
    return 0;
}

}
