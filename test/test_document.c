#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "document.h"
#include "field.h"

/* tiny strdup replacement */
static char *my_strdup(const char *s) {
    size_t n = strlen(s) + 1;
    char *p = malloc(n);
    if (p) memcpy(p, s, n);
    return p;
}

struct payload {
    char *data;
    int *free_counter;
};

static void free_payload(void *v) {
    if (!v) return;
    struct payload *p = v;
    if (p->free_counter) (*(p->free_counter))++;
    free(p->data);
    free(p);
}

int main(void) {
    int freed = 0;

    /* ---------------- create ---------------- */
    Document doc = document_create();
    assert(doc && "document_create failed");

    /* ---------------- Field: create + set via Field object ---------------- */
    Field f1 = field_create("f1");
    assert(f1);
    struct payload *pl1 = malloc(sizeof(*pl1));
    pl1->data = my_strdup("one");
    pl1->free_counter = &freed;
    assert(field_set(f1, pl1, 10, free_payload) == 0);

    /* insert into document */
    assert(document_set_field(doc, "field", f1, 100) == 0);

    /* immediate lookup */
    Field got0 = document_get_field(doc, "field", 0);
    assert(got0 == f1);

    /* another Field object (new version) */
    Field f2 = field_create("f2");
    assert(f2);
    struct payload *pl2 = malloc(sizeof(*pl2));
    pl2->data = my_strdup("two");
    pl2->free_counter = &freed;
    assert(field_set(f2, pl2, 11, free_payload) == 0);

    assert(document_set_field(doc, "field", f2, 101) == 0);

    Field got1 = document_get_field(doc, "field", 1);
    Field got0_again = document_get_field(doc, "field", 0);
    assert(got1 == f2);
    assert(got0_again == f1);

    /* ---------------- Convenience: set C-string directly ---------------- */
    assert(document_set_field_cstr(doc, "greeting", "hello world", 200) == 0);
    Field fg = document_get_field(doc, "greeting", 0);
    assert(fg != NULL);

    /* NOTE: field_get expects a local_version (index). This field has only one version (local 0). */
    void *val_g = field_get(fg, 0);
    assert(val_g && strcmp((char *)val_g, "hello world") == 0);

    /* ---------------- Path-aware setter: creates intermediate subdocuments ---------------- */
    assert(document_set_field_path(doc, "users/alice/age", "30", 300) == 0);

    /* verify the field exists at that path and value is correct (local 0) */
    Field age = document_get_field(doc, "users/alice/age", 0);
    assert(age != NULL);
    void *age_val = field_get(age, 0);
    assert(age_val && strcmp((char *)age_val, "30") == 0);

    /* verify intermediate subdocuments were created */
    Document users = document_get_subdocument(doc, "users", 0);
    assert(users != NULL);
    Document alice = document_get_subdocument(users, "alice", 0);
    assert(alice != NULL);

    /* ---------------- Subdocument set/get (immediate) ---------------- */
    Document sub1 = document_create();
    assert(sub1);
    assert(document_set_subdocument(doc, "sub", sub1, 400) == 0);

    Document got_sub0 = document_get_subdocument(doc, "sub", 0);
    assert(got_sub0 == sub1);

    Document sub2 = document_create();
    assert(sub2);
    assert(document_set_subdocument(doc, "sub", sub2, 401) == 0);

    Document got_sub1 = document_get_subdocument(doc, "sub", 1);
    Document got_sub0_again = document_get_subdocument(doc, "sub", 0);
    assert(got_sub1 == sub2);
    assert(got_sub0_again == sub1);

    /* ---------------- Delete path (field) ---------------- */
    /* delete existing field "greeting" (adds a tombstone as the latest local version) */
    assert(document_delete_path(doc, "greeting", 500) == 0);

    /* field object should still exist; fetch latest local_version from field->versions */
    Field fg_after = document_get_field(doc, "greeting", 0);
    assert(fg_after == fg);
    uint64_t latest_local = fg_after->versions->local_version; /* access is allowed in tests */
    void *val_after = field_get(fg_after, latest_local);
    assert(val_after == NULL); /* deleted -> field_get returns NULL for tombstone version */

    /* deleting a non-existent path should be safe (return 0) */
    assert(document_delete_path(doc, "no/such/path", 501) == 0);

    /* ---------------- List versions (prints to stdout) ---------------- */
    assert(document_list_versions(doc, "field") == 0);

    /* ---------------- Compact / load / save (stubs) ---------------- */
    assert(document_compact(doc, "users/alice") == 0);
    assert(document_load(doc, "db.fort") == 0);
    assert(document_save(doc, "out.fort", "users") == 0);

    /* ---------------- cleanup & memory checks ---------------- */
    document_free(doc);

    /* payload free counter should reflect two frees from f1 and f2 */
    assert(freed == 2 && "Expected two payload frees (one per field payload)");

    printf("All document API tests passed.\n");
    return 0;
}

