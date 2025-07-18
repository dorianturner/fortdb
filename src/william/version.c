#include "version.h"
#include <stdlib.h>
#include <pthread.h>

static uint64_t global_version = 0;
static pthread_mutex_t version_lock = PTHREAD_MUTEX_INITIALIZER;

int version_init() {
    global_version = 0;
    return 0;
}

uint64_t version_current() {
    pthread_mutex_lock(&version_lock);
    uint64_t current = global_version;
    pthread_mutex_unlock(&version_lock);
    return current;
}

uint64_t version_next() {
    pthread_mutex_lock(&version_lock);
    uint64_t next = ++global_version;
    pthread_mutex_unlock(&version_lock);
    return next;
}

void version_cleanup() {
    pthread_mutex_destroy(&version_lock);
}