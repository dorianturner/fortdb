#include <pthread.h>
#include "field.h"
#include "hash.h"
#include "collection.h"
#include "document.h"
#include "common.h"

Document document_create(void) {
    Document doc = malloc(sizeof(struct Document));
    if (!doc) return NULL;

    pthread_rwlock_init(&doc->lock, NULL);
    if (!doc->lock) {
        free(doc);
        return NULL;
    };

    doc->fields = hashmap_create(DEFAULT_BUCKET_COUNT);
    if (!doc->fields) {
        pthread_rwlock_destroy(&doc->lock);
        free(doc);
        return NULL;
    }

    doc->subcollections = hashmap_create(DEFAULT_BUCKET_COUNT);
    if (!doc->subcollections) {
        hashmap_free(doc->fields, NULL);
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
