#include "visualiser.h"
#include <stdio.h>

static void print_indent(int level) {
    for (int i = 0; i < level; i++) printf("  ");
}

static void print_version_chain(VersionNode v) {
    for (VersionNode node = v; node; node = node->prev) {
        if (!node) continue;
        if (node->value == DELETED) {
            printf("[deleted]");
        } else if (node->free_value == free) { // string
            printf("\"%s\"", (char*)node->value);
        } else { // subdocument
            printf("{...}");
        }
        if (node->prev) printf(" -> ");
    }
}

static void print_document(Document doc, int indent) {
    if (!doc) return;

    print_indent(indent);
    printf("Fields:\n");
    for (uint64_t i = 0; i < doc->fields->bucket_count; i++) {
        for (Entry e = doc->fields->buckets[i]; e; e = e->next) {
            print_indent(indent + 1);
            printf("%s: ", e->key);
            print_version_chain((VersionNode)e->value);
            printf("\n");
        }
    }

    print_indent(indent);
    printf("Subdocuments:\n");
    for (uint64_t i = 0; i < doc->subdocuments->bucket_count; i++) {
        for (Entry e = doc->subdocuments->buckets[i]; e; e = e->next) {
            print_indent(indent + 1);
            printf("%s:\n", e->key);
            VersionNode chain = (VersionNode)e->value;
            for (VersionNode node = chain; node; node = node->prev) {
                if (node->value && node->free_value != free) { // subdocument
                    print_document((Document)node->value, indent + 2);
                }
            }
        }
    }
}

void visualize_db(VersionNode root) {
    printf("Database:\n");
    for (VersionNode v = root; v; v = v->prev) {
        Document doc = (Document)v->value;
        print_document(doc, 1);
        printf("------\n");
    }
}
