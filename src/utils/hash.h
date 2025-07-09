#ifndef HASH_H
#define HASH_H

#include <stdint.h>

typedef struct Hashmap *Hashmap;
typedef struct Entry *Entry;

struct Entry {
    char *key;
    void *value;
    Entry next;
};

struct Hashmap {
    Entry *buckets;
    uint64_t bucket_count;
    uint64_t size;
};

#endif
