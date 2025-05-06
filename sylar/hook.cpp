#include "hook.hpp"
#include <dlfcn.h>
#include "log.hpp"
#include "fiber.hpp"
#include "iomanager.hpp"
#include "macro.hpp"

sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");
namespace sylar {

static thread_local bool t_hook_enable = false;

#define HOOK_FUN(XX) \
    XX(sleep) \
    XX(accept) \
    XX(read) \
    XX(write) \

void hook_init() {
    static bool is_inited = false;
    if(is_inited) {
        return;
    }
#define XX(name) name ## _f = (name ## _fun)dlsym(RTLD_NEXT, #name);
    HOOK_FUN(XX);
#undef XX
}

struct _HookIniter {
    _HookIniter() {
        hook_init();
    }
};

static _HookIniter s_hook_initer;

bool is_hook_enable() {
    return t_hook_enable;
}

void set_hook_enable(bool flag) {
    t_hook_enable = flag;
}

}

struct timer_info {
    int cancelled = 0;
};

template<typename OriginFun, typename... Args>
static ssize_t do_io(int fd, OriginFun fun, const char* hook_fun_name, uint32_t event, Args&&... args) {
    if(!sylar::t_hook_enable) {
        return fun(fd, std::forward<Args>(args)...);
    }
    if(fd <= 4){
        return fun(fd, std::forward<Args>(args)...);
    }

retry:
    ssize_t n = fun(fd, std::forward<Args>(args)...);
    while(n == -1 && errno == EINTR) {
        n = fun(fd, std::forward<Args>(args)...);
    }
    if(n == -1 && errno == EAGAIN) {
        sylar::IOManager* iom = sylar::IOManager::GetThis();
        int rt = iom->addEvent(fd, (sylar::IOManager::Event)(event));
        if(rt == 0){
            sylar::Fiber::YieldToHold();
            goto retry;
        }
        else {
            return -1;
        };
    }
    return n;
}


extern "C" {
#define XX(name) name ## _fun name ## _f = nullptr;
    HOOK_FUN(XX);
#undef XX

unsigned int sleep(unsigned int seconds) {
    if(!sylar::t_hook_enable) {
        return sleep_f(seconds);
    }

    sylar::Fiber::ptr fiber = sylar::Fiber::GetThis();
    sylar::IOManager* iom = sylar::IOManager::GetThis();
    iom->addTimer(seconds * 1000, std::bind((void(sylar::Scheduler::*)(sylar::Fiber::ptr, int thread))&sylar::IOManager::schedule, iom, fiber, -1));
    sylar::Fiber::YieldToHold();
    return 0;
}

int accept(int fd, struct sockaddr *addr, socklen_t *addrlen) {
    return do_io(fd, accept_f, "accept", sylar::IOManager::READ, addr, addrlen);
}

ssize_t read(int fd, void *buf, size_t count) {
    return do_io(fd, read_f, "read", sylar::IOManager::READ, buf, count);
}

ssize_t write(int fd, const void *buf, size_t count) {
    return do_io(fd, write_f, "write", sylar::IOManager::WRITE, buf, count);
}

}
