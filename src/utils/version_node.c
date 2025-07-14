#include "version_node.h"
#include <stdlib.h>
#include <stdint.h>

VersionNode version_node_create(void *value, uint64_t global_version, uint64_t local_version, VersionNode prev) {
    VersionNode node = malloc(sizeof(struct VersionNode));
    if (!node) return NULL;

    node->value = value;
    node->global_version = global_version;
    node->local_version = local_version;
    node->prev = prev;

    return node;
}

void version_node_free(VersionNode head, void (*free_value)(void *)) {
    VersionNode current = head;
    while (current) {
        VersionNode next = current->prev;
        if (free_value && current->value)
            free_value(current->value);
        free(current);
        current = next;
    }
}

