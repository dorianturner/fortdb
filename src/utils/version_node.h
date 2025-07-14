#ifndef VERSION_NODE_H
#define VERSION_NODE_H

#include <stdint.h>
#include <stdlib.h>

typedef struct VersionNode *VersionNode;

struct VersionNode {
    void *value;
    uint64_t global_version;
    uint64_t local_version;
    VersionNode prev;
};

VersionNode version_node_create(void *value, uint64_t global_version, uint64_t local_version, VersionNode prev);
void version_node_free(VersionNode head, void (*free_value)(void *));

#endif

