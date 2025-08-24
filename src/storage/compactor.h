#ifndef COMPACTOR_H
#define COMPACTOR_H

#include "version_node.h"

// Recursively compact the entire database tree,
// starting at the given root VersionNode
void compactor_compact(VersionNode root);

#endif
