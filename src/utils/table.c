#define _GNU_SOURCE
#include <string.h>

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include "table.h"
#include "collection.h"
#include "document.h"
#include "field.h"
#include "version_node.h"

#define MAX_PATH_LEN 128
#define DEFAULT_BUCKET_COUNT 16

// Memory management
Table table_create(void) {
    Table t = malloc(sizeof *t);
    if (!t) return NULL;
    pthread_rwlock_init(&t->lock, NULL);
    t->collections = hashmap_create(DEFAULT_BUCKET_COUNT);
    return t;
}

void table_free(Table table) {
    if (!table) return;
    pthread_rwlock_wrlock(&table->lock);
    hashmap_free(table->collections);
    pthread_rwlock_unlock(&table->lock);
    pthread_rwlock_destroy(&table->lock);
    free(table);
}

// Returns the field at the path, creates it if it doesn't exist
// Should only aqquire within a thread locked function
// Create flag for whether or not the command can create a path if it doesn't exist
static Field resolve_path(Table table, const char *path, uint64_t version, int create) {
    char *tokens[MAX_PATH_LEN / 2];
    int token_count = 0;

    if (!path || strlen(path) >= MAX_PATH_LEN) return NULL;

    char path_copy[MAX_PATH_LEN];
    strncpy(path_copy, path, MAX_PATH_LEN);
    path_copy[MAX_PATH_LEN - 1] = '\0';

    char *tok = strtok(path_copy, "/");
    while (tok) {
        if (token_count >= (int)(MAX_PATH_LEN / 2)) return NULL;
        tokens[token_count++] = tok;
        tok = strtok(NULL, "/");
    }

    // Schema: col / doc / (subcol / subdoc)* / field  => >=3 and odd
    if (token_count < 3 || (token_count % 2) == 0) return NULL;

    // --- collection ---
    VersionNode coll_vn = hashmap_get(table->collections, tokens[0], version);
    Collection coll = coll_vn ? (Collection)coll_vn->value : NULL;
    if (!coll_vn) {
        if (!create) return NULL;
        coll = collection_create();
        if (!coll) return NULL;
        hashmap_put(table->collections, tokens[0],
            version_node_create(coll, version, 0, NULL, (void (*)(void *))collection_free),
            version, (void (*)(void *))collection_free);
    }

    // --- base document (tokens[1]) ---
    VersionNode doc_vn = hashmap_get(coll->documents, tokens[1], version);
    Document doc = doc_vn ? (Document)doc_vn->value : NULL;
    if (!doc_vn) {
        if (!create) return NULL;
        doc = document_create();
        if (!doc) return NULL;
        collection_set_document(coll, tokens[1], doc, version);
    }

    // --- walk subcollection/subdocument pairs: indices 2..token_count-3 ---
    for (int i = 2; i <= token_count - 3; i += 2) {
        const char *subcoll_key = tokens[i];
        const char *subdoc_key  = tokens[i + 1];

        VersionNode sc_vn = hashmap_get(doc->subcollections, subcoll_key, version);
        coll = sc_vn ? (Collection)sc_vn->value : NULL;
        if (!sc_vn) {
            if (!create) return NULL;
            coll = collection_create();
            if (!coll) return NULL;
            document_set_subcollection(doc, subcoll_key, coll, version);
        }

        VersionNode sd_vn = hashmap_get(coll->documents, subdoc_key, version);
        doc = sd_vn ? (Document)sd_vn->value : NULL;
        if (!sd_vn) {
            if (!create) return NULL;
            doc = document_create();
            if (!doc) return NULL;
            collection_set_document(coll, subdoc_key, doc, version);
        }
    }

    // --- final field ---
    const char *field_key = tokens[token_count - 1];
    VersionNode f_vn = hashmap_get(doc->fields, field_key, version);
    Field field = f_vn ? (Field)f_vn->value : NULL;
    if (!f_vn) {
        if (!create) return NULL;
        field = field_create(field_key);
        if (!field) return NULL;
        document_set_field(doc, field_key, field, version);
    }

    return field;
}

int table_set_field(Table table, const char *path, const char *value, uint64_t version) {
    if (!table || !path || !value) return -1;

    pthread_rwlock_wrlock(&table->lock); // write lock (no rdlock first)

    Field field = resolve_path(table, path, version, /*create=*/1);
    if (!field) {
        pthread_rwlock_unlock(&table->lock);
        return -1;
    }

    char *copy = strdup(value);
    if (!copy) {
        pthread_rwlock_unlock(&table->lock);
        return -1;
    }

    pthread_rwlock_unlock(&table->lock);
    return field_set(field, copy, version, free);
}

void *table_get_field(Table table, const char *path, uint64_t local_version) {
    if (!table || !path) return NULL;

    pthread_rwlock_rdlock(&table->lock);
    Field field = resolve_path(table, path, local_version, /*create=*/0);
    if (!field) {
        pthread_rwlock_unlock(&table->lock);
        return NULL;
    }
    void *value = field_get(field, local_version);
    pthread_rwlock_unlock(&table->lock);
    return value;
}

int table_delete_path(Table table, const char *path, uint64_t version) {
    if (!table || !path) return -1;

    pthread_rwlock_wrlock(&table->lock);
    Field field = resolve_path(table, path, version, /*create=*/0);
    if (!field) {
        pthread_rwlock_unlock(&table->lock);
        return -1;
    }
    int deleted = field_delete(field, version);
    pthread_rwlock_unlock(&table->lock);
    return deleted;
}



// STUBS because I cba rn
int table_list_versions(Table table, const char *path) {
    return 0;
}

int table_compact(Table table, const char *path) {
    return 0;
}
int table_load(Table table, const char *filepath) {
    return 0;
}

int table_save(Table table, const char *filename, const char *path) {
    return 0;
}
