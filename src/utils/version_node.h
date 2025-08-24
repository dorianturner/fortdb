#ifndef VERSION_NODE_H
#define VERSION_NODE_H

#include <stdint.h>
#include <stdlib.h>

typedef struct VersionNode *VersionNode;

//value should be data or pointer
struct VersionNode {
    void *value;
    uint64_t global_version;
    uint64_t local_version;
    VersionNode prev;
    void (*free_value)(void *);
};

VersionNode version_node_create(void *value, 
        uint64_t global_version, 
        uint64_t local_version, 
        VersionNode prev, 
        void (*free_value)(void *));

void version_node_free(VersionNode head);
void version_node_compact(VersionNode head);
#endif
