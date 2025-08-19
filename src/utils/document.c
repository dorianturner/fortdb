#define _GNU_SOURCE
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
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
    hashmap_put(doc->fields, key, field, global_version, (void (*)(void *))field_free);
    pthread_rwlock_unlock(&doc->lock);
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
    if (hashmap_put(doc->subdocuments, key, subdoc, global_version, (void (*)(void *))document_free) != 0)
        return -1;
    pthread_rwlock_unlock(&doc->lock);
    return 0;
}

