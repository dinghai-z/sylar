#ifndef _SYLAR_SCHEDULER_H_
#define _SYLAR_SCHEDULER_H_

#include "fiber.hpp"
#include "thread.hpp"
#include <vector>
#include <list>
#include <atomic>

namespace sylar{

class Scheduler{
public:
    typedef std::shared_ptr<Scheduler> ptr;
    typedef Mutex MutexType;

    Scheduler(std::size_t thread_count = 1, const std::string name = "");
    virtual ~Scheduler(){}
    void start();
    void stop();

    /**
     * @brief 调度协程
     * @param[in] fc 协程或函数
     * @param[in] thread 协程执行的线程id,-1标识任意线程
     */
    template<typename FiberOrCb>
    void schedule(FiberOrCb f, pid_t t = -1){
        MutexType::Lock lock(m_mutex);
        bool need_tickle = m_fibers.empty();
        FiberAndThread ft(f, t);
        if(ft.fiber || ft.cb){
            m_fibers.push_back(ft);
        }
        if(need_tickle) {
            tickle();
        }
    }

    /**
     * @brief 批量调度协程
     * @param[in] begin 协程数组的开始
     * @param[in] end 协程数组的结束
     */
    template<class InputIterator>
    void schedule(InputIterator begin, InputIterator end) {
        MutexType::Lock lock(m_mutex);
        bool need_tickle = m_fibers.empty();
        while(begin != end){
            FiberAndThread ft(&*begin, -1);
            if(ft.fiber || ft.cb){
                m_fibers.push_back(ft);
            }
            ++begin;
        }
        if(need_tickle) {
            tickle();
        }
    }

    std::string getName(){return m_name;}
    static Scheduler *GetThis();
    
protected:
    void run();
    virtual void tickle();
    virtual void idle();
    virtual bool stopping();
    void setThis();

private:
    struct FiberAndThread{
        FiberAndThread(){}

        FiberAndThread(Fiber::ptr f, pid_t t)
            :fiber(f)
            ,thread_id(t){
        }

        FiberAndThread(Fiber::ptr *f, pid_t t)
            :thread_id(t){
            fiber.swap(*f);
        }

        FiberAndThread(std::function<void()> f, pid_t t)
            :cb(f)
            ,thread_id(t){
        }

        FiberAndThread(std::function<void()> *f, pid_t t)
            :thread_id(t){
            cb.swap(*f);
        }

        void reset(){
            fiber= nullptr;
            cb = nullptr;
            thread_id = -1;
        }

        Fiber::ptr fiber= nullptr;
        std::function<void()> cb = nullptr;
        pid_t thread_id = -1;
    };

protected:
    std::string m_name;
    std::vector<Thread::ptr> m_threads;
    std::list<FiberAndThread> m_fibers;
    std::vector<pid_t> m_threadIds;
    std::size_t m_thread_count = 0;
    std::atomic<std::size_t> m_active_thread_count = {0};
    std::atomic<std::size_t> m_idle_thread_count = {0};
    MutexType m_mutex;
    std::atomic<bool> m_stopping;
};

}

#endif