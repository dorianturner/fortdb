#ifndef COMPACTOR_H
#define COMPACTOR_H

#include "version_node.h"

// Recursively compact the entire database tree,
// starting at the given root VersionNode
int compactor_compact(VersionNode root);
int compactor_compact_path(VersionNode root, const char *path);

#endif
