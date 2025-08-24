#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>  
#include <pthread.h> 

#include "../src/storage/serializer.h"
#include "../src/utils/document.h"
#include "../src/utils/version_node.h"

#define TEST_FILE "test.fortdb"

void print_test_result(const char *name, int passed) {
    printf("%s: %s\n", name, passed ? "PASSED" : "FAILED");
}

int main() {
    // Create root document
    Document root_doc = document_create();
    if (!root_doc) return 1;

    // Root fields
    document_set_field(root_doc, "Name", "Alice", 1);
    document_set_field(root_doc, "Name", "Alicia", 2);
    document_set_field(root_doc, "Age", "30", 1);
    document_set_field(root_doc, "Age", "31", 2);
    document_set_field(root_doc, "Occupation", "Engineer", 1);
    document_set_field(root_doc, "Occupation", "Senior Engineer", 2);

    // Nested Address
    Document address = document_create();
    document_set_subdocument(root_doc, "Address", address, 1);
    document_set_field(address, "City", "Paris", 1);
    document_set_field(address, "City", "Lyon", 2);
    document_set_field(address, "Street", "1st Avenue", 1);
    document_set_field(address, "Street", "2nd Avenue", 2);

    // Nested Coordinates inside Address
    Document coords = document_create();
    document_set_subdocument(address, "Coordinates", coords, 1);
    document_set_field(coords, "Latitude", "48.8566", 1);
    document_set_field(coords, "Latitude", "45.7640", 2);
    document_set_field(coords, "Longitude", "2.3522", 1);
    document_set_field(coords, "Longitude", "4.8357", 2);

    // Nested Company subdocument
    Document company = document_create();
    document_set_subdocument(root_doc, "Company", company, 1);
    document_set_field(company, "Name", "TechCorp", 1);
    document_set_field(company, "Name", "TechCorp International", 2);
    document_set_field(company, "Founded", "2000", 1);
    document_set_field(company, "Founded", "2001", 2);

    // Even deeper subdocument: Company/Location
    Document company_location = document_create();
    document_set_subdocument(company, "Location", company_location, 1);
    document_set_field(company_location, "City", "New York", 1);
    document_set_field(company_location, "City", "San Francisco", 2);
    document_set_field(company_location, "Country", "USA", 1);
    document_set_field(company_location, "Country", "United States", 2);

    // Wrap in VersionNode
    VersionNode root = version_node_create(root_doc, 1, 1, NULL, (void(*)(void *))document_free);
    if (!root) return 1;

    // Test some fields using correct names
    char *val = document_get_field(root_doc, "Name", UINT64_MAX);
    print_test_result("Root Name latest", val && strcmp(val, "Alicia") == 0);

    val = document_get_field(address, "City", UINT64_MAX);
    print_test_result("Address City latest", val && strcmp(val, "Lyon") == 0);

    val = document_get_field(coords, "Latitude", UINT64_MAX);
    print_test_result("Coordinates Latitude latest", val && strcmp(val, "45.7640") == 0);

    val = document_get_field(company_location, "Country", UINT64_MAX);
    print_test_result("Company Location Country latest", val && strcmp(val, "United States") == 0);

    // Serialize
    if (serialize_db(root, TEST_FILE) != 0) {
        fprintf(stderr, "Serialization failed\n");
        version_node_free(root);
        return 1;
    }
    print_test_result("Serialization", 1);

    version_node_free(root);
    // remove(TEST_FILE); // Keep for inspection

    return 0;
}
