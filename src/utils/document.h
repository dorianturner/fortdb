#ifndef DOCUMENT_H
#define DOCUMENT_H

#if defined(_WIN32)
#include "windows_compat.h"
#else
#include <pthread.h>
#endif
#include <stdint.h>
#include "hash.h"

#ifndef DELETED
#define DELETED ((void*)1)
#endif

typedef struct Document *Document;

struct Document {
    pthread_rwlock_t lock;
    Hashmap fields;          // char* → Entry(VersionNode(char*))
    Hashmap subdocuments;    // char* → Entry(VersionNode(Document))
};

// Memory management
Document document_create(void);
void document_free(Document doc);

// Field getters/setters 
// For convenience, we only set strings as our values
char *document_get_field(Document doc, const char *key, uint64_t local_version);
int document_set_field(Document doc, const char *key, const char *value, uint64_t global_version);
int document_set_field_cstr(Document doc, const char *key, const char *value, uint64_t global_version);
int document_set_field_path(Document root, const char *path, const char *value, uint64_t global_version);

// Subdocument getters/setters
Document document_get_subdocument(Document doc, const char *key, uint64_t local_version);
int document_set_subdocument(Document doc, const char *key, Document subdoc, uint64_t global_version);

// Path ops
int document_delete_path(Document doc, const char *path, uint64_t global_version);
int document_list_versions(Document doc, const char *path);

// Stubs rn
int document_compact(Document doc, const char *path);
int document_load(Document doc, const char *path);
int document_save(Document doc, const char *filename, const char *path);

#endif

