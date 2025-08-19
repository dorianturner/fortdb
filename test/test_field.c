#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>


#include "field.h"


/* portable strdup replacement */
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


    Field f = field_create("test-field");
    assert(f && "field_create returned NULL");


    /* first value -> local_version 0 */
    struct payload *pl1 = malloc(sizeof(*pl1));
    assert(pl1);
    pl1->data = my_strdup("one");
    pl1->free_counter = &freed;
    assert(field_set(f, pl1, 1, free_payload) == 0);


    /* second value -> local_version 1 */
    struct payload *pl2 = malloc(sizeof(*pl2));
    assert(pl2);
    pl2->data = my_strdup("two");
    pl2->free_counter = &freed;
    assert(field_set(f, pl2, 2, free_payload) == 0);


    /* delete -> local_version 2, uses DELETED marker */
    assert(field_delete(f, 3) == 0);


    /* latest (local_version 2) should behave as deleted (NULL) */
    void *d = field_get(f, 2);
    assert(d == NULL && "Expected deleted at local_version 2");


    /* older versions still accessible */
    void *v1 = field_get(f, 0);
    assert(v1 == pl1 && "Expected pl1 at local_version 0");


    void *v2 = field_get(f, 1);
    assert(v2 == pl2 && "Expected pl2 at local_version 1");


    /* free the field -> should free pl1 and pl2 via their free_payload */
    field_free(f);
    assert(freed == 2 && "Expected free_payload to be called twice");


    printf("All field tests passed.\n");
    return 0;
}
