#ifndef TABLE_H
#define TABLE_H

#include <pthread.h>
#include "hash.h"
#include "collection.h"

typedef struct Table *Table;

// Just have a root table for now
struct Table {
    pthread_rwlock_t lock;
    HashMap collections;     // char* → Entry(VersionNode(Collection))
};

// Memory management
Table table_create(void);
void table_free(Table table);

// Collection getters/setters
Collection table_get_collection(Table table, const char *key, uint64_t local_version);
int table_set_collection(Table table, const char *key, Collection coll, uint64_t global_version);

#endif

