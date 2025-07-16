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

// Field getters/setters
Field document_get_field(Document doc, const char *key, uint64_t local_version) {
   if (!doc || !key) return NULL;
   Field field = hashmap_get(doc->fields, key, local_version);
   return field;
}

// Putting a Version_Node(Field) into the hashmap
int document_set_field(Document doc, const char *key, Field field, uint64_t global_version) {
    if (!doc || !key || !field) return -1;
    hashmap_put(doc->fields, key, field, global_version, (*field_free));
    return 0;
}


// Subcollection getters/setters
Collection document_get_subcollection(Document doc, const char *key, uint64_t local_version) {
    if (!doc || !key) return NULL;
    Field subcol = hashmap_get(doc->subcollections, key, local_version);
    return subcol;
}

int document_set_subcollection(Document doc, const char *key, Collection subcoll, uint64_t global_version) {
    if (!doc || !key || !subcoll) return -1;
    Collection subcol = hashmap_put(doc->subcollections, key, subcoll, global_version, (*collection_free));
    return 0;
}
