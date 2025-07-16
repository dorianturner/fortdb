#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <pthread.h>

typedef enum {
    RECORD_SET = 1,
    RECORD_DELETE = 2
} RecordType;

// Append-only log record layout
// IDK if this is needed for now
struct Record {
    uint64_t global_version;  
    RecordType type;         
    uint32_t key_len;
    uint32_t val_len;
    char data[];              // key followed by value
};

extern uint64_t GLOBAL_VERSION;
extern pthread_mutex_t GLOBAL_VERSION_LOCK;

#endif

