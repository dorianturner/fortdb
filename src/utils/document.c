#define _GNU_SOURCE
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "hash.h"
#include "document.h"
#include "version_node.h"

#ifndef DELETED
#define DELETED ((void*)1)
#endif

#define DEFAULT_BUCKET_COUNT 16

/* Helper: find Entry for key by scanning buckets (avoids needing hash() from hash.c) */
static Entry hashmap_find_entry(Hashmap map, const char *key) {
    if (!map || !key) return NULL;
    for (uint64_t i = 0; i < map->bucket_count; i++) {
        Entry e = map->buckets[i];
        while (e) {
            if (strcmp(e->key, key) == 0) return e;
            e = e->next;
        }
    }
    return NULL;
}

Document document_create(void) {
    Document doc = malloc(sizeof(struct Document));
    if (!doc) return NULL;

    if (pthread_rwlock_init(&doc->lock, NULL) != 0) {
        free(doc);
        return NULL;
    }

    doc->fields = hashmap_create(DEFAULT_BUCKET_COUNT);
    if (!doc->fields) {
        pthread_rwlock_destroy(&doc->lock);
        free(doc);
        return NULL;
    }

    doc->subdocuments = hashmap_create(DEFAULT_BUCKET_COUNT);
    if (!doc->subdocuments) {
        hashmap_free(doc->fields);
        pthread_rwlock_destroy(&doc->lock);
        free(doc);
        return NULL;
    }

    return doc;
}

void document_free(Document doc) {
    if (!doc) return;
    hashmap_free(doc->fields);        // frees string values via their free callback (or leaves sentinel)
    hashmap_free(doc->subdocuments);  // frees subdocuments recursively via document_free callback
    pthread_rwlock_destroy(&doc->lock);
    free(doc);
}

/* ---------- helpers for path traversal ---------- */

static int resolve_parent_and_key(Document root,
                                  const char *path,
                                  Document *out_parent,
                                  char **out_key,
                                  int create_missing,
                                  uint64_t global_version)
{
    if (!root || !path || !out_parent || !out_key) return -1;

    char *tmp = strdup(path);
    if (!tmp) return -1;

    char *saveptr = NULL;
    char *token = strtok_r(tmp, "/", &saveptr);
    if (!token) { free(tmp); return -1; }

    Document current = root;
    char *next = NULL;
    while (1) {
        next = strtok_r(NULL, "/", &saveptr);
        if (next == NULL) {
            /* token is the final component */
            *out_parent = current;
            *out_key = strdup(token);
            free(tmp);
            if (!*out_key) return -1;
            return 0;
        }

        /* token is an intermediate component: ensure subdocument exists (or resolve it) */
        Document child = document_get_subdocument(current, token, 0);
        if (!child && create_missing) {
            child = document_create();
            if (!child) { free(tmp); return -1; }
            if (document_set_subdocument(current, token, child, global_version) != 0) {
                document_free(child);
                free(tmp);
                return -1;
            }
        }
        if (!child) {
            /* missing and not creating */
            free(tmp);
            return -1;
        }
        current = child;
        token = next;
    }
}

/* ---------- field get / set (path-aware) ---------- */

/*
 * Returns:
 *  - pointer to the string value for the requested local_version (do NOT free),
 *  - DELETED sentinel if it's a tombstone,
 *  - NULL if not found.
 *
 * Caller should treat returned pointer as read-only (owned by the version system).
 */

/* document_get_field: return exact local_version, or if local_version == UINT64_MAX return latest */
char *document_get_field(Document doc, const char *key_or_path, uint64_t local_version) {
    if (!doc || !key_or_path) return NULL;

    Document parent = NULL;
    char *final_key = NULL;
    if (resolve_parent_and_key(doc, key_or_path, &parent, &final_key, 0, 0) != 0) {
        return NULL;
    }

    if (!parent) { free(final_key); return NULL; }

    if (pthread_rwlock_rdlock(&parent->lock) != 0) {
        free(final_key);
        return NULL;
    }

    /* If caller asked for "latest" sentinel, return head->value directly */
    if (local_version == UINT64_MAX) {
        Entry e = hashmap_find_entry(parent->fields, final_key);
        char *val = NULL;
        if (e) {
            VersionNode head = (VersionNode)e->value;
            val = head ? (char *)head->value : NULL;
        }
        pthread_rwlock_unlock(&parent->lock);
        free(final_key);
        return val;
    }

    /* existing exact-match behavior */
    char *val = (char *)hashmap_get(parent->fields, final_key, local_version);
    pthread_rwlock_unlock(&parent->lock);
    free(final_key);
    return val;
}


/* set a string value at key (creates/updates the version chain). value is duplicated internally. */
int document_set_field(Document doc, const char *key, const char *value, uint64_t global_version) {
    if (!doc || !key || !value) return -1;

    char *dup = strdup(value);
    if (!dup) return -1;

    if (pthread_rwlock_wrlock(&doc->lock) != 0) {
        free(dup);
        return -1;
    }
    /* pass free as the free_value callback so the string will be freed when the version chain is freed */
    int rc = hashmap_put(doc->fields, key, dup, global_version, free);
    pthread_rwlock_unlock(&doc->lock);
    if (rc != 0) {
        /* hashmap_put failed and will not have taken ownership of dup */
        free(dup);
        return -1;
    }
    return 0;
}

/* Convenience: keep an alias for the previous name (still duplicates value internally) */
int document_set_field_cstr(Document doc, const char *key, const char *value, uint64_t global_version) {
    return document_set_field(doc, key, value, global_version);
}

int document_set_field_path(Document root, const char *path, const char *value, uint64_t global_version) {
    if (!root || !path || !value) return -1;

    Document parent = NULL;
    char *final_key = NULL;
    if (resolve_parent_and_key(root, path, &parent, &final_key, 1, global_version) != 0) {
        return -1;
    }
    if (!parent || !final_key) { free(final_key); return -1; }

    int rc = document_set_field(parent, final_key, value, global_version);
    free(final_key);
    return rc;
}

/* ---------- subdocument get/set (immediate only) ---------- */

Document document_get_subdocument(Document doc, const char *key, uint64_t local_version) {
    if (!doc || !key) return NULL;
    if (pthread_rwlock_rdlock(&doc->lock) != 0) return NULL;
    Document subdoc = (Document)hashmap_get(doc->subdocuments, key, local_version);
    pthread_rwlock_unlock(&doc->lock);
    return subdoc;
}

int document_set_subdocument(Document doc, const char *key, Document subdoc, uint64_t global_version) {
    if (!doc || !key || !subdoc) return -1;
    if (pthread_rwlock_wrlock(&doc->lock) != 0) return -1;
    int rc = hashmap_put(doc->subdocuments, key, subdoc, global_version, (void (*)(void *))document_free);
    pthread_rwlock_unlock(&doc->lock);
    return rc;
}

/* ---------- delete / admin helpers ---------- */

int document_delete_path(Document doc, const char *path, uint64_t global_version) {
    if (!doc || !path) return -1;

    Document parent = NULL;
    char *final_key = NULL;
    if (resolve_parent_and_key(doc, path, &parent, &final_key, 0, 0) != 0) {
        /* nothing to delete */
        return 0;
    }
    if (!parent || !final_key) { free(final_key); return -1; }

    /* First check existence under read lock */
    if (pthread_rwlock_rdlock(&parent->lock) != 0) {
        free(final_key);
        return -1;
    }
    Entry e = hashmap_find_entry(parent->fields, final_key);
    pthread_rwlock_unlock(&parent->lock);

    if (!e) {
        free(final_key);
        return 0; /* nothing to delete */
    }

    /* Add tombstone version under write lock */
    if (pthread_rwlock_wrlock(&parent->lock) != 0) {
        free(final_key);
        return -1;
    }
    /* store DELETED sentinel; pass NULL as free callback so sentinel isn't freed */
    int rc = hashmap_put(parent->fields, final_key, DELETED, global_version, NULL);
    pthread_rwlock_unlock(&parent->lock);
    free(final_key);
    return rc;
}

/*
 * List versions for a field at `path`.
 * Prints lines like:
 *   v1: <value>
 *   v2: <value>
 *
 * Assumes values are printable C-strings (or DELETED sentinel).
 */
int document_list_versions(Document doc, const char *path) {
    if (!doc || !path) return -1;

    Document parent = NULL;
    char *final_key = NULL;
    if (resolve_parent_and_key(doc, path, &parent, &final_key, 0, 0) != 0) {
        fprintf(stderr, "document_list_versions: path not found: %s\n", path);
        return -1;
    }

    if (!parent) { free(final_key); return -1; }

    if (pthread_rwlock_rdlock(&parent->lock) != 0) {
        free(final_key);
        return -1;
    }

    Entry e = hashmap_find_entry(parent->fields, final_key);
    if (!e) {
        pthread_rwlock_unlock(&parent->lock);
        fprintf(stderr, "document_list_versions: field not found: %s\n", path);
        free(final_key);
        return -1;
    }

    /* e->value is the head VersionNode for that key (hashmap stores VersionNode chains) */
    VersionNode curr = (VersionNode)e->value;
    while (curr) {
        /* print 1-based "v1, v2" numbering and a space after colon to match README/tests */
        if (curr->value == DELETED) {
            printf("v%llu: <deleted>\n", (unsigned long long)(curr->local_version));
        } else {
            char *s = (char *)curr->value;
            if (s) printf("v%llu: %s\n", (unsigned long long)(curr->local_version), s);
            else printf("v%llu: <nil>\n", (unsigned long long)(curr->local_version));
        }
        curr = curr->prev;
    }
    pthread_rwlock_unlock(&parent->lock);

    free(final_key);
    return 0;
}

/* Compact stub (no-op for now). */
int document_compact(Document doc, const char *path) {
    (void)doc; (void)path;
    return 0;
}

/* load/save stubs: do not print here; decode_and_execute prints a single message. */
int document_load(Document doc, const char *path) {
    (void)doc; (void)path;
    return 0;
}

int document_save(Document doc, const char *filename, const char *path) {
    (void)doc; (void)filename; (void)path;
    return 0;
}

