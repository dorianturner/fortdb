#define _GNU_SOURCE
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "field.h"
#include "hash.h"
#include "document.h"
#include "common.h"

#define DEFAULT_BUCKET_COUNT 16

Document document_create(void) {
    Document doc = malloc(sizeof(struct Document));
    if (!doc) return NULL;

    if (pthread_rwlock_init(&doc->lock, NULL) != 0) {
        free(doc);
        return NULL;
    }

    doc->fields = hashmap_create(DEFAULT_BUCKET_COUNT);
    if (!doc->fields) {
        pthread_rwlock_destroy(&doc->lock);
        free(doc);
        return NULL;
    }

    doc->subdocuments = hashmap_create(DEFAULT_BUCKET_COUNT);
    if (!doc->subdocuments) {
        hashmap_free(doc->fields);
        pthread_rwlock_destroy(&doc->lock);
        free(doc);
        return NULL;
    }

    return doc;
}

void document_free(Document doc) {
    if (!doc) return;
    hashmap_free(doc->fields);
    hashmap_free(doc->subdocuments);  // recursively frees subdocuments
    pthread_rwlock_destroy(&doc->lock);
    free(doc);
}

// Field getters/setters
Field document_get_field(Document doc, const char *key, uint64_t local_version) {
    if (!doc || !key) return NULL;
    if (pthread_rwlock_rdlock(&doc->lock) != 0) return NULL;
    Field field = hashmap_get(doc->fields, key, local_version);
    pthread_rwlock_unlock(&doc->lock);
    return field;
}

int document_set_field(Document doc, const char *key, Field field, uint64_t global_version) {
    if (!doc || !key || !field) return -1;
    if (pthread_rwlock_wrlock(&doc->lock) != 0) return -1;
    int rc = hashmap_put(doc->fields, key, field, global_version, (void (*)(void *))field_free);
    pthread_rwlock_unlock(&doc->lock);
    return rc;
}

// Convenience: set a C-string directly (allocates and wraps into a Field)
int document_set_field_cstr(Document doc, const char *key, const char *value, uint64_t global_version) {
    if (!doc || !key || !value) return -1;

    Field f = field_create(key);
    if (!f) return -1;

    char *dup = strdup(value);
    if (!dup) {
        field_free(f);
        return -1;
    }

    if (field_set(f, dup, global_version, free) != 0) {
        free(dup);
        field_free(f);
        return -1;
    }

    if (pthread_rwlock_wrlock(&doc->lock) != 0) {
        field_free(f);
        return -1;
    }
    int rc = hashmap_put(doc->fields, key, f, global_version, (void (*)(void *))field_free);
    pthread_rwlock_unlock(&doc->lock);
    if (rc != 0) {
        /* hashmap_put failed, field_free already registered as free callback would not be called */
        field_free(f);
        return -1;
    }
    return 0;
}

// Subdocument getters/setters
Document document_get_subdocument(Document doc, const char *key, uint64_t local_version) {
    if (!doc || !key) return NULL;
    if (pthread_rwlock_rdlock(&doc->lock) != 0) return NULL;
    Document subdoc = hashmap_get(doc->subdocuments, key, local_version);
    pthread_rwlock_unlock(&doc->lock);
    return subdoc;
}

int document_set_subdocument(Document doc, const char *key, Document subdoc, uint64_t global_version) {
    if (!doc || !key || !subdoc) return -1;
    if (pthread_rwlock_wrlock(&doc->lock) != 0) return -1;
    int rc = hashmap_put(doc->subdocuments, key, subdoc, global_version, (void (*)(void *))document_free);
    pthread_rwlock_unlock(&doc->lock);
    return rc;
}

/* ---------- Path / admin helpers (minimal implementations) ---------- */

/* For now we treat `path` as a top-level key only (no dotted paths). */
int document_delete_path(Document doc, const char *path, uint64_t global_version) {
    if (!doc || !path) return -1;
    if (pthread_rwlock_rdlock(&doc->lock) != 0) return -1;
    Field f = hashmap_get(doc->fields, path, /*local_version*/ 0);
    pthread_rwlock_unlock(&doc->lock);
    if (!f) {
        /* nothing to delete */
        return 0;
    }
    /* Add a deleted tombstone version to the field */
    return field_delete(f, global_version);
}

/* List versions stub: you can expand to iterate VersionNode chain */
int document_list_versions(Document doc, const char *path) {
    (void)doc; (void)path;
    fprintf(stderr, "document_list_versions: not implemented (stub)\n");
    return 0;
}

/* Compact stub: no-op for now */
int document_compact(Document doc, const char *path) {
    (void)doc; (void)path;
    /* Implement compaction of version chains if desired. */
    return 0;
}

/* load/save stubs. they should (in the future) load/save subtree at `path`. */
int document_load(Document doc, const char *path) {
    (void)doc; (void)path;
    fprintf(stderr, "document_load: not implemented (stub)\n");
    return 0;
}

int document_save(Document doc, const char *filename, const char *path) {
    (void)doc; (void)filename; (void)path;
    fprintf(stderr, "document_save: not implemented (stub)\n");
    return 0;
}

