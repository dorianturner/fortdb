#ifndef FIELD_H
#define FIELD_H

#include <stdint.h>
#include <pthread.h>
#include "version_node.h"

typedef struct Field *Field;

struct Field {
    pthread_rwlock_t lock;
    char *name;
    VersionNode versions;
};

// Memory management
Field field_create(const char *name);
void field_free(Field field, void (free_node_value)(void *));

// Getters/setters
int field_set(Field field, void *value, uint64_t global_version);
void *field_get(Field field, uint64_t local_version);

#endif

