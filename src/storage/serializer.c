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

#include "serializer.h"
#include "deserializer.h"
#include "version_node.h"
#include "document.h"
#include "hash.h"

#define MAGIC "DBV1"
static const uint32_t FORMAT_VER = 1;



/* Helper: write a 64-bit integer in big-endian */
static int write_be64(FILE *f, uint64_t val) {
    uint64_t be = htobe64(val);
    return fwrite(&be, sizeof(be), 1, f) == 1 ? 0 : -1;
}

/* Serialize a VersionNode */
int serialize_version_node(VersionNode ver, FILE *file) {
    if (!ver || !file) return -1;

    uint64_t be64;

    // global_version
    be64 = htobe64(ver->global_version);
    if (fwrite(&be64, sizeof(be64), 1, file) != 1) return -1;

    // local_version
    be64 = htobe64(ver->local_version);
    if (fwrite(&be64, sizeof(be64), 1, file) != 1) return -1;

    // type + value
    if (ver->value == DELETED) {
        uint8_t type = 0;
        if (fwrite(&type, sizeof(type), 1, file) != 1) return -1;
    } else if (ver->free_value == free) { // string
        uint8_t type = 1;
        if (fwrite(&type, sizeof(type), 1, file) != 1) return -1;

        char *str = (char*)ver->value;
        uint64_t len = strlen(str);
        if (write_be64(file, len) != 0) return -1;
        if (len && fwrite(str, 1, len, file) != len) return -1;
    } else { // subdocument
        uint8_t type = 2;
        if (fwrite(&type, sizeof(type), 1, file) != 1) return -1;
        if (serialize_document((Document)ver->value, file) != 0) return -1;
    }

    return 0;
}

/* Serialize a Document */
int serialize_document(Document doc, FILE *file) {
    if (!doc || !file) return -1;

    // 1. Fields
    size_t field_count = doc->fields->size;
    if (write_be64(file, field_count) != 0) return -1;

    for (uint64_t i = 0; i < doc->fields->bucket_count; i++) {
        for (Entry e = doc->fields->buckets[i]; e; e = e->next) {
            char *key = e->key;
            VersionNode chain = (VersionNode)e->value;

            uint64_t key_len = strlen(key);
            if (write_be64(file, key_len) != 0) return -1;
            if (key_len && fwrite(key, 1, key_len, file) != key_len) return -1;

            // Count version nodes in chain
            size_t ver_count = 0;
            for (VersionNode v = chain; v; v = v->prev) ver_count++;
            if (write_be64(file, ver_count) != 0) return -1;

            // Serialize version nodes
            for (VersionNode v = chain; v; v = v->prev) {
                if (serialize_version_node(v, file) != 0) return -1;
            }
        }
    }

    // 2. Subdocuments
    size_t sub_count = doc->subdocuments->size;
    if (write_be64(file, sub_count) != 0) return -1;

    for (uint64_t i = 0; i < doc->subdocuments->bucket_count; i++) {
        for (Entry e = doc->subdocuments->buckets[i]; e; e = e->next) {
            char *key = e->key;
            VersionNode chain = (VersionNode)e->value;

            uint64_t key_len = strlen(key);
            if (write_be64(file, key_len) != 0) return -1;
            if (key_len && fwrite(key, 1, key_len, file) != key_len) return -1;

            size_t ver_count = 0;
            for (VersionNode v = chain; v; v = v->prev) ver_count++;
            if (write_be64(file, ver_count) != 0) return -1;

            for (VersionNode v = chain; v; v = v->prev) {
                if (serialize_version_node(v, file) != 0) return -1;
            }
        }
    }

    return 0;
}

/* Serialize DB root */
int serialize_db(VersionNode root, const char *filename) {
    if (!filename) return -1;

    FILE *f = fopen(filename, "wb");
    if (!f) return -1;

    // Magic
    if (fwrite(MAGIC, 1, 4, f) != 4) goto fail;

    // Format version
    uint32_t be32 = htonl(FORMAT_VER);
    if (fwrite(&be32, sizeof(be32), 1, f) != 1) goto fail;

    // Reserved flags
    be32 = 0;
    if (fwrite(&be32, sizeof(be32), 1, f) != 1) goto fail;

    // Count root versions
    size_t count = 0;
    for (VersionNode v = root; v; v = v->prev) count++;
    if (write_be64(f, count) != 0) goto fail;

    // Serialize root versions
    for (VersionNode v = root; v; v = v->prev) {
        if (serialize_version_node(v, f) != 0) goto fail;
    }

    fclose(f);
    return 0;

fail:
    if (f) fclose(f);
    return -1;
}
