#include "lock.h"

RWLock::RWLock(ELockMode lockMode) 
{
    pthread_rwlockattr_t attr;
    pthread_rwlockattr_init(&attr);
    if (lockMode == READ_PRIORITY) 
    {
        pthread_rwlockattr_setkind_np(&attr, PTHREAD_RWLOCK_PREFER_READER_NP);
    } 
    else if (lockMode == WRITE_PRIORITY) 
    {
        pthread_rwlockattr_setkind_np(&attr, PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP);
    }
    pthread_rwlock_init(&rwlock_, &attr);
}

RWLock::~RWLock() 
{
    pthread_rwlock_destroy(&rwlock_);
}



SafeLock::SafeLock() 
{
    pthread_mutex_init(&lock_, NULL);
}

SafeLock::~SafeLock() 
{
    pthread_mutex_destroy(&lock_);
}
