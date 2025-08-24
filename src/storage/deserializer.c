#include <endian.h>    // be64toh, htobe64
/* portable fallbacks if not provided by system headers */
#ifndef htobe64
#define htobe64(x) (__builtin_bswap64((uint64_t)(x)))
#endif
#ifndef be64toh
#define be64toh(x) (__builtin_bswap64((uint64_t)(x)))
#endif
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*#if defined(_WIN32)
#include <winsock2.h>
#include "windows_compat.h"
#define be64toh(x) _byteswap_uint64(x)
#define htobe64(x) _byteswap_uint64(x)
#else

#endif*/

#include <arpa/inet.h> // ntohl, htonl

#include <pthread.h>
#include <unistd.h>    // fsync

#include "deserializer.h"
#include "serializer.h"
#include "version_node.h"
#include "document.h"
#include "hash.h"



#define MAGIC "DBV1"
static const uint32_t FORMAT_VER = 1;


int deserialize_db(const char *filename, VersionNode *root_out) {
    if (!filename || !root_out) return -1;

    FILE *f = fopen(filename, "rb");
    if (!f) return -1;

    char magic[4];
    uint32_t be32;

    // 1. Read and verify magic
    if (fread(magic, 1, 4, f) != 4) goto fail;
    if (memcmp(magic, MAGIC, 4) != 0) goto fail;

    // 2. Read and verify format version
    if (fread(&be32, sizeof(be32), 1, f) != 1) goto fail;
    if (ntohl(be32) != FORMAT_VER) goto fail;

    // 3. Skip reserved flags
    if (fread(&be32, sizeof(be32), 1, f) != 1) goto fail;

    // 4. Read count of root versions
    uint64_t be64;
    if (fread(&be64, sizeof(be64), 1, f) != 1) goto fail;
    uint64_t ver_count = be64toh(be64);

    VersionNode head = NULL, tail = NULL;
    for (uint64_t i = 0; i < ver_count; i++) {
        VersionNode ver = NULL;
        if (deserialize_version_node(&ver, f) != 0) goto fail;

        ver->prev = NULL;
        if (!head) {
            head = tail = ver;
        } else {
            tail->prev = ver;
            tail = ver;
        }
    }

    fclose(f);
    *root_out = head;
    return 0;

    fail:
        if (f) fclose(f);
        *root_out = NULL;
        return -1;
}


/* Fixed: build chains so head = latest, head->prev -> older -> ... -> NULL */
int deserialize_document(Document *doc_out, FILE *file) {
    if (!doc_out || !file) return -1;

    Document doc = document_create();
    if (!doc) return -1;

    uint64_t be64, field_count;

    /* 1. Fields */
    if (fread(&be64, sizeof(be64), 1, file) != 1) goto fail;
    field_count = be64toh(be64);

    for (uint64_t i = 0; i < field_count; i++) {
        if (fread(&be64, sizeof(be64), 1, file) != 1) goto fail;
        uint64_t key_len = be64toh(be64);

        char *key = malloc(key_len + 1);
        if (!key) goto fail;
        if (key_len && fread(key, 1, (size_t)key_len, file) != (size_t)key_len) { free(key); goto fail; }
        key[key_len] = '\0';

        /* number of versions */
        if (fread(&be64, sizeof(be64), 1, file) != 1) { free(key); goto fail; }
        uint64_t ver_count = be64toh(be64);

        VersionNode head = NULL;
        VersionNode tail = NULL; /* tail points to the most-recently-added node so we can append older nodes */

        for (uint64_t v = 0; v < ver_count; v++) {
            VersionNode ver = NULL;
            if (deserialize_version_node(&ver, file) != 0) { free(key); goto fail; }

            ver->prev = NULL; /* will be set by appending logic */

            if (!head) {
                head = tail = ver; /* first node read is the latest */
            } else {
                /* append older node after tail */
                tail->prev = ver;
                tail = ver;
            }
        }

        if (hashmap_set_raw(doc->fields, key, head) != 0) { free(key); goto fail; }
        free(key);
    }

    /* 2. Subdocuments */
    if (fread(&be64, sizeof(be64), 1, file) != 1) goto fail;
    uint64_t subdoc_count = be64toh(be64);

    for (uint64_t i = 0; i < subdoc_count; i++) {
        if (fread(&be64, sizeof(be64), 1, file) != 1) goto fail;
        uint64_t key_len = be64toh(be64);

        char *key = malloc(key_len + 1);
        if (!key) goto fail;
        if (key_len && fread(key, 1, (size_t)key_len, file) != (size_t)key_len) { free(key); goto fail; }
        key[key_len] = '\0';

        if (fread(&be64, sizeof(be64), 1, file) != 1) { free(key); goto fail; }
        uint64_t ver_count = be64toh(be64);

        VersionNode head = NULL;
        VersionNode tail = NULL;
        for (uint64_t v = 0; v < ver_count; v++) {
            VersionNode ver = NULL;
            if (deserialize_version_node(&ver, file) != 0) { free(key); goto fail; }
            ver->prev = NULL;
            if (!head) head = tail = ver;
            else { tail->prev = ver; tail = ver; }
        }

        if (hashmap_set_raw(doc->subdocuments, key, head) != 0) { free(key); goto fail; }
        free(key);
    }

    *doc_out = doc;
    return 0;

    fail:
    if (doc) document_free(doc);
    return -1;
}


/* Fixed: set free_value for subdocuments (document_free) */
int deserialize_version_node(VersionNode *ver_out, FILE *file) {
    if (!ver_out || !file) return -1;

    uint64_t be64;
    uint8_t type;

    /* global_version */
    if (fread(&be64, sizeof(be64), 1, file) != 1) return -1;
    uint64_t global_version = be64toh(be64);

    /* local_version */
    if (fread(&be64, sizeof(be64), 1, file) != 1) return -1;
    uint64_t local_version = be64toh(be64);

    /* type */
    if (fread(&type, sizeof(type), 1, file) != 1) return -1;

    void *value = NULL;
    void (*free_value)(void *) = NULL;

    switch (type) {
        case 0: /* tombstone */
            value = DELETED;
            free_value = NULL;
            break;

        case 1: { /* string */
            if (fread(&be64, sizeof(be64), 1, file) != 1) return -1;
            uint64_t len = be64toh(be64);
            char *str = malloc(len + 1);
            if (!str) return -1;
            if (len && fread(str, 1, (size_t)len, file) != (size_t)len) { free(str); return -1; }
            str[len] = '\0';
            value = str;
            free_value = free;
            break;
        }

        case 2: { /* subdocument */
            Document doc = NULL;
            if (deserialize_document(&doc, file) != 0) return -1;
            value = doc;
            free_value = (void(*)(void *))document_free;
            break;
        }

        default:
            return -1;
    }

    VersionNode node = version_node_create(value, global_version, local_version, NULL, free_value);
    if (!node) {
        if (type == 1 && value) free(value);
        if (type == 2 && value) document_free((Document)value);
        return -1;
    }

    *ver_out = node;
    return 0;
}


