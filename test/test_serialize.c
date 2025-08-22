#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>  
#include <pthread.h> 

#include "../src/storage/serializer.h"
#include "../src/utils/document.h"
#include "../src/utils/version_node.h"

#define TEST_FILE "test_human.db"

void print_test_result(const char *name, int passed) {
    printf("%s: %s\n", name, passed ? "PASSED" : "FAILED");
}

int main() {
    // Create root document
    Document root_doc = document_create();
    if (!root_doc) return 1;

    // Root-level fields with versions
    document_set_field(root_doc, "name", "Alice Smith", 1);
    document_set_field(root_doc, "name", "Alice S.", 2);  // short version
    document_set_field(root_doc, "email", "alice@example.com", 1);

    // Create subdoc1 for work info
    Document work_doc = document_create();
    document_set_field(work_doc, "company", "TechCorp", 1);
    document_set_field(work_doc, "title", "Software Engineer", 1);
    document_set_field(work_doc, "title", "Senior Software Engineer", 2);

    // Create subsubdoc1 for office location inside work
    Document office_doc = document_create();
    document_set_field(office_doc, "building", "Main HQ", 1);
    document_set_field(office_doc, "floor", "5", 1);
    document_set_field(office_doc, "desk", "5B", 1);
    document_set_subdocument(work_doc, "office", office_doc, 1);

    // Attach work_doc to root as subdoc1
    document_set_subdocument(root_doc, "work_info", work_doc, 1);

    // Create subdoc2 for personal contacts
    Document contacts_doc = document_create();
    document_set_field(contacts_doc, "phone_mobile", "+1234567890", 1);
    document_set_field(contacts_doc, "phone_home", "+0987654321", 1);

    // Attach contacts_doc to root as subdoc2
    document_set_subdocument(root_doc, "contacts", contacts_doc, 1);

    // Optional: add a path field inside office_doc
    document_set_field_path(root_doc, "work_info/office/parking_spot", "P12", 1);

    // Wrap in VersionNode
    VersionNode root = version_node_create(root_doc, 1, 1, NULL, (void(*)(void *))document_free);
    if (!root) return 1;

    // Test some fields
    char *val = document_get_field(root_doc, "name", UINT64_MAX);
    print_test_result("Root name latest", val && strcmp(val, "Alice S.") == 0);

    Document got_work = document_get_subdocument(root_doc, "work_info", 0);
    print_test_result("Get work_info", got_work != NULL);

    Document got_office = document_get_subdocument(root_doc, "work_info/office", 0);
    print_test_result("Get office subdocument", got_office != NULL);

    val = document_get_field(root_doc, "work_info/office/parking_spot", UINT64_MAX);
    print_test_result("Parking spot field", val && strcmp(val, "P12") == 0);

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
