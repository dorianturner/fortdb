#include "version_node.h"
#include <stdlib.h>
#include <stdint.h>

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

