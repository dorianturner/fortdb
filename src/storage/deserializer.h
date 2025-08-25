#ifndef DESERIALIZER_H
#define DESERIALIZER_H

#include <stdint.h>
#include <stdio.h>
#include "version_node.h"
#include "document.h"

/**
 * Deserialize an entire database from a file.
 * 
 * @param filename Path to the serialized file.
 * @param root_out Pointer to a VersionNode* that will be overwritten with the root chain.
 * @return 0 on success, -1 on failure.
 */
int deserialize_db(const char *filename, VersionNode *root_out);

/**
 * Deserialize a single VersionNode from a file.
 * 
 * @param ver_out Pointer to a VersionNode* that will be allocated and returned.
 * @param file Open FILE* to read from.
 * @return 0 on success, -1 on failure.
 */
int deserialize_version_node(VersionNode *ver_out, FILE *file);

/**
 * Deserialize a Document (with its fields and subdocuments) from a file.
 * 
 * @param doc_out Pointer to a Document* that will be allocated and returned.
 * @param file Open FILE* to read from.
 * @return 0 on success, -1 on failure.
 */
int deserialize_document(Document *doc_out, FILE *file);

#endif /* DESERIALIZER_H */
