#include "sylar/fiber.hpp"
#include "sylar/log.hpp"
#include "sylar/thread.hpp"

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("root");

void run_fiber(){
    SYLAR_LOG_DEBUG(g_logger) << "  fiber begin";
    sylar::Fiber::YieldToReady();
    SYLAR_LOG_DEBUG(g_logger) << "  fiber mid";
    sylar::Fiber::YieldToHold();
    SYLAR_LOG_DEBUG(g_logger) << "  fiber end";
}

void test_fiber(){
    SYLAR_LOG_DEBUG(g_logger) << " test begin";
    sylar::Fiber::ptr fiber(new sylar::Fiber(&run_fiber));
    SYLAR_LOG_DEBUG(g_logger) << " test: fiber state = " << fiber->getState();
    fiber->swapIn();
    SYLAR_LOG_DEBUG(g_logger) << " test: fiber state = " << fiber->getState();
    fiber->swapIn();
    SYLAR_LOG_DEBUG(g_logger) << " test: fiber state = " << fiber->getState();
    fiber->swapIn();
    SYLAR_LOG_DEBUG(g_logger) << " test: fiber state = " << fiber->getState();
}

int main(){
    SYLAR_LOG_DEBUG(g_logger) << "main begin";
    std::vector<sylar::Thread::ptr> thread_pool;
    thread_pool.resize(5);
    for(int i = 0; i != 5; i++){
        //thread_pool.push_back(sylar::Thread::ptr(new sylar::Thread(&test_fiber, "t_name_" + std::to_string(i))));
        thread_pool[i].reset(new sylar::Thread(&test_fiber, "t_name_" + std::to_string(i)));
    }

    for(auto i : thread_pool){
        i->join();
    }
    SYLAR_LOG_DEBUG(g_logger) << "main end";
}