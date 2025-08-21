#ifndef SERIALIZER_H
#define SERIALIZER_H

#include <stdint.h>
#include <stdio.h>

// Forward declarations to avoid pulling in full internals unnecessarily
typedef struct Document* Document;
typedef struct VersionNode* VersionNode;

/* Error codes */
#define ERR_OK      0
#define ERR_WRITE   1
#define ERR_INVALID 2
#define ERR_MEM     3
#define ERR_LOCK    4

/* Write a single field (path, type, versions) into a file */
int write_field_entry(FILE* f, const char* path, uint32_t type, VersionNode* versions);

/* Recursively serialize a document and its subdocuments */
int serialize_document(Document doc, const char *path_prefix, FILE *f);

/* Serialize an entire DB rooted at `root` into a file */
int serialize_db(Document root, const char *filename);

#endif /* SERIALIZER_H */
