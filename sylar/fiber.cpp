#include "fiber.hpp"
#include <atomic>
#include <stdlib.h>
#include "macro.hpp"
#include "log.hpp"

namespace sylar{

static thread_local Fiber *t_fiber = Fiber::GetThis().get();
static thread_local Fiber::ptr t_thread_fiber = nullptr;

static std::atomic<uint32_t> s_fiber_id(0);
static std::atomic<uint32_t> s_fiber_count(0);

static std::size_t Get_stack_size(){return 1024 * 128;}

Logger::ptr g_logger = SYLAR_LOG_NAME("system");

Fiber::Fiber(std::function<void()> cb, std::size_t stack_size)
    :m_cb(cb)
    ,m_id(++s_fiber_id){
    SYLAR_ASSERT(m_cb != nullptr);
    s_fiber_count++;
    if(stack_size == 0){
        m_stack_size = Get_stack_size();
    } else {
        m_stack_size = stack_size;
    }

    m_stack = malloc(m_stack_size);
    SYLAR_ASSERT2(m_stack != NULL, "malloc failed");
    
    if(getcontext(&m_context)){
        SYLAR_ASSERT2(false, "getcontext error");
    }
    m_context.uc_link = nullptr;
    m_context.uc_stack.ss_sp = m_stack;
    m_context.uc_stack.ss_size = m_stack_size;
    makecontext(&m_context, &MainFun, 0);
    //SYLAR_LOG_DEBUG(g_logger) << "Fiber() id = " << m_id;
}

Fiber::~Fiber(){
    s_fiber_count--;
    if(m_stack != nullptr){
        SYLAR_ASSERT(m_state == INIT || m_state == TERM);
        free(m_stack);
    } else {
        SYLAR_ASSERT(!m_cb);
        SYLAR_ASSERT(m_state == EXEC);
    }
    // SYLAR_LOG_DEBUG(g_logger) << "~Fiber() id = " << m_id;
}

Fiber::Fiber(){
    //m_state = EXEC;
    setState(EXEC);
    if(getcontext(&m_context)){
        SYLAR_ASSERT2(false, "getcontext error");
    }
    SetThis(this);
    s_fiber_count++;
    // SYLAR_LOG_DEBUG(g_logger) << "Fiber() id = " << m_id;
}

void Fiber::SetThis(Fiber* f){
    t_fiber = f;
}

Fiber::ptr Fiber::GetThis(){
    if(t_fiber != nullptr){
        return t_fiber->shared_from_this();
    }
    Fiber::ptr main_fiber(new Fiber);
    SYLAR_ASSERT(t_fiber == main_fiber.get());
    t_thread_fiber = main_fiber;
    return t_fiber->shared_from_this();
}

uint32_t Fiber::GetFiberId(){
    if(t_fiber != nullptr)
        return t_fiber->getId();
    return 0;
}

void Fiber::reset(std::function<void()> cb){
    SYLAR_ASSERT(m_stack);
    SYLAR_ASSERT(m_state == TERM || m_state == INIT);
    m_cb = cb;
    if(getcontext(&m_context)){
        SYLAR_ASSERT2(false, "getcontext error");
    }
    m_context.uc_link = nullptr;
    m_context.uc_stack.ss_sp = m_stack;
    m_context.uc_stack.ss_size = m_stack_size;
    makecontext(&m_context, &MainFun, 0);
    //m_state = INIT;
    setState(INIT);
}

void Fiber::setState(State s){
    m_state = s;
}

void Fiber::swapIn(){
    SetThis(this);
    SYLAR_ASSERT(m_state != EXEC);
    //m_state = EXEC;
    setState(EXEC);
    if(swapcontext(&t_thread_fiber->m_context, &m_context))
        SYLAR_ASSERT2(false, "swapcontext error");
}

void Fiber::swapOut(){
    SetThis(t_thread_fiber.get());
    if(swapcontext(&m_context, &t_thread_fiber->m_context))
        SYLAR_ASSERT2(false, "swapcontext error");
}

void Fiber::YieldToReady(){
    Fiber::ptr cur = GetThis();
    SYLAR_ASSERT(cur->m_state == EXEC);
    //cur->m_state = READY;
    cur->setState(READY);
    cur->swapOut();
}

void Fiber::YieldToHold(){
    Fiber::ptr cur = GetThis();
    SYLAR_ASSERT(cur->m_state == EXEC);
    //cur->m_state = HOLD;
    cur->setState(HOLD);
    cur->swapOut();
}

void Fiber::MainFun(){
    Fiber::ptr cur = GetThis();
    SYLAR_ASSERT(cur != nullptr);
    try{
        cur->m_cb();
        //cur->m_state = TERM;
        cur->setState(TERM);
        cur->m_cb = nullptr;
    } catch (...) {
        //cur->m_state = TERM;
        cur->setState(TERM);
        SYLAR_LOG_ERROR(g_logger) << "Fiber Except"
            << " fiber_id=" << cur->getId()
            << std::endl
            << sylar::BacktraceToString();
    }
    auto raw_ptr = cur.get();
    cur.reset();
    raw_ptr->swapOut();

    SYLAR_ASSERT2(false, "never reach");
}

}