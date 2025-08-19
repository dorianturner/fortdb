#ifndef FIELD_H
#define FIELD_H

#include <pthread.h>
#include <stdint.h>
#include "version_node.h"

extern char deleted_marker;
#define DELETED ((void *)&deleted_marker)

typedef struct Field *Field;

struct Field {
    pthread_rwlock_t lock;
    char *name;
    VersionNode versions;
};

// Memory management
Field field_create(const char *name);
void field_free(Field field);

// Getters/setters
int field_set(Field field, void *value, uint64_t global_version, void (*free_value)(void *));
int field_delete(Field field, uint64_t global_version);
void *field_get(Field field, uint64_t local_version);

#endif

