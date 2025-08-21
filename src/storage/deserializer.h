#ifndef DOCUMENT_DESERIALIZE_H
#define DOCUMENT_DESERIALIZE_H

#include <stdio.h>
#include <stdint.h>

// Error codes
#define ERR_OK          0
#define ERR_MEM         1
#define ERR_LOCK        2
#define ERR_INVALID     3
#define ERR_WRITE       4

// Forward declarations
struct VersionNode;
struct Document;
typedef struct VersionNode* VersionNode;
typedef struct Document* Document;
typedef struct Hashmap* Hashmap;

/*
 * Deserialize a single field's VersionNode chain from a file.
 * Allocates memory for values. Caller must free via version_node_free.
 */
int deserialize_field(FILE* f, VersionNode* out_versions);

/*
 * Deserialize a hashmap from a file.
 * - map: the hashmap to populate
 * - is_field: 1 for fields, 0 for subdocuments
 */
int deserialize_hashmap(FILE* f, Hashmap map, int is_field);

/*
 * Deserialize an entire document from a file.
 * Populates the fields and subdocuments hashmaps.
 */
int deserialize_document(FILE* f, Document doc);

#endif