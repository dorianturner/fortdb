#include "compactor.h"
#include "../utils/document.h"
#include "../utils/version_node.h"
#include "../utils/hash.h"

// Recursively compact a document
static void compact_document(Document doc) {
    if (!doc) return;

    // Compact all fields
    for (uint64_t i = 0; i < doc->fields->bucket_count; i++) {
        Entry e = doc->fields->buckets[i];
        while (e) {
            VersionNode v = (VersionNode)e->value;
            version_node_compact(v);
            e = e->next;
        }
    }

    // Compact all subdocuments recursively
    for (uint64_t i = 0; i < doc->subdocuments->bucket_count; i++) {
        Entry e = doc->subdocuments->buckets[i];
        while (e) {
            VersionNode v = (VersionNode)e->value;
            version_node_compact(v);

            Document subdoc = (Document)v->value;
            compact_document(subdoc);

            e = e->next;
        }
    }
}

// Compact starting at a root VersionNode
void compactor_compact(VersionNode root) {
    if (!root) return;

    // Compact the root itself
    version_node_compact(root);

    // If root points to a Document, compact all its fields/subdocs
    Document doc = (Document)root->value;
    compact_document(doc);
}
