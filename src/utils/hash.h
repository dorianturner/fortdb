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

Hashmap hashmap_create(uint64_t bucket_count);
void hashmap_free(Hashmap map);
int hashmap_put(Hashmap map, const char *key, void *value);
void *hashmap_get(Hashmap map, const char *key);

#endif
