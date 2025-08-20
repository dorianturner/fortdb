#include <stddef.h>   // for size_t
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <stddef.h>
#include "windows_compat.h"
#include <winsock2.h>       // htonl, ntohl
#define htobe64(x) _byteswap_uint64(x)
#define be64toh(x) _byteswap_uint64(x)
#else
#include <pthread.h>
#include <arpa/inet.h>      // htonl, ntohl
#include <endian.h>         // htobe64, be64toh
#endif

#include "version_node.h"
#include "collection.h"
#include "document.h"
#include "field.h"
#include "hash.h"

#define ERR_OK      0
#define ERR_WRITE   1
#define ERR_INVALID 2
#define ERR_MEM     3
#define ERR_LOCK    4

#define TYPE_STRING 1

int write_field_entry(FILE* f, const char* path, uint32_t type, VersionNode versions) {
    if (!f || !path) return ERR_INVALID;

    // Write path length
    uint32_t path_len = strlen(path);
    uint32_t path_len_be = htonl(path_len);
    if (fwrite(&path_len_be, sizeof(uint32_t), 1, f) != 1) return ERR_WRITE;

    // Write path string
    if (fwrite(path, sizeof(char), path_len, f) != path_len) return ERR_WRITE;

    // Write field type
    uint32_t type_be = htonl(type);
    if (fwrite(&type_be, sizeof(uint32_t), 1, f) != 1) return ERR_WRITE;

    // Count versions
    int version_count = 0;
    VersionNode current = versions;
    while (current) {
        version_count++;
        current = current->prev;
    }
    uint32_t version_count_be = htonl(version_count);
    if (fwrite(&version_count_be, sizeof(uint32_t), 1, f) != 1) return ERR_WRITE;

    // Write each version
    VersionNode v = versions;
    while (v) {
        uint64_t local_version_be = htobe64(v->local_version);
        if (fwrite(&local_version_be, sizeof(uint64_t), 1, f) != 1) return ERR_WRITE;

        uint64_t value_len = strlen((char*)v->value);
        uint64_t value_len_be = htobe64(value_len);
        if (fwrite(&value_len_be, sizeof(uint64_t), 1, f) != 1) return ERR_WRITE;

        if (fwrite(v->value, 1, value_len, f) != value_len) return ERR_WRITE;

        v = v->prev;
    }

    return ERR_OK;
}

int serialize_document(Document doc, const char *path_prefix, FILE *f) {
    if (!doc || !path_prefix || !f) return ERR_INVALID;

    // Lock the document for reading
    if (pthread_rwlock_rdlock(&doc->lock) != 0) return ERR_LOCK;

    // 1. Serialize all fields in this document
    for (uint64_t i = 0; i < doc->fields->bucket_count; i++) {
        Entry entry = doc->fields->buckets[i];
        while (entry) {

            struct Field *field = (struct Field *)(((struct VersionNode *)entry->value)->value);

            // Build full path: path_prefix + "/" + field_name
            int64_t path_len = strlen(path_prefix) + strlen(field->name) + (strlen(path_prefix) ? 2 : 1);

            char *path = malloc(path_len);
            
            if (!path) {
                pthread_rwlock_unlock(&doc->lock);
                return ERR_MEM;
            }

            // Avoiding starting "/" if path_prefix is empty
            if (strlen(path_prefix) == 0) {
                snprintf(path, path_len, "%s", field->name);
            } else {
                snprintf(path, path_len, "%s/%s", path_prefix, field->name);
            }


            // Write the field entry
            int res = write_field_entry(f, path, TYPE_STRING, field->versions);
            free(path);
            if (res != ERR_OK) {
                pthread_rwlock_unlock(&doc->lock);
                return res;
            }

            entry = entry->next;
        }
    }

    // 2. Recursively serialize all subdocuments
    for (uint64_t i = 0; i < doc->subdocuments->bucket_count; i++) {
        Entry entry = doc->subdocuments->buckets[i];
        while (entry) {
            Document subdoc = (Document)((VersionNode)entry->value)->value;

            // Build new path prefix: path_prefix + "/" + subdocument name
            uint64_t path_len = strlen(path_prefix) + strlen(entry->key) + 2;
            char *sub_path = malloc(path_len);
            if (!sub_path) {
                pthread_rwlock_unlock(&doc->lock);
                return ERR_MEM;
            }
            snprintf(sub_path, path_len, "%s/%s", path_prefix, entry->key);

            int res = serialize_document(subdoc, sub_path, f);
            free(sub_path);
            if (res != ERR_OK) {
                pthread_rwlock_unlock(&doc->lock);
                return res;
            }

            entry = entry->next;
        }
    }

    pthread_rwlock_unlock(&doc->lock);
    return ERR_OK;
}

int serialize_db(Document root, const char *filename) {
    if (!root || !filename) return ERR_INVALID;

    FILE *f = fopen(filename, "wb");
    if (!f) {
        perror("Failed to open file for writing");
        return ERR_WRITE;
    }

    // Write optional DB metadata (e.g., version)
    uint32_t db_version = 1;
    uint32_t db_version_be = htonl(db_version);
    fwrite(&db_version_be, sizeof(uint32_t), 1, f);

    // Start serialization from the root document
    int res = serialize_document(root, "", f);

    fclose(f);
    return res;
}


