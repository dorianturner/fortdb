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

// Field getters/setters (field-based API)
// NOTE: `key` may be a '/'-separated path. If it is, the function will
// traverse subdocuments to reach the parent of the final component.
Field document_get_field(Document doc, const char *key, uint64_t local_version);
int document_set_field(Document doc, const char *key, Field field, uint64_t global_version);

// Convenience: set a C string value directly (creates a Field internally)
int document_set_field_cstr(Document doc, const char *key, const char *value, uint64_t global_version);

// Path-aware setter: will create any missing intermediate subdocuments
int document_set_field_path(Document root, const char *path, const char *value, uint64_t global_version);

// Subdocument getters/setters
Document document_get_subdocument(Document doc, const char *key, uint64_t local_version);
int document_set_subdocument(Document doc, const char *key, Document subdoc, uint64_t global_version);

// Path / admin ops
// - document_delete_path: deletes a field at the given path (adds tombstone). Deleting an entire
//   subdocument subtree is not implemented here (will return success if nothing to delete).
int document_delete_path(Document doc, const char *path, uint64_t global_version);

// - List versions for a field at `path`; prints to stdout. Simple implementation that prints
//   each version's local id and the string value (or "<deleted>" if tombstone).
int document_list_versions(Document doc, const char *path);

// - Compact / load / save: minimal implementations / stubs that return 0 (success)
//   and print a short message. You can extend these to do real I/O / compaction.
int document_compact(Document doc, const char *path);
int document_load(Document doc, const char *path);
int document_save(Document doc, const char *filename, const char *path);

#endif

