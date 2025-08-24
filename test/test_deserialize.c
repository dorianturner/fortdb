#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>

#include "../src/storage/deserializer.h"
#include "../src/utils/document.h"
#include "../src/utils/version_node.h"

#define TEST_FILE "test.fortdb"

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
    char *val = document_get_field(deserialized_doc, "Name", UINT64_MAX);
    print_test_result("Post-deserialize Name latest", val && strcmp(val, "Alicia") == 0);

    val = document_get_field(deserialized_doc, "Age", 1);
    print_test_result("Post-deserialize Age v1", val && strcmp(val, "30") == 0);

    val = document_get_field(deserialized_doc, "Occupation", UINT64_MAX);
    print_test_result("Post-deserialize Occupation latest", val && strcmp(val, "Senior Engineer") == 0);

    Document address = document_get_subdocument(deserialized_doc, "Address", 0);
    print_test_result("Post-deserialize Address exists", address != NULL);

    val = document_get_field(address, "City", UINT64_MAX);
    print_test_result("Post-deserialize Address City latest", val && strcmp(val, "Lyon") == 0);

    Document coords = document_get_subdocument(address, "Coordinates", 0);
    print_test_result("Post-deserialize Address Coordinates exists", coords != NULL);

    val = document_get_field(coords, "Latitude", 1);
    print_test_result("Post-deserialize Latitude v1", val && strcmp(val, "48.8566") == 0);

    Document company = document_get_subdocument(deserialized_doc, "Company", 0);
    print_test_result("Post-deserialize Company exists", company != NULL);

    Document location = document_get_subdocument(company, "Location", 0);
    print_test_result("Post-deserialize Company Location exists", location != NULL);

    val = document_get_field(location, "Country", UINT64_MAX);
    print_test_result("Post-deserialize Company Location Country latest", val && strcmp(val, "United States") == 0);

    printf("\nPost-deserialize versions for Name:\n");
    document_list_versions(deserialized_doc, "Name");

    version_node_free(deserialized_root);

    return 0;
}
