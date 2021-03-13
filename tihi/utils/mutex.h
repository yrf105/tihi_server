#ifndef TIHI_UTILS_MUTEX_H_
#define TIHI_UTILS_MUTEX_H_

#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>

#include <atomic>

#include "utils/noncopyable.h"

namespace tihi {

class Semaphore : public Noncopyable {
public:
    Semaphore(const uint32_t count = 0);
    ~Semaphore();

    void wait();
    void notify();

private:
    sem_t semaphore_;
};

template <typename T>
class ScopedLock {
public:
    ScopedLock(T& mutex) : mutex_(mutex) { lock(); }

    ~ScopedLock() { unlock(); }

    void lock() {
        if (!locked_) {
            mutex_.lock();
            locked_ = true;
        }
    }

    void unlock() {
        if (locked_) {
            mutex_.unlock();
            locked_ = false;
        }
    }

private:
    T& mutex_;
    bool locked_ = false;
};

template <typename T>
class ScopedReadLock {
public:
    ScopedReadLock(T& mutex) : mutex_(mutex) { lock(); }

    ~ScopedReadLock() { unlock(); }

    void lock() {
        if (!locked_) {
            mutex_.rdlock();
            locked_ = true;
        }
    }

    void unlock() {
        if (locked_) {
            mutex_.unlock();
            locked_ = false;
        }
    }

private:
    T& mutex_;
    bool locked_ = false;
};

template <typename T>
class ScopedWriteLock {
public:
    ScopedWriteLock(T& mutex) : mutex_(mutex) { lock(); }

    ~ScopedWriteLock() { unlock(); }

    void lock() {
        if (!locked_) {
            mutex_.wrlock();
            locked_ = true;
        }
    }

    void unlock() {
        if (locked_) {
            mutex_.unlock();
            locked_ = false;
        }
    }

private:
    T& mutex_;
    bool locked_ = false;
};

class RWMutex {
public:
    using read_lock = ScopedReadLock<RWMutex>;
    using write_lock = ScopedWriteLock<RWMutex>;

    RWMutex() { pthread_rwlock_init(&rwlock_, nullptr); }

    ~RWMutex() { pthread_rwlock_destroy(&rwlock_); }

    void rdlock() { pthread_rwlock_rdlock(&rwlock_); }

    void wrlock() { pthread_rwlock_wrlock(&rwlock_); }

    void unlock() { pthread_rwlock_unlock(&rwlock_); }

private:
    pthread_rwlock_t rwlock_;
};

class NullRWMutex {
public:
    using read_lock = ScopedReadLock<NullRWMutex>;
    using write_lock = ScopedWriteLock<NullRWMutex>;

    NullRWMutex() {}

    ~NullRWMutex() {}

    void rdlock() {}

    void wrlock() {}

    void unlock() {}

private:
    pthread_rwlock_t rwlock_;
};

class Mutex {
public:
    using mutex = ScopedLock<Mutex>;
    Mutex() { pthread_mutex_init(&mutex_, nullptr); }

    ~Mutex() { pthread_mutex_destroy(&mutex_); }

    void lock() { pthread_mutex_lock(&mutex_); }

    void unlock() { pthread_mutex_unlock(&mutex_); }

private:
    pthread_mutex_t mutex_;
};

class NullMutex {
public:
    using null_mutex = ScopedLock<NullMutex>;
    NullMutex() {}

    ~NullMutex() {}

    void lock() {}

    void unlock() {}
};

class SpinLock {
public:
    using spinlock = ScopedLock<SpinLock>;

    SpinLock() {
        pthread_spin_lock(&spinlock_);
    }

    ~SpinLock() {
        pthread_spin_destroy(&spinlock_);
    }

    void lock() {
        pthread_spin_lock(&spinlock_);
    }

    void unlock() {
        pthread_spin_unlock(&spinlock_);
    }

private:
    pthread_spinlock_t spinlock_;

};

class CASLock {
public:
    using cas_lock = ScopedLock<CASLock>;

    CASLock() {
        mutex_.clear();
    }

    ~CASLock() {

    }

    void lock() {
        while (std::atomic_flag_test_and_set_explicit(&mutex_, std::memory_order_acquire));
    }

    void unlock() {
        std::atomic_flag_clear_explicit(&mutex_, std::memory_order_release);
    }

private:
    volatile std::atomic_flag mutex_;

};

}  // namespace tihi

#endif  // TIHI_UTILS_MUTEX_H_