#ifndef _SYLAR_FIBER_H_
#define _SYLAR_FIBER_H_

#include <string>
#include <stdio.h>
#include <ucontext.h>
#include <unistd.h>
#include <functional>
#include <memory>

namespace sylar{

class Fiber : public std::enable_shared_from_this<Fiber>{
public:
    typedef std::shared_ptr<Fiber> ptr;
    enum State{
        INIT = 0,
        EXEC = 1,
        READY = 2,
        HOLD = 3,
        TERM = 4,
    };

    Fiber(std::function<void()> cb, std::size_t stack_size = 0);
    ~Fiber();
    void reset(std::function<void()> cb);
    void swapIn();
    void swapOut();
    uint32_t getId(){return m_id;}
    State getState(){return m_state;}
    static Fiber::ptr GetThis();
    static void SetThis(Fiber*);
    static void YieldToReady();
    static void YieldToHold();
    static uint32_t GetFiberId();

private:
    void setState(State s);
    Fiber();
    static void MainFun();

private:
    std::function<void()> m_cb = nullptr;
    uint32_t m_id = 0;
    ucontext_t m_context;
    std::size_t m_stack_size = 0;
    void *m_stack = nullptr;
    State m_state = INIT;
};

}

#endif