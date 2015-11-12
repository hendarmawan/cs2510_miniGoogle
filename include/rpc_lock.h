#ifndef __RPC_LOCK_H__
#define __RPC_LOCK_H__

#include <pthread.h>
#ifdef __APPLE__
#include <libkern/OSAtomic.h>
#endif

class mutex_lock {
    public:
        mutex_lock() { }
        virtual ~mutex_lock() { }

    public:
        int init() {
            return pthread_mutex_init(&m_lock, NULL);
        }

        int lock() {
            return pthread_mutex_lock(&m_lock);
        }

        int unlock() {
            return pthread_mutex_unlock(&m_lock);
        }

        void destroy() {
            pthread_mutex_destroy(&m_lock);
        }

    private:
        pthread_mutex_t m_lock;
};

class rw_lock {
    public:
        rw_lock() { }
        virtual ~rw_lock() { }

    public:
        int init() {
            return pthread_rwlock_init(&m_lock, NULL);
        }

        int rdlock() {
            return pthread_rwlock_rdlock(&m_lock);
        }

        int wrlock() {
            return pthread_rwlock_wrlock(&m_lock);
        }

        int unlock() {
            return pthread_rwlock_unlock(&m_lock);
        }

        void destroy() {
            pthread_rwlock_destroy(&m_lock);
        }

    private:
        pthread_rwlock_t m_lock;
};

class auto_wrlock {
    public:
        auto_wrlock(rw_lock *lock) : m_lock(lock) {
            m_lock->wrlock();
        }

        virtual ~auto_wrlock() {
            m_lock->unlock();
        }

    private:
        rw_lock *m_lock;
};

class auto_rdlock {
    public:
        auto_rdlock(rw_lock *lock) : m_lock(lock) {
            m_lock->rdlock();
        }

        virtual ~auto_rdlock() {
            m_lock->unlock();
        }

    private:
        rw_lock *m_lock;
};

class spin_lock {
    public:
        spin_lock() { }
        virtual ~spin_lock() { }

    public:
        int init() {
#ifdef __APPLE__
            m_lock = OS_SPINLOCK_INIT;
            return 0;
#elif __linux
            return pthread_spin_init(&m_lock, PTHREAD_PROCESS_PRIVATE);
#endif
        }

        int lock() {
#ifdef __APPLE__
            OSSpinLockLock(&m_lock);
            return 0;
#elif __linux
            return pthread_spin_lock(&m_lock);
#endif
        }

        int unlock() {
#ifdef __APPLE__
            OSSpinLockUnlock(&m_lock);
            return 0;
#elif __linux
            return pthread_spin_unlock(&m_lock);
#endif
        }

        void destroy() {
#ifdef __linux
            pthread_spin_destroy(&m_lock);
#endif
        }

    private:
#ifdef __APPLE__
        OSSpinLock m_lock;
#elif __linux
        pthread_spinlock_t m_lock;
#endif
};

#endif
