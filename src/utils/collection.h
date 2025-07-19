#ifndef COLLECTION_H
#define COLLECTION_H

#include <pthread.h>
#include "hash.h"
#include "document.h"

struct Collection;
typedef struct Collection *Collection;

struct Collection {
    pthread_rwlock_t lock;
    Hashmap documents;       // char* → Entry(VersionNode(Document))
};

// Memory management
Collection collection_create(void);
void collection_free(Collection coll);

// Document getters/setters
Document collection_get_document(Collection coll, const char *key, uint64_t local_version);
int collection_set_document(Collection coll, const char *key, Document doc, uint64_t global_version);

#endif

