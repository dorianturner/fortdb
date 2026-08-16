#include "compactor.h"
#include "../utils/document.h"
#include "../utils/version_node.h"
#include "../utils/hash.h"

/* The caller must hold doc->lock for writing. Every version chain and map
 * entry below is stable for the complete traversal. */
static int compact_document_locked(Document doc) {
    if (!doc) return 1;

    for (uint64_t i = 0; i < doc->fields->bucket_count; i++) {
        for (Entry e = doc->fields->buckets[i]; e; e = e->next) {
            if (version_node_compact_locked((VersionNode)e->value) != 0) return 1;
        }
    }

    for (uint64_t i = 0; i < doc->subdocuments->bucket_count; i++) {
        for (Entry e = doc->subdocuments->buckets[i]; e; e = e->next) {
            VersionNode chain = (VersionNode)e->value;
            if (version_node_compact_locked(chain) != 0) return 1;

            Document child = chain ? (Document)chain->value : NULL;
            if (!child) continue;
            if (pthread_rwlock_wrlock(&child->lock) != 0) return 1;
            int ret = compact_document_locked(child);
            pthread_rwlock_unlock(&child->lock);
            if (ret != 0) return ret;
        }
    }
    return 0;
}

int compactor_compact(VersionNode root) {
    if (!root) return 1;

    if (pthread_rwlock_wrlock(&root->lock) != 0) return 1;

    Document doc = (Document)root->value;
    if (!doc || pthread_rwlock_wrlock(&doc->lock) != 0) {
        pthread_rwlock_unlock(&root->lock);
        return 1;
    }

    int ret = version_node_compact_locked(root);
    if (ret == 0) ret = compact_document_locked(doc);
    pthread_rwlock_unlock(&doc->lock);
    pthread_rwlock_unlock(&root->lock);
    return ret;
}

int compactor_compact_path(VersionNode root, const char *path) {
    if (!root || !path) return 1;

    if (pthread_rwlock_rdlock(&root->lock) != 0) return 1;

    Document doc = (Document)root->value;
    Document parent = NULL;
    char *key = NULL;
    if (!doc || resolve_parent_and_key(doc, path, &parent, &key, 0, 0) != 0) {
        pthread_rwlock_unlock(&root->lock);
        return 1;
    }

    if (pthread_rwlock_wrlock(&parent->lock) != 0) {
        document_free(parent);
        free(key);
        pthread_rwlock_unlock(&root->lock);
        return 1;
    }

    Entry field = hashmap_find_entry(parent->fields, key);
    Entry sub = hashmap_find_entry(parent->subdocuments, key);
    VersionNode chain = field ? (VersionNode)field->value :
                         (sub ? (VersionNode)sub->value : NULL);
    int ret = chain ? version_node_compact_locked(chain) : 1;

    if (ret == 0 && sub && chain && chain->value) {
        Document child = (Document)chain->value;
        if (pthread_rwlock_wrlock(&child->lock) != 0) {
            ret = 1;
        } else {
            ret = compact_document_locked(child);
            pthread_rwlock_unlock(&child->lock);
        }
    }

    pthread_rwlock_unlock(&parent->lock);
    document_free(parent);
    free(key);
    pthread_rwlock_unlock(&root->lock);
    return ret;
}
