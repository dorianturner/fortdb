#include "compactor.h"
#include "document.h"
#include "version_node.h"
#include "hash.h"
#include <stddef.h>

static void compact_node(VersionNode node);

// Compact all fields + subdocuments in a Document
static void compact_document(Document doc) {
    if (!doc) return;

    // Compact fields
    for (uint64_t i = 0; i < doc->fields->bucket_count; i++) {
        for (Entry e = doc->fields->buckets[i]; e; e = e->next) {
            VersionNode vn = (VersionNode)e->value;
            compact_node(vn);
        }
    }

    // Compact subdocuments
    for (uint64_t i = 0; i < doc->subdocuments->bucket_count; i++) {
        for (Entry e = doc->subdocuments->buckets[i]; e; e = e->next) {
            VersionNode vn = (VersionNode)e->value;
            compact_node(vn);

            // Recurse into the subdocument itself
            if (vn && vn->value) {
                compact_document((Document)vn->value);
            }
        }
    }
}

// Compact a single VersionNode and recurse if its value is a Document
static void compact_node(VersionNode node) {
    if (!node) return;

    // Collapse version chain (keep only newest node)
    version_node_compact(node);

    // If this VersionNode contains a Document, recurse
    if (node->value) {
        compact_document((Document)node->value);
    }
}

// Public entry point: compact the entire DB tree
void compactor_compact(VersionNode root) {
    compact_node(root);
}
