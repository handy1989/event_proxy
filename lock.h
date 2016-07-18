#ifndef LOCK_H_
#define LOCK_H_

#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <iostream>

enum ELockMode {
    NO_PRIORITY,
    WRITE_PRIORITY,
    READ_PRIORITY
};

class RWLock {
  public:
    explicit RWLock(ELockMode lockMode = NO_PRIORITY);
    ~RWLock();

    inline int rdlock() {
        return pthread_rwlock_rdlock(&rwlock_);
    }

    inline int wrlock() {
        return pthread_rwlock_wrlock(&rwlock_);
    }

    inline int tryrdlock() {
        return pthread_rwlock_tryrdlock(&rwlock_);
    }

    inline int trywrlock() {
        return pthread_rwlock_trywrlock(&rwlock_);
    }

    inline int unlock() {
        return pthread_rwlock_unlock(&rwlock_);
    }

  private:
    pthread_rwlock_t rwlock_;
};

enum ELockType {
    READ_LOCKER,
    WRITE_LOCKER
};

class ScopedRWLock {
  public:
    ScopedRWLock(RWLock& locker, const ELockType lock_type)
        : locker_(locker) {

        if (lock_type == READ_LOCKER) {
            locker_.rdlock();
        } else {
            locker_.wrlock();
        }
    }

    ~ScopedRWLock() {
        locker_.unlock();
    }

  private:
    RWLock& locker_;
};

class SafeLock {
  public:
    SafeLock();
    ~SafeLock();
    inline int lock() {
        return pthread_mutex_lock(&lock_);
    }

    inline int unlock() {
        return pthread_mutex_unlock(&lock_);
    }
  protected:
    pthread_mutex_t lock_;
};

class ScopedSafeLock {
  public:
    explicit ScopedSafeLock(SafeLock& locker)
        : locker_(locker) {
        locker_.lock();
    }

    ~ScopedSafeLock() {
        locker_.unlock();
    }

  private:
    SafeLock& locker_;
};

#endif // LOCK_H_
