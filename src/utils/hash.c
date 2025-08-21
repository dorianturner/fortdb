#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include "hash.h"
#include "version_node.h"

// FNV-1a hashing, see wikipedia
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

    map->datapoints = 0;
    map->bucket_count = bucket_count;
    map->size = 0;
    map->buckets = calloc(bucket_count, sizeof(Entry));
    if (!map->buckets) {
        free(map);
        return NULL;
    }

    return map;
}

//why is this still called entry??

// PRE: Entry values are always VersionNodes
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

// PRE: Entrys contain version nodes
int hashmap_put(Hashmap map, const char *key, void *value, uint64_t global_version, void (free_value)(void *)) {
    if (!map || !key || !value) return -1;

    /* If inserting one more would exceed load factor, try to grow first */
    if ((double)(map->size + 1) / (double)map->bucket_count > LOAD_FACTOR) {
        /* ignore rehash failure and continue */
        (void)hashmap_rehash(map, map->bucket_count * 2);
    }

    uint64_t index = hash(key) % map->bucket_count;
    Entry current = map->buckets[index];

    /* If key exists, prepend a new VersionNode to its chain */
    while (current) {
        if (strcmp(current->key, key) == 0) {
            VersionNode old_head = (VersionNode)current->value;
            uint64_t local_version = old_head ? old_head->local_version + 1 : 1;
            VersionNode new_head = version_node_create(value, global_version, local_version, old_head, free_value);
            if (!new_head) return -1;
            current->value = new_head;

            //for serialization purposes
            map->datapoints++;
            if(map->head){
                map->tail->prev = new_head;
                map->tail = new_head;
            }else{
                map->head = new_head;
                map->tail = new_head;
            }
            

            return 0;
        }
        current = current->next;
    }

    /* Key not found: create new entry with local_version = 1 */
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

    /* Recompute index in case rehash changed bucket_count earlier */
    index = hash(key) % map->bucket_count;
    new_entry->value = new_head;
    new_entry->next = map->buckets[index];
    map->buckets[index] = new_entry;
    map->size++;

    //for serialization purposes
    map->datapoints++;
    if(map->head){
        map->tail->prev = new_head;
        map->tail = new_head;
    }else{
        map->head = new_head;
        map->tail = new_head;
    }
    
    return 0;
}

// POST: We have put an Entry(VersionNode(value)) to the right bucket

// A bit inefficient for now ig but how to make faster idk (maybe iterators)
void *hashmap_get(Hashmap map, const char *key, uint64_t local_version) {
    if (!map || !key) return NULL;

    uint64_t index = hash(key) % map->bucket_count;
    Entry current = map->buckets[index];

    while (current) {
        if (strcmp(current->key, key) == 0) {
            VersionNode head = (VersionNode)current->value;

            /* Treat local_version == 0 as "latest" for backward compatibility. */
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


