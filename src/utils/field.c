#define _GNU_SOURCE
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "version_node.h"
#include "field.h"

char deleted_marker;

Field field_create(const char *name) {
    Field field = malloc(sizeof(struct Field));
    if (!field) return NULL;

    if (pthread_rwlock_init(&field->lock, NULL) != 0) {
        free(field);
        return NULL;
    }

    field->name = strdup(name);
    if (!field->name) {
        pthread_rwlock_destroy(&field->lock);
        free(field);
        return NULL;
    }

    field->versions = NULL;
    return field;
}

void field_free(Field field) {
    if (!field) return;
    version_node_free(field->versions);
    pthread_rwlock_destroy(&field->lock);
    free(field->name);
    free(field);
}

int field_set(Field field, void *value, uint64_t global_version, void (*free_value)(void *)) {
    if (field == NULL) return -1;

    if (pthread_rwlock_wrlock(&field->lock) != 0) {
        return -1;
    }

    uint64_t local_version = 0;
    if (field->versions != NULL) {
        local_version = field->versions->local_version + 1;
    }

    VersionNode prev = field->versions;
    VersionNode head = version_node_create(value, global_version, local_version, prev, free_value);
    if (!head) {
        pthread_rwlock_unlock(&field->lock);
        return -1;
    }

    field->versions = head;

    pthread_rwlock_unlock(&field->lock);
    return 0;
}

void *field_get(Field field, uint64_t local_version) {
    if (field == NULL) return NULL;

    if (pthread_rwlock_rdlock(&field->lock) != 0) {
        return NULL;
    }

    VersionNode curr = field->versions;
    while (curr != NULL && curr->local_version >= local_version) {
        if (curr->local_version == local_version) {
            void *val = curr->value;
            pthread_rwlock_unlock(&field->lock);
            if (val == DELETED) return NULL;
            return val;
        }
        curr = curr->prev;
    }

    pthread_rwlock_unlock(&field->lock);
    return NULL;
}

int field_delete(Field field, uint64_t global_version) {
    return field_set(field, DELETED, global_version, NULL);
}
