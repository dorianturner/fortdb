#ifndef SERIALIZER_H
#define SERIALIZER_H

#include <stdio.h>
#include <stdint.h>

struct VersionNode;
typedef struct VersionNode *VersionNode;

struct Document;
typedef struct Document *Document;

/* Deleted marker pointer */
extern void * const DELETED;

/* Serialize the entire database root (VersionNode containing Document) to file atomically.
 * Returns 0 on success, -1 on failure.
 */
int serialize_db(VersionNode root, const char *filename);

/* Serialize a Document to an open FILE*.
 * Returns 0 on success, -1 on failure.
 */
int serialize_document(Document doc, FILE *file);

/* Serialize a single VersionNode to an open FILE*.
 * Handles string, tombstone, and Document payloads.
 * Returns 0 on success, -1 on failure.
 */
int serialize_version_node(VersionNode ver, FILE *file);

#endif /* SERIALIZER_H */
