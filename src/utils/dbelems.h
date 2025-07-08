#ifndef DBELEMS_H
#define DBELEMS_H

#include <stdlib.h>
#include <stdint.h>  
#include <pthread.h>
#include <network/threads.h>

typedef struct VersionNode *VersionNode;
typedef struct Cell *Cell;
typedef struct Row *Row;
typedef struct Table *Table;

struct Record {
    uint64_t global_version;
    uint64_t local_version;
    uint8_t  type;        // 1=SET, 2=DELETE
    uint32_t key_len;
    uint32_t val_len;     // zero for DELETE
    // Flexible array members must be at the end and only one allowed
    char data[]; // key and value packed here (manual offset handling required)
};

struct VersionNode {
    void *value;
    uint64_t global_version;
    uint64_t local_version;
    VersionNode prev;
};

// DLL of values
struct Cell {
    VersionNode head;
    VersionNode tail;
    uint64_t size;
};

// Row nums -> parameters hashmap
struct Row {
    Hashmap cols;
};

// Threaded hashmap of ROWs
struct Table {
    pthread_rwlock_t lock;
    Hashmap rows;
};

#endif

