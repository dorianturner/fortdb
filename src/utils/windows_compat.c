
#include "windows_compat.h"

int pthread_rwlock_init(pthread_rwlock_t *lock, void *attr) {
    (void)attr; // unused
    InitializeSRWLock(lock);
    return 0;
}

int pthread_rwlock_rdlock(pthread_rwlock_t *lock) {
    AcquireSRWLockShared(lock);
    return 0;
}

int pthread_rwlock_wrlock(pthread_rwlock_t *lock) {
    AcquireSRWLockExclusive(lock);
    return 0;
}

int pthread_rwlock_unlock(pthread_rwlock_t *lock) {
    // We can’t tell if it's held shared/exclusive, so caller must be consistent
    ReleaseSRWLockShared(lock); // or ReleaseSRWLockExclusive
    return 0;
}

int pthread_rwlock_destroy(pthread_rwlock_t *lock) {
    (void)lock;
    // Nothing to do for SRWLOCK
    return 0;
}
// Mutex
int pthread_mutex_init(pthread_mutex_t *mutex, void *attr) {
    (void)attr;
    InitializeCriticalSection(mutex);
    return 0;
}
int pthread_mutex_lock(pthread_mutex_t *mutex) {
    EnterCriticalSection(mutex);
    return 0;
}
int pthread_mutex_unlock(pthread_mutex_t *mutex) {
    LeaveCriticalSection(mutex);
    return 0;
}
int pthread_mutex_destroy(pthread_mutex_t *mutex) {
    DeleteCriticalSection(mutex);
    return 0;
}