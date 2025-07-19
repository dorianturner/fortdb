#ifndef DOCUMENT_H
#define DOCUMENT_H


#define _GNU_SOURCE
#include <stdint.h>
#include <pthread.h>
#include "field.h"
#include "hash.h"
#include "collection.h"

struct Collection;
typedef struct Collection *Collection;

typedef struct Document *Document;

struct Document {
    pthread_rwlock_t lock;
    Hashmap fields;          // char* → Entry(VersionNode(Field))
    Hashmap subcollections;  // char* → Entry(VersionNode(Collection))
};

// Memory management
Document document_create(void);
void document_free(Document doc);

// Field getters/setters
Field document_get_field(Document doc, const char *key, uint64_t local_version);
int document_set_field(Document doc, const char *key, Field field, uint64_t global_version);

// Subcollection getters/setters
Collection document_get_subcollection(Document doc, const char *key, uint64_t local_version);
int document_set_subcollection(Document doc, const char *key, Collection subcoll, uint64_t global_version);

#endif

