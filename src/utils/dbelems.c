#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include "dbelems.h"

static VersionNode version_node_new(void *value, uint64_t global_version, uint64_t local_version) {
    VersionNode node = malloc(sizeof(struct VersionNode));
    if (!node) return NULL;
    node->value = value;
    node->global_version = global_version;
    node->local_version = local_version;
    node->prev = NULL;
    return node;
}

int cell_put(Cell cell, void *value, uint64_t global_version) {
    if (!cell || !key || !value) return -1;

    VersionNode node = version_node_new(value, global_version, cell->size + 1);
    if (!node) return -1;

    if (!cell->head) {
        cell->head = cell->tail = node;
    } else {
        node->prev = cell->tail;
        cell->tail = node;
    }
    cell->size++;
    return 0;
}

// Fast access for most recent but slow traversal for versioned values
void *cell_get(Cell cell, uint64_t local_version) {
    if (!cell) return NULL;

    VersionNode current = cell->tail;
    while (current) {
        if (current->local_version <= local_version) {
            return current->value;
        }
        current = current->prev;
    }
    return NULL;
}

void free_version_node(VersionNode node) {
    while (node) {
        VersionNode prev = node->prev;
        free(node->value);
        free(node);
        node = prev;
    }
}

void free_cell(Cell cell) {
    if (!cell) return;
    free_version_node(cell->tail);
    free(cell);
}

