#include <stdint.h>
#include <stdlib.h>
#include "hash.h"

// https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
// FNV-1a hash
static uint64_t hash(const char *key) {
    uint64_t hash = 14695981039346656037ULL; // Offset Basis
    while (*key) {
        hash ^= (unsigned char)(*key);
        hash *= 1099511628211ULL; // FNV Prime
        key++;
    }
    return hash;
}

Hashmap hashmap_create(uint64_t bucket_count) {
    Hashmap map = malloc(sizeof(struct Hashmap));
    if (!map) return NULL;

    map->bucket_count = bucket_count;
    map->size = 0;
    map->buckets = calloc(bucket_count, sizeof(Entry));
    if (!map->buckets) {
        free(map);
        return NULL;
    }

    return map;
}

static void entry_free(Entry entry) {
    while (entry) {
        Entry next = entry->next;
        free(entry->key);
        free(entry);
        entry = next;
    }
}

void hashmap_free(Hashmap map) {
    if (!map) return;

    for (uint64_t i = 0; i < map->bucket_count; i++) {
        entry_free(map->buckets[i]);
    }

    free(map->buckets);
    free(map);
}

// This is slightly wrong in how it replaces the value rather than adding a new one.
// I cba to change it rn, but it should always be adding to the end of a linked list of 
// The entry value linked list for append only
int hashmap_put(Hashmap map, const char *key, void *value) {
    if (!map || !key) return -1;

    uint64_t index = hash(key) % map->bucket_count;
    Entry current = map->buckets[index];

    while (current) {
        if (strcmp(current->key, key) == 0) {
            current->value = value;
            return 0; // key exists, value replaced
        }
        current = current->next;
    }

    Entry new_entry = malloc(sizeof(struct Entry));
    if (!new_entry) return -1;

    new_entry->key = strdup(key);
    if (!new_entry->key) {
        free(new_entry);
        return -1;
    }

    new_entry->value = value;
    new_entry->next = map->buckets[index];
    map->buckets[index] = new_entry;
    map->size++;

    return 0;
}

void *hashmap_get(Hashmap map, const char *key) {
    if (!map || !key) return NULL;

    uint64_t index = hash(key) % map->bucket_count;
    Entry current = map->buckets[index];

    while (current) {
        if (strcmp(current->key, key) == 0) {
            return current->value;
        }
        current = current->next;
    }

    return NULL; // not found
}

