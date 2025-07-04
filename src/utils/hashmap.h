#ifndef HASHMAP_H
#define HASHMAP_H

#include <stdint.h>
#include <stdlib.h>

typedef struct kv_entry *kv_entry;
struct kv_entry {
    char *key;
    void *value;
    kv_entry next;
};

typedef struct hashmap *hashmap;
struct hashmap {
    kv_entry *buckets;
    size_t size;
};


hashmap hashmap_create(size_t initial_size);
void hashmap_destroy(hashmap map, void (*free_cb)(void *));
int hashmap_put(hashmap map, const char *key, void *value);
void *hashmap_get(hashmap map, const char *key);
uint64_t fnv1a_hash(const char *key);

#endif
