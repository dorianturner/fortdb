#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "table.h"

int main(void) {
    Table t = table_create();
    assert(t && "table_create failed");

    /* ---------------- Basic set/get ---------------- */
    assert(table_set_field(t, "users/alice/profile/name", "Alice", 1) == 0);
    assert(table_set_field(t, "users/alice/profile/age", "30", 2) == 0);

    // First writes are always local version 0
    char *name = table_get_field(t, "users/alice/profile/name", 0);
    char *age  = table_get_field(t, "users/alice/profile/age", 0);
    assert(name && strcmp(name, "Alice") == 0);
    assert(age  && strcmp(age, "30") == 0);

    printf("Basic set/get passed.\n");

    /* ---------------- Versioning ---------------- */
    assert(table_set_field(t, "users/alice/profile/name", "Alice V2", 3) == 0);

    // Local version 0 -> "Alice", local version 1 -> "Alice V2"
    char *name_v0 = table_get_field(t, "users/alice/profile/name", 0);
    char *name_v1 = table_get_field(t, "users/alice/profile/name", 1);
    assert(name_v0 && strcmp(name_v0, "Alice") == 0);
    assert(name_v1 && strcmp(name_v1, "Alice V2") == 0);

    printf("Versioned set/get passed.\n");

    /* ---------------- Deletion ---------------- */
    assert(table_delete_path(t, "users/alice/profile/age", 4) == 0);
    char *age_after_delete = table_get_field(t, "users/alice/profile/age", 1);
    assert(age_after_delete == NULL);

    printf("Delete path passed.\n");

    /* ---------------- Nested deeper ---------------- */
    assert(table_set_field(t, "root/doc1/sub1/doc2/fieldX", "nested", 5) == 0);
    char *nested_val = table_get_field(t, "root/doc1/sub1/doc2/fieldX", 0);
    assert(nested_val && strcmp(nested_val, "nested") == 0);

    printf("Nested path passed.\n");

    table_free(t);
    printf("All table tests passed.\n");
    return 0;
}

