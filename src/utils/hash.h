#ifndef HASH_H
#define HASH_H

#include <stdint.h>

typedef struct Entry *Entry;

// Value is always a VersionNode
struct Entry {
    char *key;
    void *value;
    Entry next;
};

typedef struct Hashmap {
    Entry *buckets;
    uint64_t bucket_count;
    uint64_t size;
} Hashmap;

Hashmap hashmap_create(uint64_t bucket_count);
void hashmap_free(Hashmap map);
int hashmap_put(Hashmap map, const char *key, void *value, uint64_t global_version, void (free_value)(void *));
void *hashmap_get(Hashmap map, const char *key, uint64_t local_version);

#endif

