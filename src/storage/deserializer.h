#ifndef DESERIALIZER_H
#define DESERIALIZER_H

#include <stdint.h>
#include <stdio.h>

// Forward declarations (same as in serializer.h)
typedef struct Document* Document;
typedef struct VersionNode VersionNode;

/* Error codes (mirror serializer for consistency) */
#define ERR_OK      0
#define ERR_READ    1
#define ERR_INVALID 2
#define ERR_MEM     3
#define ERR_LOCK    4

/* Read a single field (path, type, versions) from a file */
int read_field_entry(FILE* f, char** out_path, uint32_t* out_type, VersionNode** out_versions);

/* Recursively deserialize a document and its subdocuments */
int deserialize_document(Document doc, const char* path_prefix, FILE* f);

/* Deserialize an entire DB from a file */
int deserialize_db(Document* out_root, const char* filename);

#endif /* DESERIALIZER_H */
