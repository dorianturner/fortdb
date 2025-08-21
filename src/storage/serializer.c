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

static char deleted_marker;
#define DELETED ((void *)&deleted_marker)

#include "serializer.h"
#include "version_node.h"
#include "document.h"
#include "field.h"
#include "hash.h"

#ifndef ERR_OK
#define ERR_OK 0
#define ERR_WRITE 1
#define ERR_LOCK 2
#define ERR_NOMEM 3
#endif

/* Type tags for version node payloads */
#define TAG_STRING   1   /* value is NUL-terminated C string (binary-safe via strlen) */
#define TAG_SUBDOC   2   /* value is Document* */
#define TAG_DELETED  3   /* value == DELETED */

/* Forward declarations of your prewritten types (user-supplied elsewhere) */
typedef struct VersionNode *VersionNode;
typedef struct Entry *Entry;
typedef struct Hashmap *Hashmap;
typedef struct Document *Document;
typedef struct Field *Field;

/* External functions provided elsewhere in codebase (declared here for compiler) */
extern Document document_create(void);
extern void document_free(Document doc);
extern Field field_create(const char *name);
extern void field_free(Field field);

/* Forward prototype */
int serialize_document(Document doc, FILE *f);

/* Write a version chain. is_field==1 => treat value as C string or DELETED.
   is_field==0 => treat value as Document* or DELETED. */
static int write_version_node(FILE *f, VersionNode v, int is_field) {
    /* count nodes in chain */
    uint64_t count = 0;
    for (VersionNode t = v; t; t = t->prev) count++;
    uint64_t count_be = htobe64(count);
    if (fwrite(&count_be, sizeof(uint64_t), 1, f) != 1) return ERR_WRITE;

    /* write nodes latest-first */
    for (VersionNode t = v; t; t = t->prev) {
        uint64_t local_be = htobe64(t->local_version);
        uint64_t global_be = htobe64(t->global_version);
        if (fwrite(&local_be, sizeof(uint64_t), 1, f) != 1) return ERR_WRITE;
        if (fwrite(&global_be, sizeof(uint64_t), 1, f) != 1) return ERR_WRITE;

        /* determine tag */
        if (t->value == DELETED) {
            uint8_t tag = TAG_DELETED;
            if (fwrite(&tag, sizeof(uint8_t), 1, f) != 1) return ERR_WRITE;
            /* no payload */
            continue;
        }

        if (is_field) {
            /* fields: expect C string (user code used strlen previously).
               This is binary-safe only if values are NUL-terminated strings.
               If your values can be binary blobs, you must add a length field
               to VersionNode in the core data model. */
            char *s = (char *)t->value;
            uint8_t tag = TAG_STRING;
            if (fwrite(&tag, sizeof(uint8_t), 1, f) != 1) return ERR_WRITE;

            uint64_t val_len = (s != NULL) ? (uint64_t)strlen(s) : 0;
            uint64_t val_len_be = htobe64(val_len);
            if (fwrite(&val_len_be, sizeof(uint64_t), 1, f) != 1) return ERR_WRITE;
            if (val_len > 0) {
                if (fwrite(s, 1, val_len, f) != val_len) return ERR_WRITE;
            }
        } else {
            /* subdocuments: expect Document* */
            uint8_t tag = TAG_SUBDOC;
            if (fwrite(&tag, sizeof(uint8_t), 1, f) != 1) return ERR_WRITE;
            Document sub = (Document)t->value;
            if (!sub) {
                /* treat NULL subdocument as zero-length subdoc: write zero entries */
                /* We still call serialize_document which will write empty maps. */
            }
            int res = serialize_document(sub, f);
            if (res != ERR_OK) return res;
        }
    }
    return ERR_OK;
}

/* Serialize a hashmap as:
   [uint64_t entry_count]
     for each entry:
       [uint32_t key_len][key bytes]
       [version_chain...] (written by write_version_node)
*/
int serialize_hashmap(FILE* f, Hashmap map, int is_field) {
    if (!map) {
        uint64_t zero = 0;
        uint64_t zero_be = htobe64(zero);
        if (fwrite(&zero_be, sizeof(uint64_t), 1, f) != 1) return ERR_WRITE;
        return ERR_OK;
    }

    /* count entries (iterate buckets) */
    uint64_t entries = 0;
    for (uint64_t i = 0; i < map->bucket_count; i++) {
        Entry e = map->buckets[i];
        while (e) { entries++; e = e->next; }
    }
    uint64_t entries_be = htobe64(entries);
    if (fwrite(&entries_be, sizeof(uint64_t), 1, f) != 1) return ERR_WRITE;

    /* write entries */
    for (uint64_t i = 0; i < map->bucket_count; i++) {
        Entry e = map->buckets[i];
        while (e) {
            const char *key = e->key ? e->key : "";
            uint32_t key_len = (uint32_t)strlen(key);
            uint32_t key_len_be = htonl(key_len);
            if (fwrite(&key_len_be, sizeof(uint32_t), 1, f) != 1) return ERR_WRITE;
            if (key_len > 0) {
                if (fwrite(key, 1, key_len, f) != key_len) return ERR_WRITE;
            }

            /* e->value is a VersionNode per your definitions */
            VersionNode v = (VersionNode)e->value;
            int res = write_version_node(f, v, is_field);
            if (res != ERR_OK) return res;

            e = e->next;
        }
    }
    return ERR_OK;
}

int serialize_document(Document doc, FILE* f) {
    if (!doc) {
        /* write empty maps for a NULL document */
        uint64_t zero = 0;
        uint64_t zero_be = htobe64(zero);
        if (fwrite(&zero_be, sizeof(uint64_t), 1, f) != 1) return ERR_WRITE; /* fields count */
        if (fwrite(&zero_be, sizeof(uint64_t), 1, f) != 1) return ERR_WRITE; /* subdocs count */
        return ERR_OK;
    }

    if (pthread_rwlock_rdlock(&doc->lock) != 0) return ERR_LOCK;

    int res;
    res = serialize_hashmap(f, doc->fields, 1);
    if (res != ERR_OK) { pthread_rwlock_unlock(&doc->lock); return res; }

    res = serialize_hashmap(f, doc->subdocuments, 0);
    if (res != ERR_OK) { pthread_rwlock_unlock(&doc->lock); return res; }

    pthread_rwlock_unlock(&doc->lock);
    return ERR_OK;
}

int serialize_db(Document root, const char *filename) {
    FILE *f = fopen(filename, "wb");
    if (!f) return ERR_WRITE;

    uint32_t db_version = htonl(1);
    if (fwrite(&db_version, sizeof(uint32_t), 1, f) != 1) { fclose(f); return ERR_WRITE; }

    int res = serialize_document(root, f);
    fclose(f);
    return res;
}
