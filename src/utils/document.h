#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <pthread.h>
#include <stdint.h>
#include "field.h"
#include "hash.h"

typedef struct Document *Document;

struct Document {
    pthread_rwlock_t lock;
    Hashmap fields;          // char* → Entry(VersionNode(Field))
    Hashmap subdocuments;    // char* → Entry(VersionNode(Document))
};

// Memory management
Document document_create(void);
void document_free(Document doc);

// Field getters/setters
Field document_get_field(Document doc, const char *key, uint64_t local_version);
int document_set_field(Document doc, const char *key, Field field, uint64_t global_version);

// Subdocument getters/setters
Document document_get_subdocument(Document doc, const char *key, uint64_t local_version);
int document_set_subdocument(Document doc, const char *key, Document subdoc, uint64_t global_version);

#endif

