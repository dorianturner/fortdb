#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include "hash.h"
#include "version_node.h"

#define LOAD_FACTOR 0.75

// FNV-1a hashing
static uint64_t hash(const char *key) {
    uint64_t hash = 14695981039346656037ULL;
    while (*key) {
        hash ^= (unsigned char)(*key++);
        hash *= 1099511628211ULL;
    }
    return hash;
}

Hashmap hashmap_create(uint64_t bucket_count) {
    Hashmap map = malloc(sizeof(struct Hashmap));
    if (!map) return NULL;

    map->datapoints   = 0;
    map->bucket_count = bucket_count;
    map->size         = 0;
    map->head         = NULL;
    map->tail         = NULL;
    map->buckets      = calloc(bucket_count, sizeof(Entry));
    if (!map->buckets) {
        free(map);
        return NULL;
    }
    return map;
}

static void entry_free(Entry entry) {
    while (entry) {
        Entry next = entry->next;
        version_node_free(entry->value);
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

static int hashmap_rehash(Hashmap map, uint64_t new_bucket_count) {
    Entry *new_buckets = calloc(new_bucket_count, sizeof(Entry));
    if (!new_buckets) return -1;

    for (uint64_t i = 0; i < map->bucket_count; i++) {
        Entry current = map->buckets[i];
        while (current) {
            Entry next = current->next;
            uint64_t index = hash(current->key) % new_bucket_count;
            current->next = new_buckets[index];
            new_buckets[index] = current;
            current = next;
        }
    }

    free(map->buckets);
    map->buckets      = new_buckets;
    map->bucket_count = new_bucket_count;
    return 0;
}

int hashmap_put(Hashmap map, const char *key, void *value,
                uint64_t global_version, void (free_value)(void *)) {
    if (!map || !key || !value) return -1;

    /* Grow if load factor exceeded */
    if ((double)(map->size + 1) / (double)map->bucket_count > LOAD_FACTOR) {
        (void)hashmap_rehash(map, map->bucket_count * 2);
    }

    uint64_t index = hash(key) % map->bucket_count;
    Entry current  = map->buckets[index];

    // Update existing key
    while (current) {
        if (strcmp(current->key, key) == 0) {
            VersionNode old_head    = (VersionNode)current->value;
            uint64_t local_version  = old_head ? old_head->local_version + 1 : 1;
            VersionNode new_head    = version_node_create(
                                          value, global_version,
                                          local_version, old_head, free_value);
            if (!new_head) return -1;
            current->value = new_head;

            // serialization bookkeeping
            map->datapoints++;
            if (map->head) {
                map->tail->prev = new_head;
                map->tail       = new_head;
            } else {
                map->head = map->tail = new_head;
            }
            return 0;
        }
        current = current->next;
    }

    // Insert new key
    VersionNode new_head = version_node_create(value, global_version, 1, NULL, free_value);
    if (!new_head) return -1;

    Entry new_entry = malloc(sizeof(struct Entry));
    if (!new_entry) {
        version_node_free(new_head);
        return -1;
    }

    new_entry->key = strdup(key);
    if (!new_entry->key) {
        version_node_free(new_head);
        free(new_entry);
        return -1;
    }

    index = hash(key) % map->bucket_count; // in case rehash happened
    new_entry->value = new_head;
    new_entry->next  = map->buckets[index];
    map->buckets[index] = new_entry;
    map->size++;

    // serialization bookkeeping
    map->datapoints++;
    if (map->head) {
        map->tail->prev = new_head;
        map->tail       = new_head;
    } else {
        map->head = map->tail = new_head;
    }
    return 0;
}

void *hashmap_get(Hashmap map, const char *key, uint64_t local_version) {
    if (!map || !key) return NULL;

    uint64_t index = hash(key) % map->bucket_count;
    Entry current  = map->buckets[index];

    while (current) {
        if (strcmp(current->key, key) == 0) {
            VersionNode head = (VersionNode)current->value;

            // local_version == 0 → latest
            if (local_version == 0) {
                return head ? head->value : NULL;
            }
            while (head && head->local_version >= local_version) {
                if (head->local_version == local_version) {
                    return head->value;
                }
                head = head->prev;
            }
            return NULL;
        }
        current = current->next;
    }
    return NULL;
}

static int hashmap_set_raw(Hashmap map, const char *key, void *value_chain) {
    if (!map || !key) return -1;

    uint64_t index = hash(key) % map->bucket_count;

    Entry new_entry = malloc(sizeof(struct Entry));
    if (!new_entry) return -1;
    new_entry->key = strdup(key);
    if (!new_entry->key) { free(new_entry); return -1; }
    new_entry->value = value_chain;
    new_entry->next = map->buckets[index];
    map->buckets[index] = new_entry;
    map->size++;
    return 0;
}
