#define _GNU_SOURCE
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include "field.h"
#include "hash.h"
#include "collection.h"
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

    doc->subcollections = hashmap_create(DEFAULT_BUCKET_COUNT);
    if (!doc->subcollections) {
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
    hashmap_free(doc->subcollections);
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

// Putting a Version_Node(Field) into the hashmap
int document_set_field(Document doc, const char *key, Field field, uint64_t global_version) {
    if (!doc || !key || !field) return -1;
    if (pthread_rwlock_wrlock(&doc->lock) != 0) return -1;
    hashmap_put(doc->fields, key, field, global_version, (void (*)(void *))field_free);
    pthread_rwlock_unlock(&doc->lock);
    return 0;
}

// Subcollection getters/setters
Collection document_get_subcollection(Document doc, const char *key, uint64_t local_version) {
    if (!doc || !key) return NULL;
    if (pthread_rwlock_wrlock(&doc->lock) != 0) return NULL;
    Collection subcol = hashmap_get(doc->subcollections, key, local_version);
    pthread_rwlock_unlock(&doc->lock);
    return subcol;
}

int document_set_subcollection(Document doc, const char *key, Collection subcoll, uint64_t global_version) {
    if (!doc || !key || !subcoll) return -1;
    if (pthread_rwlock_wrlock(&doc->lock) != 0) return -1;
    if (hashmap_put(doc->subcollections, key, subcoll, global_version, (void (*)(void *))collection_free) != 0) return -1;
    pthread_rwlock_unlock(&doc->lock);
    return 0;
}
