#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "collection.h"
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

    /* Create a document containing a field with a payload */
    Document d1 = document_create();
    assert(d1);
    Field f = field_create("f");
    assert(f);
    struct payload *pl = malloc(sizeof(*pl));
    pl->data = my_strdup("payload");
    pl->free_counter = &freed;
    assert(field_set(f, pl, 1, free_payload) == 0);
    assert(document_set_field(d1, "a", f, 50) == 0);

    /* Create collection and put d1 into it -> local_version 0 */
    Collection coll = collection_create();
    assert(coll);
    assert(collection_set_document(coll, "doc", d1, 100) == 0);

    Document got0 = collection_get_document(coll, "doc", 0);
    assert(got0 == d1);

    /* Replace with a new document d2 -> local_version 1 */
    Document d2 = document_create();
    assert(d2);
    Field f2 = field_create("f2");
    assert(f2);
    struct payload *pl2 = malloc(sizeof(*pl2));
    pl2->data = my_strdup("p2");
    pl2->free_counter = &freed;
    assert(field_set(f2, pl2, 2, free_payload) == 0);
    assert(document_set_field(d2, "b", f2, 60) == 0);

    assert(collection_set_document(coll, "doc", d2, 101) == 0);

    /* Testing versioned gets of documents */
    Document got1 = collection_get_document(coll, "doc", 1);
    Document got0_again = collection_get_document(coll, "doc", 0);
    assert(got1 == d2);
    assert(got0_again == d1);

    /* Free collection -> should free documents and all inner fields -> freed should be 2 */
    collection_free(coll);
    assert(freed == 2 && "Expected both payloads to be freed via document_free");

    printf("All collection tests passed.\n");
    return 0;
}

