#ifndef _SYLAR_MUTEX_H_
#define _SYLAR_MUTEX_H_

#include <semaphore.h>
#include <pthread.h>

namespace sylar{

class Semaphore{
public:
    Semaphore(unsigned int value = 0);
    ~Semaphore();
    void wait();
    void notify();
private:
    sem_t m_sem;
};

template<typename T>
class ScopedLock{
public:
    ScopedLock(T &mutex)
        :m_mutex(mutex){
        m_mutex.lock();
    }

    ~ScopedLock(){
        if(m_locked){
            m_mutex.unlock();
            m_locked = false;
        }
    }

    void unlock(){
        if(m_locked){
            m_mutex.unlock();
            m_locked = false;
        }
    }

private:
    T &m_mutex;
    bool m_locked = true;
};

template<typename T>
class ScopedReadLock{
public:
    ScopedReadLock(T &rwlock)
        :m_rwlock(rwlock){
        m_rwlock.rdlock();
    }

    ~ScopedReadLock(){
        if(m_locked){
            m_rwlock.unlock();
            m_locked = false;
        }
    }

    void unlock(){
        if(m_locked){
            m_rwlock.unlock();
            m_locked = false;
        }
    }

private:
    T &m_rwlock;
    bool m_locked = true;
};

template<typename T>
class ScopedWriteLock{
public:
    ScopedWriteLock(T &rwlock)
        :m_rwlock(rwlock){
        m_rwlock.wrlock();
    }

    ~ScopedWriteLock(){
        if(m_locked){
            m_rwlock.unlock();
            m_locked = false;
        }
    }

    void unlock(){
        if(m_locked){
            m_rwlock.unlock();
            m_locked = false;
        }
    }

private:
    T &m_rwlock;
    bool m_locked = true;
};

class Mutex{
public:
    typedef ScopedLock<Mutex> Lock;

    Mutex(){
        pthread_mutex_init(&m_mutex, nullptr);
    }

    ~Mutex(){
        pthread_mutex_destroy(&m_mutex);
    }

    void lock(){
        pthread_mutex_lock(&m_mutex);
    }

    void unlock(){
        pthread_mutex_unlock(&m_mutex);
    }

private:
    pthread_mutex_t m_mutex;
};

class SpinLock{
public:
    typedef ScopedLock<SpinLock> Lock; 
    SpinLock(){
        pthread_spin_init(&m_spinlock, 0);
    }

    ~SpinLock(){
        pthread_spin_destroy(&m_spinlock);
    }

    void lock(){
        pthread_spin_lock(&m_spinlock);
    }

    void unlock(){
        pthread_spin_unlock(&m_spinlock);
    }

private:
    pthread_spinlock_t m_spinlock;
};

class RWLock{
public:
    typedef ScopedReadLock<RWLock> ReadLock;
    typedef ScopedWriteLock<RWLock> WriteLock;
    RWLock(){
        pthread_rwlock_init(&m_rwlock, nullptr);
    }

    ~RWLock(){
        pthread_rwlock_destroy(&m_rwlock);
    }

    void rdlock(){
        pthread_rwlock_rdlock(&m_rwlock);
    }

    void wrlock(){
        pthread_rwlock_wrlock(&m_rwlock);
    }

    void unlock(){
        pthread_rwlock_unlock(&m_rwlock);
    }
private:
    pthread_rwlock_t m_rwlock;
};

}

#endif