#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>  // For UINT64_MAX
#include <pthread.h> // Needed for pthread_rwlock_t in document.h

#include "../src/storage/deserializer.h"
#include "../src/utils/document.h"
#include "../src/utils/version_node.h"

#define TEST_FILE "test.db"

void print_test_result(const char *name, int passed) {
    printf("%s: %s\n", name, passed ? "PASSED" : "FAILED");
}

int main() {
    // Deserialize the previously serialized DB
    VersionNode deserialized_root = NULL;
    if (deserialize_db(TEST_FILE, &deserialized_root) != 0) {
        fprintf(stderr, "Deserialization failed\n");
        return 1;
    }
    print_test_result("Deserialization", 1);

    Document deserialized_doc = (Document)deserialized_root->value;

    // Test gets after deserialize
    char *val = document_get_field(deserialized_doc, "field1", UINT64_MAX);
    print_test_result("Post-deserialize get latest field1", val && strcmp(val, "value1_v3") == 0);

    val = document_get_field(deserialized_doc, "field1", 1);
    print_test_result("Post-deserialize get version 1 field1", val && strcmp(val, "value1_v1") == 0);

    Document got_sub = document_get_subdocument(deserialized_doc, "subdoc", 0);
    print_test_result("Post-deserialize get subdoc", got_sub != NULL);

    val = document_get_field(deserialized_doc, "field2", UINT64_MAX);
    print_test_result("Post-deserialize get deleted field2", val == DELETED);

    val = document_get_field(deserialized_doc, "subdoc/subfield_path", UINT64_MAX);
    print_test_result("Post-deserialize get path field", val && strcmp(val, "pathvalue") == 0);

    printf("\nPost-deserialize versions for field1:\n");
    document_list_versions(deserialized_doc, "field1");

    version_node_free(deserialized_root);

    return 0;
}
