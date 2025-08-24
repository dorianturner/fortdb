#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include "hash.h"
#include "version_node.h"

#ifndef DELETED
#define DELETED ((void*)1)
#endif


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

    /* Update existing key */
    while (current) {
        if (strcmp(current->key, key) == 0) {
            VersionNode old_head   = (VersionNode)current->value;
            uint64_t local_version = old_head ? old_head->local_version + 1 : 1;
            VersionNode new_head = version_node_create(value, global_version,
                                                       local_version, old_head, free_value);
            if (!new_head) return -1;
            current->value = new_head;
            return 0;
        }
        current = current->next;
    }

    /* Insert new key */
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

    index = hash(key) % map->bucket_count; /* re-evaluate in case rehash occurred */
    new_entry->value = new_head;
    new_entry->next  = map->buckets[index];
    map->buckets[index] = new_entry;
    map->size++;

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

int hashmap_set_raw(Hashmap map, const char *key, void *value_chain) {
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

Entry hashmap_find_entry(Hashmap map, const char *key) {
    if (!map || !key) return NULL;
    for (uint64_t i = 0; i < map->bucket_count; i++) {
        Entry e = map->buckets[i];
        while (e) {
            if (strcmp(e->key, key) == 0) return e;
            e = e->next;
        }
    }
    return NULL;
}

// Document get path helpers
// Compat between UINT64 as latest vs expected 0, not ideal
void *hashmap_get_version(Hashmap map, const char *key, uint64_t local_version) {
    if (!map || !key) return NULL;
    if (local_version == UINT64_MAX) return hashmap_get(map, key, 0); /* underlying uses 0 => latest */
    return hashmap_get(map, key, local_version);
}

char **hashmap_collect_live_keys(Hashmap map, size_t *out_count) {
    if (!map || !out_count) return NULL;
    size_t cap = 16, n = 0;
    char **arr = malloc(cap * sizeof(char*));
    if (!arr) { *out_count = 0; return NULL; }

    for (uint64_t i = 0; i < map->bucket_count; ++i) {
        Entry e = map->buckets[i];
        while (e) {
            VersionNode vh = (VersionNode)e->value;
            if (vh && vh->value != DELETED) {
                if (n >= cap) {
                    size_t nc = cap * 2;
                    char **tmp = realloc(arr, nc * sizeof(char*));
                    if (!tmp) {
                        /* cleanup */
                        for (size_t j = 0; j < n; ++j) free(arr[j]);
                        free(arr);
                        *out_count = 0;
                        return NULL;
                    }
                    arr = tmp;
                    cap = nc;
                }
                arr[n++] = strdup(e->key);
            }
            e = e->next;
        }
    }

    if (n == 0) {
        free(arr);
        *out_count = 0;
        return NULL;
    }

    char **sh = realloc(arr, n * sizeof(char*));
    if (sh) arr = sh;
    *out_count = n;
    return arr;
}

char *hashmap_join_live_keys(Hashmap map) {
    size_t count = 0;
    char **keys = hashmap_collect_live_keys(map, &count);
    if (!keys) return strdup(""); /* no live keys */

    /* compute length (fast path) */
    size_t total = 0;
    for (size_t i = 0; i < count; ++i) total += strlen(keys[i]) + 2; /* comma+space */
    char *out = malloc(total + 1);
    if (!out) {
        for (size_t i = 0; i < count; ++i) free(keys[i]);
        free(keys);
        return strdup("");
    }
    out[0] = '\0';
    for (size_t i = 0; i < count; ++i) {
        if (i) strcat(out, ", ");
        strcat(out, keys[i]);
    }
    for (size_t i = 0; i < count; ++i) free(keys[i]);
    free(keys);
    return out;
}
