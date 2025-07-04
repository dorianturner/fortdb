#include <stdint.h>
#include <stdlib.h>
#include "hashmap.h"

hashmap hashmap_create(size_t initial_size) {
    hashmap map = malloc(sizeof(struct hashmap));
    if (!map) return NULL;

    map->buckets = calloc(initial_size, sizeof(kv_entry));
    if (!map->buckets) {
        free(map);
        return NULL;
    }

    map->size = initial_size;
    return map;
}

void hashmap_destroy(hashmap map, void (*free_cb)(void *)) {
    if (!map) return;

    for (size_t i = 0; i < map->size; ++i) {
        kv_entry entry = map->buckets[i];
        while (entry) {
            kv_entry next = entry->next;
            if (free_cb) free_cb(entry->value);
            free(entry->key); // maybe not needed but you could've strdup'd
            free(entry);
            entry = next;
        }
    }

    free(map->buckets);
    free(map);
}

static uint64_t fnv1a_hash(const char *key) {
    uint64_t hash = 14695981039346656037ULL; // FNV offset basis
    while (*key) {
        hash ^= (unsigned char)*key++;
        hash *= 1099511628211ULL; // FNV prime
    }
    return hash;
}

int hashmap_put(hashmap map, const char *key, void *value, void (*put_cb)(list_t versions, void *entry)) {
    if (!map || !key) return -1;

    uint64_t hash = fnv1a_hash(key);
    size_t index = hash % map->size;

    kv_entry current = map->buckets[index];
    while (current != NULL) {
        if (strcmp(current->key, key) == 0) {
            if (put_cb != NULL) {
                put_cb((list_t)current->value, value);
            } else {
                current->value = value;
            }
            return 0;
        }
        current = current->next;
    }

    // Key not found — create new value
    void *stored_value;

    if (put_cb != NULL) {
        list_t new_list = list_create();
        if (!new_list) return -1;
        put_cb(new_list, value);
        stored_value = new_list;
    } else {
        stored_value = value;
    }

    kv_entry new_entry = malloc(sizeof(struct kv_entry));
    if (!new_entry) {
        if (put_cb != NULL) list_destroy((list_t)stored_value, NULL);
        return -1;
    }

    new_entry->key = strdup(key);
    new_entry->value = stored_value;
    new_entry->next = map->buckets[index];
    map->buckets[index] = new_entry;

    return 0;
}

