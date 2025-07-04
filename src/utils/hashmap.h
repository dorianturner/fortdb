#ifndef HASHMAP_H
#define HASHMAP_H

#include <stdint.h>
#include <stdlib.h>
#include <list.h>

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
void *hashmap_get(hashmap map, const char *key);
int hashmap_put(
    hashmap map,
    const char *key,
    void *value,
    void (*put_cb)(list versions, void *entry)
);


#endif
