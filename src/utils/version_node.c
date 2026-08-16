#include "version_node.h"
#include <stdlib.h>
#include <stdint.h>
#include "document.h"
VersionNode version_node_create(void *value, uint64_t global_version, uint64_t local_version, VersionNode prev, void (*free_value)(void *)) {
    VersionNode node = malloc(sizeof(struct VersionNode));
    if (!node) return NULL;

    node->value = value;
    node->global_version = global_version;
    node->local_version = local_version;
    node->prev = prev;
    node->free_value = free_value;
    if (pthread_rwlock_init(&node->lock, NULL) != 0) {
        free(node);
        return NULL;
    }
    if (pthread_mutex_init(&node->ref_lock, NULL) != 0) {
        pthread_rwlock_destroy(&node->lock);
        free(node);
        return NULL;
    }
    node->references = 1;

    return node;
}

VersionNode version_node_retain(VersionNode node) {
    if (!node) return NULL;
    if (pthread_mutex_lock(&node->ref_lock) != 0) return NULL;
    if (node->references == 0) {
        pthread_mutex_unlock(&node->ref_lock);
        return NULL;
    }
    node->references++;
    pthread_mutex_unlock(&node->ref_lock);
    return node;
}

void version_node_release(VersionNode node) {
    if (!node) return;
    if (pthread_mutex_lock(&node->ref_lock) != 0) return;
    if (node->references == 0) {
        pthread_mutex_unlock(&node->ref_lock);
        return;
    }
    node->references--;
    if (node->references != 0) {
        pthread_mutex_unlock(&node->ref_lock);
        return;
    }

    VersionNode prev = node->prev;
    void *value = node->value;
    void (*free_value)(void *) = node->free_value;
    node->prev = NULL;
    pthread_mutex_unlock(&node->ref_lock);

    if (free_value && value) free_value(value);
    pthread_mutex_destroy(&node->ref_lock);
    pthread_rwlock_destroy(&node->lock);
    free(node);
    version_node_release(prev);
}

void version_node_free(VersionNode head){
    version_node_release(head);
}

int version_node_compact_locked(VersionNode head) {
    if (!head) return(1);
    if (head->prev) {
        VersionNode old_chain = head->prev;
        head->prev = NULL;
        version_node_free(old_chain);
    }
    return 0;
}

int version_node_compact(VersionNode head) {
    if (!head || pthread_rwlock_wrlock(&head->lock) != 0) return 1;
    int ret = version_node_compact_locked(head);
    pthread_rwlock_unlock(&head->lock);
    return ret;
}

VersionNode find_version_node_by_path(VersionNode root, const char *path) {
    if (!root || !path) return NULL;

    if (pthread_rwlock_rdlock(&root->lock) != 0) return NULL;
    VersionNode result = NULL;

    // Use document API: root->value should be a Document
    Document doc = (Document)root->value;
    if (!doc) goto done;

    char *final_value = document_get_path(doc, path, UINT64_MAX);
    if (!final_value) goto done;
    if (final_value != (char *)DELETED) free(final_value);

    // Now walk the field chain to find the VersionNode corresponding to final_value
    Document parent = NULL;
    char *final_key = NULL;
    if (resolve_parent_and_key(doc, path, &parent, &final_key, 0, 0) != 0) goto done;
    if (!parent) { free(final_key); goto done; }

    if (pthread_rwlock_rdlock(&parent->lock) != 0) {
        document_free(parent);
        free(final_key);
        goto done;
    }
    Entry e = hashmap_find_entry(parent->fields, final_key);
    result = e ? version_node_retain((VersionNode)e->value) : NULL;
    pthread_rwlock_unlock(&parent->lock);
    free(final_key);
    document_free(parent);

done:
    pthread_rwlock_unlock(&root->lock);
    return result; // release with version_node_free when finished
}
