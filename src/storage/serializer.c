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
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

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
static int serialize_document_locked(Document doc, FILE *file) {
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

int serialize_document(Document doc, FILE *file) {
    if (!doc || !file) return -1;
    if (pthread_rwlock_rdlock(&doc->lock) != 0) return -1;
    int ret = serialize_document_locked(doc, file);
    pthread_rwlock_unlock(&doc->lock);
    return ret;
}

/* Serialize DB root */
int serialize_db(VersionNode root, const char *filename) {
    if (!root || !filename) return -1;

    size_t temp_len = strlen(filename) + sizeof(".tmp.XXXXXX");
    char *temp_name = malloc(temp_len);
    if (!temp_name) return -1;
    int n = snprintf(temp_name, temp_len, "%s.tmp.XXXXXX", filename);
    if (n < 0 || (size_t)n >= temp_len) {
        free(temp_name);
        return -1;
    }
    int fd = mkstemp(temp_name);
    if (fd < 0) {
        free(temp_name);
        return -1;
    }
    FILE *f = fdopen(fd, "wb");
    if (!f) {
        close(fd);
        unlink(temp_name);
        free(temp_name);
        return -1;
    }

    if (pthread_rwlock_rdlock(&root->lock) != 0) {
        fclose(f);
        unlink(temp_name);
        free(temp_name);
        return -1;
    }

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

    if (fflush(f) != 0 || fsync(fileno(f)) != 0) goto fail;
    if (fclose(f) != 0) {
        f = NULL;
        goto fail;
    }
    f = NULL;
    if (rename(temp_name, filename) != 0) goto fail_after_close;

    /* A successful rename is atomic; syncing the directory makes the name
     * replacement durable across a crash on POSIX filesystems. */
    const char *slash = strrchr(filename, '/');
    char *dir = NULL;
    if (slash) {
        size_t dir_len = (size_t)(slash - filename);
        dir = malloc(dir_len + 1);
        if (!dir) goto fail_after_rename;
        memcpy(dir, filename, dir_len);
        dir[dir_len] = '\0';
    } else {
        dir = strdup(".");
        if (!dir) goto fail_after_rename;
    }
    int dir_fd = open(dir, O_RDONLY);
    free(dir);
    if (dir_fd < 0 || fsync(dir_fd) != 0) {
        if (dir_fd >= 0) close(dir_fd);
        free(temp_name);
        pthread_rwlock_unlock(&root->lock);
        return -1;
    }
    close(dir_fd);
    free(temp_name);
    pthread_rwlock_unlock(&root->lock);
    return 0;

fail:
    if (f) fclose(f);
    pthread_rwlock_unlock(&root->lock);
    unlink(temp_name);
    free(temp_name);
    return -1;

fail_after_close:
    unlink(temp_name);
fail_after_rename:
    free(temp_name);
    pthread_rwlock_unlock(&root->lock);
    return -1;
}
