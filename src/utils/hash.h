#ifndef HASH_H
#define HASH_H

#include <stdint.h>
#include "version_node.h"
typedef struct Entry *Entry;
typedef struct Hashmap *Hashmap;

struct Hashmap {
    Entry *buckets;
    uint64_t bucket_count;
    uint64_t size;

    uint64_t datapoints;
    //using versionnode as a linked list, not how it was directly intended for versioning
    VersionNode head;
    VersionNode tail;
};

// Value is always a VersionNode
struct Entry {
    char *key;
    void *value;
    Entry next;
};



Hashmap hashmap_create(uint64_t bucket_count);
void hashmap_free(Hashmap map);
int hashmap_put(Hashmap map, const char *key, void *value, uint64_t global_version, void (*free_value)(void *));
void *hashmap_get(Hashmap map, const char *key, uint64_t local_version);
int hashmap_set_raw(Hashmap map, const char *key, void *value_chain);
Entry hashmap_find_entry(Hashmap map, const char *key);
#endif

