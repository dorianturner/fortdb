#pragma once

#include <stddef.h>
#include <windows.h>

// Type aliases so pthread-like names exist on Windows
typedef SRWLOCK pthread_rwlock_t;

// Function declarations
int pthread_rwlock_init(pthread_rwlock_t *lock, void *attr);
int pthread_rwlock_rdlock(pthread_rwlock_t *lock);
int pthread_rwlock_wrlock(pthread_rwlock_t *lock);
int pthread_rwlock_unlock(pthread_rwlock_t *lock);
int pthread_rwlock_destroy(pthread_rwlock_t *lock);
