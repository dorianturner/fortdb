#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h> // ntohl, htonl
#include <pthread.h>
#include "deserializer.h"
#include "version_node.h"
#include "document.h"
#include "hash.h"

#ifndef be64toh
#define be64toh(x) (__builtin_bswap64((uint64_t)(x)))
#endif
#ifndef htobe64
#define htobe64(x) (__builtin_bswap64((uint64_t)(x)))
#endif

#define MAGIC "DBV1"
static const uint32_t FORMAT_VER = 1;

static int read_be64(FILE *f, uint64_t *out) {
    uint64_t be;
    if (fread(&be, sizeof(be), 1, f) != 1) return -1;
    *out = be64toh(be);
    return 0;
}

/* Forward declarations */
int deserialize_version_node(VersionNode *ver_out, FILE *file);
int deserialize_document(Document *doc_out, FILE *file);

int deserialize_db(const char *filename, VersionNode *root_out) {
    if (!filename || !root_out) return -1;
    *root_out = NULL;

    FILE *f = fopen(filename, "rb");
    if (!f) return -1;

    char magic[4];
    uint32_t be32;

    if (fread(magic, 1, 4, f) != 4) goto fail;
    if (memcmp(magic, MAGIC, 4) != 0) goto fail;

    if (fread(&be32, sizeof(be32), 1, f) != 1) goto fail;
    if (ntohl(be32) != FORMAT_VER) goto fail;

    if (fread(&be32, sizeof(be32), 1, f) != 1) goto fail; // reserved

    uint64_t ver_count;
    if (read_be64(f, &ver_count) != 0) goto fail;

    VersionNode head = NULL;
    VersionNode tail = NULL;

    for (uint64_t i = 0; i < ver_count; i++) {
        VersionNode ver = NULL;
        if (deserialize_version_node(&ver, f) != 0) goto fail;
        ver->prev = NULL;

        if (!head) head = tail = ver;
        else { tail->prev = ver; tail = ver; }
    }

    fclose(f);
    *root_out = head;
    return 0;

fail:
    if (f) fclose(f);
    *root_out = NULL;
    return -1;
}

/* Deserialize a VersionNode */
int deserialize_version_node(VersionNode *ver_out, FILE *file) {
    if (!ver_out || !file) return -1;
    *ver_out = NULL;

    uint64_t global_version, local_version;
    uint8_t type;

    if (read_be64(file, &global_version) != 0) return -1;
    if (read_be64(file, &local_version) != 0) return -1;
    if (fread(&type, sizeof(type), 1, file) != 1) return -1;

    void *value = NULL;
    void (*free_value)(void *) = NULL;

    switch (type) {
        case 0: value = DELETED; free_value = NULL; break;
        case 1: { // string
            uint64_t len;
            if (read_be64(file, &len) != 0) return -1;
            char *str = malloc(len + 1);
            if (!str) return -1;
            if (len && fread(str, 1, len, file) != len) { free(str); return -1; }
            str[len] = '\0';
            value = str;
            free_value = free;
            break;
        }
        case 2: { // subdocument
            Document doc = NULL;
            if (deserialize_document(&doc, file) != 0) return -1;
            value = doc;
            free_value = (void(*)(void *))document_free;
            break;
        }
        default: return -1;
    }

    *ver_out = version_node_create(value, global_version, local_version, NULL, free_value);
    if (!*ver_out) {
        if (type == 1 && value) free(value);
        if (type == 2 && value) document_free((Document)value);
        return -1;
    }

    return 0;
}

/* Deserialize a Document */
int deserialize_document(Document *doc_out, FILE *file) {
    if (!doc_out || !file) return -1;
    *doc_out = document_create();
    if (!*doc_out) return -1;

    uint64_t count;
    if (read_be64(file, &count) != 0) goto fail;

    // Fields
    for (uint64_t i = 0; i < count; i++) {
        uint64_t key_len;
        if (read_be64(file, &key_len) != 0) goto fail;
        char *key = malloc(key_len + 1);
        if (!key) goto fail;
        if (key_len && fread(key, 1, key_len, file) != key_len) { free(key); goto fail; }
        key[key_len] = '\0';

        uint64_t ver_count;
        if (read_be64(file, &ver_count) != 0) { free(key); goto fail; }

        VersionNode head = NULL, tail = NULL;
        for (uint64_t v = 0; v < ver_count; v++) {
            VersionNode ver = NULL;
            if (deserialize_version_node(&ver, file) != 0) { free(key); goto fail; }
            ver->prev = NULL;
            if (!head) head = tail = ver;
            else { tail->prev = ver; tail = ver; }
        }

        if (hashmap_set_raw((*doc_out)->fields, key, head) != 0) { free(key); goto fail; }
        free(key);
    }

    // Subdocuments
    if (read_be64(file, &count) != 0) goto fail;
    for (uint64_t i = 0; i < count; i++) {
        uint64_t key_len;
        if (read_be64(file, &key_len) != 0) goto fail;
        char *key = malloc(key_len + 1);
        if (!key) goto fail;
        if (key_len && fread(key, 1, key_len, file) != key_len) { free(key); goto fail; }
        key[key_len] = '\0';

        uint64_t ver_count;
        if (read_be64(file, &ver_count) != 0) { free(key); goto fail; }

        VersionNode head = NULL, tail = NULL;
        for (uint64_t v = 0; v < ver_count; v++) {
            VersionNode ver = NULL;
            if (deserialize_version_node(&ver, file) != 0) { free(key); goto fail; }
            ver->prev = NULL;
            if (!head) head = tail = ver;
            else { tail->prev = ver; tail = ver; }
        }

        if (hashmap_set_raw((*doc_out)->subdocuments, key, head) != 0) { free(key); goto fail; }
        free(key);
    }

    return 0;

fail:
    if (*doc_out) document_free(*doc_out);
    *doc_out = NULL;
    return -1;
}
