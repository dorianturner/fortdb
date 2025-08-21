#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/utils/document.h"
#include "../src/utils/hash.h"
#include "../src/utils/version_node.h"
#include "../src/storage/deserializer.h"
#include "../src/storage/serializer.h"

int main(void) {
    // 1. Create a root document
    Document doc = document_create();
    if (!doc) { printf("Failed to create document\n"); return 1; }

    // 2. Set some fields
    document_set_field_cstr(doc, "name", "Alice", 1);
    document_set_field_cstr(doc, "age", "30", 2);

    // 3. Add a subdocument
    Document subdoc = document_create();
    document_set_field_cstr(subdoc, "city", "Paris", 1);
    document_set_subdocument(doc, "address", subdoc, 3);

    // 4. Serialize DB (root VersionNode)
    VersionNode root = version_node_create(doc, 1, 1, NULL, NULL);
    if (serialize_db(root, "test.db") != 0) {
        printf("Serialization failed\n");
        return 1;
    }

    // 5. Deserialize DB
    VersionNode new_root = NULL;
    if (deserialize_db("test.db", &new_root) != 0) {
        printf("Deserialization failed\n");
        return 1;
    }

    // 6. Check fields
    Document new_doc = (Document)new_root->value;
    char *name = document_get_field(new_doc, "name", 0);
    char *age  = document_get_field(new_doc, "age", 0);
    Document new_sub = document_get_subdocument(new_doc, "address", 0);
    char *city = document_get_field(new_sub, "city", 0);

    printf("Name: %s\n", name);
    printf("Age: %s\n", age);
    printf("City: %s\n", city);

    // Cleanup
    version_node_free(new_root);
    version_node_free(root);

    return 0;
}
