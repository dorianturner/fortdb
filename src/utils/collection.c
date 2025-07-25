#define _GNU_SOURCE
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include "collection.h"
#include "common.h"
#include "document.h"

#define DEFAULT_BUCKET_COUNT 16

// Memory management
Collection collection_create(void) {
    Collection collection = malloc(sizeof(struct Collection));
    if (!collection) return NULL;

    if (pthread_rwlock_init(&collection->lock, NULL) != 0) {
        free(collection);
        return NULL;
    }


    collection->documents = hashmap_create(DEFAULT_BUCKET_COUNT); 
    if (!collection->documents) {
        pthread_rwlock_destroy(&collection->lock);
        free(collection);
        return NULL;
    }
    
    return collection;
}

void collection_free(Collection coll) {
    if (!coll) return;
    hashmap_free(coll->documents);
    pthread_rwlock_destroy(&coll->lock);
    free(coll);
}

Document collection_get_document(Collection coll, const char *key, uint64_t local_version) {
    if (!coll || !key) return NULL;
    if (pthread_rwlock_rdlock(&coll->lock) != 0) return NULL;
    Document doc = hashmap_get(coll->documents, key, local_version); 
    pthread_rwlock_unlock(&coll->lock);
    return doc;
}

int collection_set_document(Collection coll, const char *key, Document doc, uint64_t global_version) {
    if (!coll || !key || !doc) return -1;
    if (pthread_rwlock_rdlock(&coll->lock) != 0) return -1;
    if (hashmap_put(coll->documents, key, doc, global_version, (void (*)(void *))document_free) != 0) return -1;
    pthread_rwlock_unlock(&coll->lock);
    return 0;
}
