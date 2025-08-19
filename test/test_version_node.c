#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "version_node.h"

/* portable strdup replacement */
static char *my_strdup(const char *s) {
    size_t n = strlen(s) + 1;
    char *p = malloc(n);
    if (p) memcpy(p, s, n);
    return p;
}

/* payload type used to verify that free_value is called */
struct payload {
    char *data;
    int *free_counter; /* incremented by free_payload when called */
};

static void free_payload(void *v) {
    if (!v) return;
    struct payload *p = (struct payload *)v;
    if (p->free_counter) (*(p->free_counter))++;
    free(p->data);
    free(p);
}

int main(void) {
    int freed = 0;

    /* Create first payload and node */
    struct payload *pl1 = malloc(sizeof(*pl1));
    assert(pl1);
    pl1->data = my_strdup("one");
    pl1->free_counter = &freed;
    VersionNode n1 = version_node_create(pl1, 10, 0, NULL, free_payload);
    assert(n1 && "version_node_create returned NULL for n1");
    assert(n1->value == pl1);
    assert(n1->global_version == 10);
    assert(n1->local_version == 0);
    assert(n1->prev == NULL);

    /* Create second payload and node (links to n1) */
    struct payload *pl2 = malloc(sizeof(*pl2));
    assert(pl2);
    pl2->data = my_strdup("two");
    pl2->free_counter = &freed;
    VersionNode n2 = version_node_create(pl2, 11, 1, n1, free_payload);
    assert(n2 && "version_node_create returned NULL for n2");
    assert(n2->value == pl2);
    assert(n2->global_version == 11);
    assert(n2->local_version == 1);
    assert(n2->prev == n1 && "n2->prev should point to n1");

    /* Create third payload and node (link chain: n3 -> n2 -> n1) */
    struct payload *pl3 = malloc(sizeof(*pl3));
    assert(pl3);
    pl3->data = my_strdup("three");
    pl3->free_counter = &freed;
    VersionNode n3 = version_node_create(pl3, 12, 2, n2, free_payload);
    assert(n3);
    assert(n3->prev == n2);

    /* Free the head (n3) -> should free all three payloads and nodes */
    version_node_free(n3);
    assert(freed == 3 && "free_value should have been called for all three payloads");

    /* calling version_node_free(NULL) should be safe / no-op */
    version_node_free(NULL);

    printf("All version_node tests passed.\n");
    return 0;
}

