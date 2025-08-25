#include "compactor.h"
#include "../utils/document.h"
#include "../utils/version_node.h"
#include "../utils/hash.h"

// Recursively compact a document
static int compact_document(Document doc) {
    if (!doc) return 1;  // fail if doc is NULL

    // Compact all fields
    for (uint64_t i = 0; i < doc->fields->bucket_count; i++) {
        Entry e = doc->fields->buckets[i];
        while (e) {
            VersionNode v = (VersionNode)e->value;

            int ret = version_node_compact(v);
            if (ret != 0) return 1;  // fail if compaction fails

            e = e->next;
        }
    }

    // Compact all subdocuments recursively
    for (uint64_t i = 0; i < doc->subdocuments->bucket_count; i++) {
        Entry e = doc->subdocuments->buckets[i];
        while (e) {
            VersionNode v = (VersionNode)e->value;

            int ret = version_node_compact(v);
            if (ret != 0) return 1;  // fail if compaction fails

            Document subdoc = (Document)v->value;
            if (subdoc) {
                ret = compact_document(subdoc);
                if (ret != 0) return 1;  // fail if recursive compaction fails
            }

            e = e->next;
        }
    }

    return 0;  // success
}


// Compact starting at a root VersionNode
int compactor_compact(VersionNode root) {
    if (!root) return 1;  // fail if root is NULL

    // Compact the root itself
    int ret = version_node_compact(root);
    if (ret != 0) return 1; // fail if version_node_compact fails

    // If root points to a Document, compact all its fields/subdocs
    Document doc = (Document)root->value;
    if (doc) {
        ret = compact_document(doc);
        if (ret != 0) return 1; // fail if compact_document fails
    }

    return 0; // success
}

