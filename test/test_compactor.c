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

    for (uint64_t i = 0; i < doc->fields->bucket_count; i++) {
        Entry e = doc->fields->buckets[i];
        while (e) {
            char buf[256];
            snprintf(buf, sizeof(buf), "%s/%s", prefix, e->key);
            print_chain_lengths(buf, (VersionNode)e->value);
            e = e->next;
        }
    }

    for (uint64_t i = 0; i < doc->subdocuments->bucket_count; i++) {
        Entry e = doc->subdocuments->buckets[i];
        while (e) {
            char buf[256];
            snprintf(buf, sizeof(buf), "%s/%s", prefix, e->key);
            Document subdoc = (Document)((VersionNode)e->value)->value;
            print_doc_chains(subdoc, buf);
            e = e->next;
        }
    }
}

int main(void) {
    Document root = document_create();

    // Create fields with multiple versions
    document_set_field(root, "Name", "Alice", 1);
    document_set_field(root, "Name", "Alicia", 2);
    document_set_field(root, "Age", "30", 1);
    document_set_field(root, "Age", "31", 2);

    // Nested subdocument
    Document address = document_create();
    document_set_subdocument(root, "Address", address, 1);
    document_set_field(address, "City", "Paris", 1);
    document_set_field(address, "City", "Lyon", 2);

    printf("Before compaction:\n");
    print_doc_chains(root, "root");

    // Compact starting at root vnode
    compactor_compact((VersionNode)root);

    printf("\nAfter compaction:\n");
    print_doc_chains(root, "root");

    document_free(root);
    return 0;
}
