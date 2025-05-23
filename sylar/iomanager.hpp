#ifndef _SYLAR_IOMANAGER_H_
#define _SYLAR_IOMANAGER_H_

#include "scheduler.hpp"
#include <sys/epoll.h>
#include "timer.hpp"

namespace sylar{

class IOManager : public Scheduler, public TimerManager {
public:
    typedef std::shared_ptr<IOManager> ptr;
    typedef RWLock RWMutexType;

    enum Event{
        NONE = 0x00,
        READ = EPOLLIN,
        WRITE = EPOLLOUT,
    };

    static IOManager *GetThis();
    IOManager(std::size_t thread_count = 1, const std::string name = "");
    ~IOManager() override;
    int addEvent(int fd, Event event, std::function<void(void)> cb = nullptr);
    void delEvent(int fd, Event event);

protected:
    void tickle() override;
    void idle() override;
    bool stopping() override;
    void resizeFdContexts(std::size_t size);
    void onTimerInsertedAtFront() override;
    /**
     * @brief 判断是否可以停止
     * @param[out] timeout 最近要出发的定时器事件间隔
     * @return 返回是否可以停止
     */
    bool stopping(uint64_t& timeout);

private:
    struct FdContext{
        typedef std::shared_ptr<FdContext> ptr;
        typedef Mutex MutexType;
        struct EventContext{
            Scheduler *scheduler;
            std::function<void(void)> cb;
            Fiber::ptr fiber;
        };

        FdContext();
        ~FdContext();

        EventContext &getContext(Event event);
        void triggerContext(Event event);
        void resetContext(Event event);

        int fd;
        Event events = NONE;
        EventContext read;
        EventContext write;
        Mutex mutex;
    };

private:
    int m_epoll_fd;
    int m_tickleFds[2];
    std::atomic<std::size_t> m_pendingEventCount = {0};
    std::vector<FdContext::ptr> m_fd_contexts;
    RWMutexType m_mutex;
};

}

#endif