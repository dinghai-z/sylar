#include "scheduler.hpp"
#include "util.hpp"
#include "macro.hpp"
#include "hook.hpp"
#include <time.h>

namespace sylar{

static thread_local Scheduler *t_scheduler = nullptr;
static thread_local Fiber *t_scheduler_fiber = nullptr;

static Logger::ptr g_logger = SYLAR_LOG_NAME("system");

Scheduler::Scheduler(std::size_t thread_count, const std::string name)
    :m_name(name)
    ,m_thread_count(thread_count){
    setThis();
}

void Scheduler::start(){
    m_stopping = false;
    m_threads.resize(m_thread_count);
    for(std::size_t i = 0; i != m_thread_count; i++){
        m_threads[i].reset(new Thread(std::bind(&sylar::Scheduler::run, this), 
                                        m_name + "_" + std::to_string(i)));
        m_threadIds.push_back(m_threads[i]->getId());
    }
}

void Scheduler::stop(){
    m_stopping = true;
    for(std::size_t i = 0; i < m_thread_count; ++i) {
        tickle();
    }
    std::vector<Thread::ptr> temp;
    {
        MutexType::Lock lock(m_mutex);
        temp.swap(m_threads);
    }
    for(auto i : temp){
        i->join();
    }
}

Scheduler* Scheduler::GetThis(){
    return t_scheduler;
}

void Scheduler::setThis(){
    t_scheduler = this;
}

void Scheduler::tickle(){
    //SYLAR_LOG_DEBUG(g_logger) << "Scheduler::tickle()";
}

bool Scheduler::stopping(){
    MutexType::Lock lock(m_mutex);
    if(m_stopping && m_active_thread_count == 0 && m_fibers.empty())
        return true;
    return false;
}

void Scheduler::idle(){
    while(!stopping()){
        Fiber::YieldToHold();
    }
}

void Scheduler::run(){
    set_hook_enable(true);
    setThis();
    t_scheduler_fiber = Fiber::GetThis().get();
    Fiber::ptr idle_fiber = Fiber::ptr(new Fiber(std::bind(&sylar::Scheduler::idle, this)));
    // Fiber::ptr ft_fiber;
    while(true){
        Fiber::ptr ft_fiber;
        FiberAndThread ft;
        bool need_tickle = false;
        {
            MutexType::Lock lock(m_mutex);
            auto it = m_fibers.begin();
            while(it != m_fibers.end()){
                if(it->thread_id != -1 && it->thread_id != GetThreadId()){
                    it++;
                    need_tickle = true;
                    continue;
                }
                SYLAR_ASSERT(it->cb != nullptr || it->fiber != nullptr);
                ft = *it;
                it = m_fibers.erase(it);
                m_active_thread_count++;
                break;
            }
            if(need_tickle || it != m_fibers.end())tickle();
        }
        if(ft.fiber != nullptr){
            {
                Fiber::MutexType::Lock lock(ft.fiber->m_mutex);
                ft.fiber->swapIn();
            }
            m_active_thread_count--;
            if(ft.fiber->getState() == Fiber::READY){
                schedule(ft.fiber, (ft.thread_id != -1) ? ft.thread_id : -1);
            }
        } else if(ft.cb != nullptr){
            if(ft_fiber != nullptr){
                ft_fiber->reset(ft.cb);
            } else {
                ft_fiber = Fiber::ptr(new Fiber(ft.cb));
            }
            {
                Fiber::MutexType::Lock lock(ft_fiber->m_mutex);
                ft_fiber->swapIn();
            }
            m_active_thread_count--;
            if(ft_fiber->getState() == Fiber::READY){
                schedule(ft_fiber, (ft.thread_id != -1) ? ft.thread_id : -1);
                ft_fiber.reset();
            } else if(ft_fiber->getState() == Fiber::HOLD){
                ft_fiber.reset();
            }
        } else {
            if(idle_fiber->getState() == Fiber::TERM){
                break;
            }
            m_idle_thread_count++;
            idle_fiber->swapIn();
            m_idle_thread_count--;
        }
    }
}

}