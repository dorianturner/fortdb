#ifndef TABLE_H
#define TABLE_H

#include <pthread.h>
#include <stdint.h>
#include <string.h>
#include "hash.h"
#include "collection.h"

typedef struct Table *Table;

struct Table {
    pthread_rwlock_t lock;
    Hashmap collections;
};

Table table_create(void);
void table_free(Table table);

int table_set_field(Table table, const char *path, const char *value, uint64_t global_version);
void *table_get_field(Table table, const char *path, uint64_t version);
int table_delete_path(Table table, const char *path, uint64_t global_version);
int table_list_versions(Table table, const char *path);
int table_compact(Table table, const char *path);
int table_load(Table table, const char *filepath);
int table_save(Table table, const char *filename, const char *path);

#endif

