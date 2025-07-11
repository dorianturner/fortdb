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

static void entry_free(Entry entry, void (*free_value)(void *)) {
    while (entry) {
        Entry next = entry->next;
        if (free_value) free_value(entry->value);
        free(entry->key);
        free(entry);
        entry = next;
    }
}


void hashmap_free(Hashmap map, void (*free_value)(void *)) {
    if (!map) return;
    for (uint64_t i = 0; i < map->bucket_count; i++) {
        entry_free(map->buckets[i], free_value);
    }
    free(map->buckets);
    free(map);
}


// Pre: Any hashmap entries are a cell
int hashmap_put(Hashmap map, const char *key, void *value, uint64_t global_version) {
    if (!map || !key || !value) return -1;

    uint64_t index = hash(key) % map->bucket_count;
    Entry current = map->buckets[index];

    while (current) {
        if (strcmp(current->key, key) == 0) {
            return cell_put((Cell)(current->value), value, global_version);
        }
        current = current->next;
    }

    // Mid: key not found
    Cell cell = malloc(sizeof(struct Cell));
    if (!cell) return -1;

    cell->tail = NULL;
    cell->size = 0;

    if (cell_put(cell, value, global_version) != 0) {
        free(cell);
        return -1;
    }

    Entry new_entry = malloc(sizeof(struct Entry));
    if (!new_entry) {
        free_cell(cell);
        return -1;
    }

    new_entry->key = strdup(key);
    if (!new_entry->key) {
        free_cell(cell);
        free(new_entry);
        return -1;
    }

    new_entry->value = cell;
    new_entry->next = map->buckets[index];
    map->buckets[index] = new_entry;
    map->size++;

    return 0;
}

void *hashmap_get(Hashmap map, const char *key, uint64_t local_version) {
    if (!map || !key) return NULL;

    uint64_t index = hash(key) % map->bucket_count;
    Entry current = map->buckets[index];

    while (current) {
        if (strcmp(current->key, key) == 0) {
            return cell_get((Cell)(current->value), local_version);
        }
        current = current->next;
    }

    return NULL;
}


