#include <stddef.h>   // for size_t
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)

#include <winsock2.h>       // htonl, ntohl
#include "windows_compat.h"

#define htobe64(x) _byteswap_uint64(x)
#define be64toh(x) _byteswap_uint64(x)
#else
#include <pthread.h>
#include <arpa/inet.h>      // htonl, ntohl
#include <endian.h>         // htobe64, be64toh
#endif

#include "deserializer.h"
#include "version_node.h"
#include "document.h"
#include "field.h"
#include "hash.h"

#define ERR_OK      0
#define ERR_WRITE   1
#define ERR_INVALID 2
#define ERR_MEM     3
#define ERR_LOCK    4

#define TYPE_STRING 1

/*
 * Conceptual overview:
 * 
 * A Document contains two hashmaps:
 *   1. fields      -> entries holding VersionNode chains of data values
 *   2. subdocuments -> entries holding VersionNode chains of nested Document objects
 *
 * Each hashmap is an array of buckets; each bucket is a linked list of Entry structs
 * to handle hash collisions. Each Entry points to a key and a VersionNode value.
 *
 * The deserializer:
 *   - Reads serialized field/subdocument data from a file
 *   - Reconstructs the VersionNode chains
 *   - Handles collisions by chaining Entry structs in buckets
 *   - Recursively reconstructs nested Documents
 */

int deserialize_field(FILE* f, VersionNode* out_versions) {
    if (!f || !out_versions) return ERR_INVALID;

    uint32_t path_len_be;
    if (fread(&path_len_be, sizeof(uint32_t), 1, f) != 1) return ERR_WRITE;
    uint32_t path_len = ntohl(path_len_be);

    char* path = malloc(path_len + 1);
    if (!path) return ERR_MEM;
    if (fread(path, 1, path_len, f) != path_len) {
        free(path);
        return ERR_WRITE;
    }
    path[path_len] = '\0'; // null-terminate

    uint32_t type_be;
    if (fread(&type_be, sizeof(uint32_t), 1, f) != 1) {
        free(path);
        return ERR_WRITE;
    }
    uint32_t type = ntohl(type_be);

    uint32_t version_count_be;
    if (fread(&version_count_be, sizeof(uint32_t), 1, f) != 1) {
        free(path);
        return ERR_WRITE;
    }
    uint32_t version_count = ntohl(version_count_be);

    VersionNode head = NULL;
    for (uint32_t i = 0; i < version_count; i++) {
        uint64_t local_version_be;
        if (fread(&local_version_be, sizeof(uint64_t), 1, f) != 1) return ERR_WRITE;
        uint64_t local_version = be64toh(local_version_be);

        uint64_t value_len_be;
        if (fread(&value_len_be, sizeof(uint64_t), 1, f) != 1) return ERR_WRITE;
        uint64_t value_len = be64toh(value_len_be);

        void* value = malloc(value_len + 1);
        if (!value) return ERR_MEM;
        if (fread(value, 1, value_len, f) != value_len) {
            free(value);
            return ERR_WRITE;
        }
        ((char*)value)[value_len] = '\0'; // null-terminate

        head = version_node_create(value, 0, local_version, head, free);
    }

    *out_versions = head;
    free(path);
    return ERR_OK;
}

int deserialize_hashmap(FILE* f, Hashmap map, int is_field) {
    if (!f || !map) return ERR_INVALID;

    for (uint64_t i = 0; i < map->bucket_count; i++) {
        uint32_t entry_count_be;
        if (fread(&entry_count_be, sizeof(uint32_t), 1, f) != 1) return ERR_WRITE;
        uint32_t entry_count = ntohl(entry_count_be);

        for (uint32_t e = 0; e < entry_count; e++) {
            uint32_t key_len_be;
            if (fread(&key_len_be, sizeof(uint32_t), 1, f) != 1) return ERR_WRITE;
            uint32_t key_len = ntohl(key_len_be);

            char* key = malloc(key_len + 1);
            if (!key) return ERR_MEM;
            if (fread(key, 1, key_len, f) != key_len) {
                free(key);
                return ERR_WRITE;
            }
            key[key_len] = '\0';

            VersionNode value_head = NULL;
            int res;
            if (is_field) {
                res = deserialize_field(f, &value_head);
            } else {
                Document subdoc = document_create();
                res = deserialize_document(f, subdoc);
                value_head = version_node_create(subdoc, 0, 0, NULL, (void (*)(void *))document_free);
            }

            if (res != ERR_OK) {
                free(key);
                return res;
            }

            hashmap_put(map, key, value_head, 0, is_field ? free : (void (*)(void *))document_free);
            free(key);
        }
    }

    return ERR_OK;
}

int deserialize_document(FILE* f, Document doc) {
    if (!f || !doc) return ERR_INVALID;

    if (pthread_rwlock_wrlock(&doc->lock) != 0) return ERR_LOCK;

    int res = deserialize_hashmap(f, doc->fields, 1);
    if (res != ERR_OK) {
        pthread_rwlock_unlock(&doc->lock);
        return res;
    }

    res = deserialize_hashmap(f, doc->subdocuments, 0);
    if (res != ERR_OK) {
        pthread_rwlock_unlock(&doc->lock);
        return res;
    }

    pthread_rwlock_unlock(&doc->lock);
    return ERR_OK;
}
