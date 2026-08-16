#ifndef VERSION_NODE_H
#define VERSION_NODE_H

#include <stdint.h>
#include <stdlib.h>
#if defined(_WIN32)
#include "windows_compat.h"
#else
#include <pthread.h>
#endif

typedef struct VersionNode *VersionNode;

//value should be data or pointer
struct VersionNode {
    void *value;
    uint64_t global_version;
    uint64_t local_version;
    VersionNode prev;
    void (*free_value)(void *);
    pthread_rwlock_t lock;
    pthread_mutex_t ref_lock;
    size_t references;
};

VersionNode version_node_create(void *value, 
        uint64_t global_version, 
        uint64_t local_version, 
        VersionNode prev, 
        void (*free_value)(void *));

/* VersionNode references are required for nodes returned by lookup helpers. */
VersionNode version_node_retain(VersionNode node);
void version_node_release(VersionNode node);
void version_node_free(VersionNode head);
/* Internal callers that already hold node->lock use this variant. */
int version_node_compact_locked(VersionNode head);
int version_node_compact(VersionNode head);

VersionNode find_version_node_by_path(VersionNode root, const char *path);

#endif
