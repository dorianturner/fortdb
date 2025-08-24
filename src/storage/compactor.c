#include "compactor.h"
#include "version_node.h"
#include "document.h"
#include "hash.h"
#include <stdint.h>
#include <stdlib.h>

/* Forward declaration */
static void compact_document(Document doc);

/* Compact a single VersionNode chain */
static void compact_node(VersionNode node) {
    if (!node) return;
    version_node_compact(node); // only frees previous versions
    // If this node contains a subdocument, recurse into it
    if (node->value) {
        Document doc = (Document)node->value;
        compact_document(doc);
    }
}

/* Compact all fields and subdocuments in a Document */
static void compact_document(Document doc) {
    if (!doc) return;

    // Compact all fields
    for (uint64_t i = 0; i < doc->fields->bucket_count; i++) {
        Entry e = doc->fields->buckets[i];
        while (e) {
            VersionNode vnode = (VersionNode)e->value;
            compact_node(vnode);
            e = e->next;
        }
    }

    // Compact all subdocuments
    for (uint64_t i = 0; i < doc->subdocuments->bucket_count; i++) {
        Entry e = doc->subdocuments->buckets[i];
        while (e) {
            VersionNode vnode = (VersionNode)e->value;
            if (vnode && vnode->value) {
                Document subdoc = (Document)vnode->value;
                compact_document(subdoc);
            }
            e = e->next;
        }
    }
}

/* Public API: start compaction from a top-level VersionNode */
void compactor_compact(VersionNode root) {
    if (!root) return;

    // Compact this version node
    compact_node(root);

    // Recurse into previous nodes in the chain
    if (root->prev) {
        compactor_compact(root->prev);
    }
}
