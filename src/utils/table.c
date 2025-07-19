#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include "table.h"
#include "collection.h"
#include "document.h"
#include "field.h"
#include "version_node.h"

#define MAX_PATH_LEN 128

// Memory management
Table table_create(void) {
    Table t = malloc(sizeof *t);
    if (!t) return NULL;
    pthread_rwlock_init(&t->lock, NULL);
    t->collections = hashmap_create();
    return t;
}

void table_free(Table table) {
    if (!table) return;
    pthread_rwlock_wrlock(&table->lock);
    HashMapIterator it = hashmap_iterator(table->collections);
    while (hashmap_next(&it)) {
        version_node_free((VersionNode)it.value);
    }
    hashmap_free(table->collections);
    pthread_rwlock_unlock(&table->lock);
    pthread_rwlock_destroy(&table->lock);
    free(table);
}

// Returns the field at the path, creates it if it doesn't exist
// Should only aqquire within a thread locked function
static Field resolve_path(Table table, const char *path, uint64_t global_version) {
    char *tokens[MAX_PATH_LEN / 2]; // case a/b/a/b/.../f as path
    int token_count = 0;

    if (strlen(path) >= MAX_PATH_LEN) return NULL;

    char path_copy[MAX_PATH_LEN];
    strncpy(path_copy, path, MAX_PATH_LEN);
    path_copy[MAX_PATH_LEN - 1] = '\0';

    char *tok = strtok(path_copy, "/");
    while (tok) {
        if (token_count >= MAX_PATH_LEN / 2) return NULL;
        tokens[token_count++] = tok;
        tok = strtok(NULL, "/");
    }

    // enforcing path structure of col/doc/[subcol/doc/...]/field
    if (token_count < 3 || token_count % 2 == 0) return NULL;

    VersionNode coll_vnode = hashmap_get(table->collections, tokens[0], global_version);
    Collection coll = coll_vnode ? (Collection)coll_vnode->value : collection_create();
    if (!coll_vnode) {
        hashmap_put(table->collections, tokens[0],
            version_node_create(coll, global_version, 0, NULL, (void (*)(void *))collection_free),
            global_version, (void (*)(void *))collection_free);
    }

    Document doc = NULL;
    // Basically mutual recursion to get final doc
    for (int i = 1; i < token_count - 1; i += 2) {
        const char *doc_key = tokens[i];
        const char *subcoll_key = tokens[i + 1];

        VersionNode doc_vnode = hashmap_get(coll->documents, doc_key, global_version);
        doc = doc_vnode ? (Document)doc_vnode->value : document_create();
        if (!doc_vnode) {
            collection_set_document(coll, doc_key, doc, global_version);
        }

        VersionNode subcoll_vnode = hashmap_get(doc->subcollections, subcoll_key, global_version);
        coll = subcoll_vnode ? (Collection)subcoll_vnode->value : collection_create();
        if (!subcoll_vnode) {
            document_set_subcollection(doc, subcoll_key, coll, global_version);
        }
    }

    const char *field_key = tokens[token_count - 1];
    VersionNode field_vnode = hashmap_get(doc->fields, field_key, global_version);
    Field field = field_vnode ? (Field)field_vnode->value : field_create(field_key);
    if (!field_vnode) {
        document_set_field(doc, field_key, field, global_version);
    }

    return field;
}

int table_set_field(Table table, const char *path, const char *value, uint64_t global_version) {
    if (!table || !path || !value) return -1;
    pthread_rwlock_rdlock(&table->lock);

    Field field = resolve_path(table, path, global_version);
    if (!field) return -1; // Some part of mallocing this field path failed

    char *copy = strdup(value);
    if (!copy) return -1;
    pthread_rwlock_unlock(&table->lock);

    return field_set(field, copy, global_version, free);
}

void *table_get_field(Table table, const char *path, uint64_t version) {
    if (!table || !path) return NULL;

    pthread_rwlock_rdlock(&table->lock);
    Field field = resolve_path(table, path, version);
    if (!field) {
        pthread_rwlock_unlock(&table->lock);
        return NULL;
    }
    void *value = field_get(field, version);
    pthread_rwlock_unlock(&table->lock);
    return value;
}

int table_delete_path(Table table, const char *path, uint64_t global_version) {
    if (!table || !path) return NULL;

    pthread_rwlock_rdlock(&table->lock);
    Field field = resolve_path(table, path, version);
    if (!field) {
        pthread_rwlock_unlock(&table->lock);
        return NULL;
    }
    int deleted = field_delete(field, global_version);
    pthread_rwlock_unlock(&table->lock);
    return value;
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
