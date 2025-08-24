#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "../src/utils/document.h"
#include "../src/utils/version_node.h"
#include "../src/storage/compactor.h"

// Helper: print length of a VersionNode chain
void print_chain_lengths(const char *prefix, VersionNode node) {
    if (!node) return;
    int count = 0;
    VersionNode current = node;
    while (current) {
        count++;
        current = current->prev;
    }
    printf("%s: chain length = %d\n", prefix, count);
}

// Recursively print all fields and subdocuments chain lengths
void print_doc_chains(Document doc, const char *prefix) {
    if (!doc) return;

    // Fields
    for (uint64_t i = 0; i < doc->fields->bucket_count; i++) {
        Entry e = doc->fields->buckets[i];
        while (e) {
            char buf[256];
            snprintf(buf, sizeof(buf), "%s/%s", prefix, e->key);
            print_chain_lengths(buf, (VersionNode)e->value);
            e = e->next;
        }
    }

    // Subdocuments
    for (uint64_t i = 0; i < doc->subdocuments->bucket_count; i++) {
        Entry e = doc->subdocuments->buckets[i];
        while (e) {
            char buf[256];
            snprintf(buf, sizeof(buf), "%s/%s", prefix, e->key);
            VersionNode subdoc_node = (VersionNode)e->value;
            Document subdoc = (Document)subdoc_node->value;
            print_doc_chains(subdoc, buf);
            e = e->next;
        }
    }
}

int main(void) {
    // Create root document
    Document root_doc = document_create();

    // Add fields with multiple versions
    document_set_field(root_doc, "Name", "Alice", 1);
    document_set_field(root_doc, "Name", "Alicia", 2);
    document_set_field(root_doc, "Age", "30", 1);
    document_set_field(root_doc, "Age", "31", 2);

    // Add nested subdocument
    Document address = document_create();
    document_set_subdocument(root_doc, "Address", address, 1);
    document_set_field(address, "City", "Paris", 1);
    document_set_field(address, "City", "Lyon", 2);

    // Wrap root document in a VersionNode
    VersionNode root_vnode = version_node_create(
        root_doc,               // value points to the Document
        1,                      // global_version
        1,                      // local_version
        NULL,                   // no previous version
        (void(*)(void *))document_free // free_value cleans up Document
    );

    printf("Before compaction:\n");
    print_doc_chains((Document)root_vnode->value, "root");

    // Compact starting at the root vnode
    compactor_compact(root_vnode);

    printf("\nAfter compaction:\n");
    print_doc_chains((Document)root_vnode->value, "root");

    // Free everything
    version_node_free(root_vnode);

    return 0;
}
