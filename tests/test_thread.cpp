#include "sylar/log.hpp"
#include <string>
#include <memory>
#include "sylar/thread.hpp"
#include "sylar/mutex.hpp"
#include <stdlib.h>
#include "sylar/macro.hpp"

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("root");
int s_count = 0;
sylar::SpinLock s_mutex;

void fun(){
    SYLAR_LOG_DEBUG(g_logger) << "name: " << sylar::Thread::GetName()
                            << " this.name: " << sylar::Thread::GetThis()->getName()
                            << " id: " << sylar::GetThreadId()
                            << " this.id: " << sylar::Thread::GetThis()->getId();
    for(int i = 0; i < 100000; i++){
        sylar::SpinLock::Lock lock(s_mutex);
        s_count++;
    }
    
    // SYLAR_ASSERT(true);
    // SYLAR_ASSERT2(false, "xxx");

}

void fun2() {
    while(true) {
        SYLAR_LOG_INFO(SYLAR_LOG_NAME("root")) << "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
        SYLAR_LOG_INFO(SYLAR_LOG_NAME("system")) << "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
    }
}

int main(){
    SYLAR_LOG_DEBUG(g_logger) << "main begin";
    
    std::vector<sylar::Thread::ptr> thread_pool;
    for(int i = 0; i != 5; i++){
        thread_pool.push_back(sylar::Thread::ptr(new sylar::Thread(&fun, std::string("thread") + std::to_string(i) + "Name")));
    }

    for(int i = 0; i != 5; i++){
        thread_pool[i]->join();
    }

    SYLAR_LOG_DEBUG(g_logger) << s_count;

    SYLAR_LOG_DEBUG(g_logger) << "main end";
    
    return 0;
}