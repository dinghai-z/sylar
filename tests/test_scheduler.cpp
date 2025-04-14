#include "sylar/fiber.hpp"
#include "sylar/log.hpp"
#include "sylar/thread.hpp"
#include "sylar/scheduler.hpp"
#include <time.h>

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("root");

void run_fiber(){
    static std::atomic<int> s_count{10};
    SYLAR_LOG_INFO(g_logger) << "test in fiber s_count=" << s_count;

    sleep(1);
    if(--s_count >= 0) {
        sylar::Scheduler::GetThis()->schedule(&run_fiber, sylar::GetThreadId());
    }
}

void test1(){
    SYLAR_LOG_DEBUG(g_logger) << "main begin";
    sylar::Scheduler scheduler(5, "testSch");
    sylar::Scheduler scheduler2(5, "testSch2");
    scheduler.start();
    scheduler2.start();
    sleep(1);
    SYLAR_LOG_DEBUG(g_logger) << "schedule more";
    scheduler.schedule(&run_fiber);
    scheduler2.schedule(&run_fiber);
    scheduler.stop();
    scheduler2.stop();
    SYLAR_LOG_DEBUG(g_logger) << "main end";
}

static std::atomic<int> s_count{0};

void fun(){
    static sylar::SpinLock s_mutex;
    for(int i = 0; i < 1000; i++){
        s_count++;
    }
}

void test2(){
    sylar::Scheduler scheduler(100, "testSch");
    scheduler.start();
    for(int i = 0; i != 100000; i++){
        scheduler.schedule(&fun);
    }
    scheduler.stop();
    SYLAR_LOG_INFO(g_logger) << s_count;
}

int main(){
    //test1();
    test2();

}