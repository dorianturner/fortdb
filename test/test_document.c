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

    Document doc = document_create();
    assert(doc && "document_create failed");

    /* ---------------- Field tests ---------------- */
    Field f1 = field_create("f1");
    assert(f1);
    struct payload *pl1 = malloc(sizeof(*pl1));
    pl1->data = my_strdup("one");
    pl1->free_counter = &freed;
    assert(field_set(f1, pl1, 10, free_payload) == 0);

    assert(document_set_field(doc, "field", f1, 100) == 0);

    Field got0 = document_get_field(doc, "field", 0);
    assert(got0 == f1);

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

    /* ---------------- Subdocument tests ---------------- */
    Document sub1 = document_create();
    assert(sub1);
    assert(document_set_subdocument(doc, "sub", sub1, 200) == 0);

    Document got_sub0 = document_get_subdocument(doc, "sub", 0);
    assert(got_sub0 == sub1);

    Document sub2 = document_create();
    assert(sub2);
    assert(document_set_subdocument(doc, "sub", sub2, 201) == 0);

    Document got_sub1 = document_get_subdocument(doc, "sub", 1);
    Document got_sub0_again = document_get_subdocument(doc, "sub", 0);
    assert(got_sub1 == sub2);
    assert(got_sub0_again == sub1);

    /* ---------------- Free everything ---------------- */
    document_free(doc);

    /* Both fields freed their payloads */
    assert(freed == 2 && "Expected two payload frees (one per field)");

    printf("All tests passed.\n");
    return 0;
}

