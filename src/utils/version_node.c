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

    return node;
}

void version_node_free(VersionNode head){
    VersionNode current = head;
    while (current) {
        VersionNode next = current->prev;
        if (current->free_value && current->value)
            current->free_value(current->value);
        free(current);
        current = next;
    }
}

int version_node_compact(VersionNode head) {
    if (!head) return(1);
    if (head->prev) {
        version_node_free(head->prev);
        head->prev = NULL;
    }return(0);}

VersionNode find_version_node_by_path(VersionNode root, const char *path) {
    if (!root || !path) return NULL;

    // Use document API: root->value should be a Document
    Document doc = (Document)root->value;
    if (!doc) return NULL;

    char *final_value = document_get_path(doc, path, UINT64_MAX);
    if (!final_value) return NULL;

    // Now walk the field chain to find the VersionNode corresponding to final_value
    Document parent = NULL;
    char *final_key = NULL;
    if (resolve_parent_and_key(doc, path, &parent, &final_key, 0, 0) != 0) return NULL;
    if (!parent) { free(final_key); return NULL; }

    Entry e = hashmap_find_entry(parent->fields, final_key);
    free(final_key);
    if (!e) return NULL;

    return (VersionNode)e->value; // return the head of the VersionNode chain
}
