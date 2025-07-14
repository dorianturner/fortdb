#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <pthread.h>
#include "field.h"
#include "hash.h"
#include "collection.h"

typedef struct Document *Document;

struct Document {
    pthread_rwlock_t lock;
    HashMap fields;          // char* → VersionNode(Field)
    HashMap subcollections;  // char* → VersionNode(Collection)
};

// Memory management
Document document_create(void);
void document_free(Document doc);

// Field getters/setters
Field document_get_field(Document doc, const char *key, uint64_t local_version);
int document_set_field(Document doc, const char *key, void *value, uint64_t global_version);

// Subcollection getters/setters
Collection document_get_subcollection(Document doc, const char *name, uint64_t local_version);
int document_set_subcollection(Document doc, const char *name, Collection subcoll, uint64_t global_version);

#endif

