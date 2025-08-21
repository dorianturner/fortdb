#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <winsock2.h>
#include "windows_compat.h"
#define be64toh(x) _byteswap_uint64(x)
#define htobe64(x) _byteswap_uint64(x)
#else
#include <arpa/inet.h> // ntohl, htonl
#include <endian.h>    // be64toh, htobe64
#include <pthread.h>
#include <unistd.h>    // fsync
#endif

#include "deserializer.h"
#include "serializer.h"
#include "version_node.h"
#include "document.h"
#include "hash.h"

#define MAGIC "DBV1"
static const uint32_t FORMAT_VER = 1;

/* Deserialize DB root */
int deserialize_db(const char *filename, VersionNode *root_out) {
    if (!filename || !root_out) return -1;

    FILE *f = fopen(filename, "rb");
    if (!f) return -1;

    char magic[4];
    uint32_t be32;

    if (fread(magic, 1, 4, f) != 4) goto fail;
    if (memcmp(magic, MAGIC, 4) != 0) goto fail;

    if (fread(&be32, sizeof(be32), 1, f) != 1) goto fail;
    if (ntohl(be32) != FORMAT_VER) goto fail;

    if (fread(&be32, sizeof(be32), 1, f) != 1) goto fail; // reserved flags

    uint64_t be64;
    if (fread(&be64, sizeof(be64), 1, f) != 1) goto fail;
    uint64_t ver_count = be64toh(be64);

    VersionNode prev = NULL, root = NULL;
    for (uint64_t i = 0; i < ver_count; i++) {
        VersionNode ver = NULL;
        if (deserialize_version_node(&ver, f) != 0) goto fail;
        ver->prev = prev;
        prev = ver;
        if (!root) root = ver;
    }

    fclose(f);
    *root_out = root;
    return 0;

fail:
    fclose(f);
    return -1;
}

/* Deserialize a Document from file */
int deserialize_document(Document *doc_out, FILE *file) {
    if (!doc_out || !file) return -1;

    Document doc = document_create();
    if (!doc) return -1;

    uint64_t be64, count;

    // 1. Number of fields
    if (fread(&be64, sizeof(be64), 1, file) != 1) goto fail;
    count = be64toh(be64);

    for (uint64_t i = 0; i < count; i++) {
        // Read key
        if (fread(&be64, sizeof(be64), 1, file) != 1) goto fail;
        uint64_t key_len = be64toh(be64);
        char *key = malloc(key_len + 1);
        if (!key) goto fail;
        if (key_len && fread(key, 1, (size_t)key_len, file) != (size_t)key_len) { free(key); goto fail; }
        key[key_len] = '\0';

        // Number of versions
        if (fread(&be64, sizeof(be64), 1, file) != 1) { free(key); goto fail; }
        uint64_t ver_count = be64toh(be64);

        VersionNode head = NULL, prev = NULL;
        for (uint64_t v = 0; v < ver_count; v++) {
            VersionNode ver = NULL;
            if (deserialize_version_node(&ver, file) != 0) { free(key); goto fail; }
            ver->prev = prev;
            prev = ver;
            if (!head) head = ver;
        }

        if (hashmap_set_raw(doc->fields, key, head) != 0) {
            free(key);
            goto fail;
        }

        free(key);
    }

    // 2. Number of subdocuments
    if (fread(&be64, sizeof(be64), 1, file) != 1) goto fail;
    count = be64toh(be64);

    for (uint64_t i = 0; i < count; i++) {
        // Read key
        if (fread(&be64, sizeof(be64), 1, file) != 1) goto fail;
        uint64_t key_len = be64toh(be64);
        char *key = malloc(key_len + 1);
        if (!key) goto fail;
        if (key_len && fread(key, 1, (size_t)key_len, file) != (size_t)key_len) { free(key); goto fail; }
        key[key_len] = '\0';

        // Number of versions
        if (fread(&be64, sizeof(be64), 1, file) != 1) { free(key); goto fail; }
        uint64_t ver_count = be64toh(be64);

        VersionNode head = NULL, prev = NULL;
        for (uint64_t v = 0; v < ver_count; v++) {
            VersionNode ver = NULL;
            if (deserialize_version_node(&ver, file) != 0) { free(key); goto fail; }
            ver->prev = prev;
            prev = ver;
            if (!head) head = ver;
        }

        if (hashmap_set_raw(doc->subdocuments, key, head) != 0) {
            free(key);
            goto fail;
        }

        free(key);
    }

    *doc_out = doc;
    return 0;

fail:
    if (doc) document_free(doc);
    return -1;
}

/* Deserialize a VersionNode from file */
int deserialize_version_node(VersionNode *ver_out, FILE *file) {
    if (!ver_out || !file) return -1;

    uint64_t be64;
    uint8_t type;

    // 1. Read global_version
    if (fread(&be64, sizeof(be64), 1, file) != 1) return -1;
    uint64_t global_version = be64toh(be64);

    // 2. Read local_version
    if (fread(&be64, sizeof(be64), 1, file) != 1) return -1;
    uint64_t local_version = be64toh(be64);

    // 3. Read type
    if (fread(&type, sizeof(type), 1, file) != 1) return -1;

    void *value = NULL;
    void (*free_value)(void*) = NULL;

    switch (type) {
        case 0: // tombstone
            value = DELETED;
            break;

        case 1: { // string
            if (fread(&be64, sizeof(be64), 1, file) != 1) return -1;
            uint64_t len = be64toh(be64);

            char *str = malloc(len + 1);
            if (!str) return -1;

            if (len && fread(str, 1, (size_t)len, file) != (size_t)len) {
                free(str);
                return -1;
            }
            str[len] = '\0';
            value = str;
            free_value = free;
            break;
        }

        case 2: { // subdocument
            Document doc = NULL;
            if (deserialize_document(&doc, file) != 0) return -1;
            value = doc;
            break;
        }

        default:
            return -1;
    }

    // Create VersionNode with no prev yet
    VersionNode ver = version_node_create(value, global_version, local_version, NULL, free_value);
    if (!ver) {
        if (type == 1) free(value);
        else if (type == 2) document_free((Document)value);
        return -1;
    }

    *ver_out = ver;
    return 0;
}

