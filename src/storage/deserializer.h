#ifndef DESERIALIZER_H
#define DESERIALIZER_H

#include <stdio.h>
#include <stdint.h>

struct VersionNode;
typedef struct VersionNode *VersionNode;

struct Document;
typedef struct Document *Document;

/* Deserialize the entire database from a file into a VersionNode root.
 * Returns 0 on success, -1 on failure.
 * On success, `*root` will point to the reconstructed version chain.
 */
int deserialize_db(const char *filename, VersionNode *root_out);

/* Deserialize a Document from an open FILE*.
 * Returns 0 on success, -1 on failure.
 * Reconstructs fields and subdocuments recursively.
 */
int deserialize_document(Document *doc_out, FILE *file);

/* Deserialize a single VersionNode from an open FILE*.
 * Handles string, tombstone, and Document payloads.
 * Returns 0 on success, -1 on failure.
 */
int deserialize_version_node(VersionNode *ver_out, FILE *file);

#endif /* DESERIALIZER_H */
