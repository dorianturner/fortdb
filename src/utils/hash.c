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

    uint64_t index = hash(key) % map->bucket_count;
    Entry current = map->buckets[index];

    while (current) {
        if (strcmp(current->key, key) == 0) {
            VersionNode old_head = (VersionNode)current->value;
            uint64_t local_version = old_head ? old_head->local_version + 1 : 0;
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
    
    // MID: Entry with key doesn't exist
    VersionNode new_head = version_node_create(value, global_version, 0, NULL, free_value);
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

