#ifndef _SYLAR_THREAD_H_
#define _SYLAR_THREAD_H_

#include <pthread.h>
#include <functional>
#include "mutex.hpp"
#include <time.h>
#include <memory>
#include <string>

namespace sylar{

class Thread{
public:
    typedef std::shared_ptr<Thread> ptr;
    Thread(std::function<void()> cb, const std::string &name);
    ~Thread();
    std::string getName() {return m_name;}
    pid_t getId(){return m_id;}
    void join();
    static Thread* GetThis();
    static std::string GetName();
    static void SetName(const std::string &name);
private:
    static void* Run(void *arg);
private:
    std::function<void()> m_cb;
    std::string m_name;
    pthread_t m_thread;
    pid_t m_id;
    Semaphore m_sem;
};

}


#endif