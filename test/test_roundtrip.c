#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>  // For UINT64_MAX

#include "../src/storage/serializer.h"
#include "../src/storage/deserializer.h"
#include "../src/utils/document.h"
#include "../src/utils/version_node.h"

#define TEST_FILE "test.db"

void print_test_result(const char *name, int passed) {
    printf("%s: %s\n", name, passed ? "PASSED" : "FAILED");
}

int main() {
    // Create root document
    Document root_doc = document_create();
    if (!root_doc) {
        fprintf(stderr, "Failed to create root document\n");
        return 1;
    }

    // Add field versions in the same document
    document_set_field(root_doc, "field1", "value1_v1", 1);
    document_set_field(root_doc, "field1", "value1_v2", 2);
    document_set_field(root_doc, "field1", "value1_v3", 4); // latest
    document_set_field(root_doc, "field2", "value2", 1);

    // Create subdocument
    Document sub_doc = document_create();
    document_set_field(sub_doc, "subfield", "subvalue", 1);
    document_set_subdocument(root_doc, "subdoc", sub_doc, 1);

    // Set a path field inside subdoc
    document_set_field_path(root_doc, "subdoc/subfield_path", "pathvalue", 2);

    // Delete a field
    document_delete_path(root_doc, "field2", 3);

    // Wrap in a VersionNode (single global version for root)
    VersionNode root = version_node_create(root_doc, 1, 1, NULL, (void(*)(void *))document_free);
    if (!root) {
        fprintf(stderr, "Failed to create root VersionNode\n");
        document_free(root_doc);
        return 1;
    }

    // Test gets before serialize
    char *val = document_get_field(root_doc, "field1", UINT64_MAX);  // Latest
    print_test_result("Pre-serialize get latest field1", val && strcmp(val, "value1_v3") == 0);

    val = document_get_field(root_doc, "field1", 1);  // Version 1
    print_test_result("Pre-serialize get version 1 field1", val && strcmp(val, "value1_v1") == 0);

    Document got_sub = document_get_subdocument(root_doc, "subdoc", 0);
    print_test_result("Pre-serialize get subdoc", got_sub != NULL);

    val = document_get_field(root_doc, "field2", UINT64_MAX);
    print_test_result("Pre-serialize get deleted field2", val == DELETED);

    printf("\nPre-serialize versions for field1:\n");
    document_list_versions(root_doc, "field1");

    // Serialize
    if (serialize_db(root, TEST_FILE) != 0) {
        fprintf(stderr, "Serialization failed\n");
        version_node_free(root);
        return 1;
    }
    print_test_result("Serialization", 1);

    version_node_free(root);

    // Deserialize
    VersionNode deserialized_root = NULL;
    if (deserialize_db(TEST_FILE, &deserialized_root) != 0) {
        fprintf(stderr, "Deserialization failed\n");
        return 1;
    }
    print_test_result("Deserialization", 1);

    Document deserialized_doc = (Document)deserialized_root->value;

    // Test gets after deserialize
    val = document_get_field(deserialized_doc, "field1", UINT64_MAX);
    print_test_result("Post-deserialize get latest field1", val && strcmp(val, "value1_v3") == 0);

    val = document_get_field(deserialized_doc, "field1", 1);
    print_test_result("Post-deserialize get version 1 field1", val && strcmp(val, "value1_v1") == 0);

    got_sub = document_get_subdocument(deserialized_doc, "subdoc", 0);
    print_test_result("Post-deserialize get subdoc", got_sub != NULL);

    val = document_get_field(deserialized_doc, "field2", UINT64_MAX);
    print_test_result("Post-deserialize get deleted field2", val == DELETED);

    val = document_get_field(deserialized_doc, "subdoc/subfield_path", UINT64_MAX);
    print_test_result("Post-deserialize get path field", val && strcmp(val, "pathvalue") == 0);

    // List versions for field1
    printf("\nPost-deserialize versions for field1:\n");
    document_list_versions(deserialized_doc, "field1");

    version_node_free(deserialized_root);
    remove(TEST_FILE);

    return 0;
}
