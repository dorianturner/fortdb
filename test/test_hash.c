#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "hash.h"
#include "version_node.h"

int main(void) {
    Hashmap map = hashmap_create(1);
    if (!map) {
        fprintf(stderr, "hashmap_create failed\n");
        return 2;
    }

    /* First insert -> internal local_version == 0 */
    char *p1 = strdup("payload1");
    if (!p1) return 2;
    /* pass the payload, hashmap will create VersionNode internally */
    if (hashmap_put(map, "key", p1, 1, free) != 0) {
        fprintf(stderr, "hashmap_put failed\n");
        return 2;
    }

    /* fetch by local_version 0 (the head created for the first insert) */
    void *r = hashmap_get(map, "key", 0);
    assert(r == p1 && "Expected first get to return the stored payload (p1)");

    /* Insert a newer version for the same key -> internal local_version == 1 */
    char *p2 = strdup("payload2");
    if (!p2) return 2;
    if (hashmap_put(map, "key", p2, 2, free) != 0) {
        fprintf(stderr, "hashmap_put failed\n");
        return 2;
    }

    /* Latest visible at local_version == 1 should be p2 */
    void *r2 = hashmap_get(map, "key", 1);
    assert(r2 == p2 && "Expected latest payload (p2) for local_version 1");

    /* A snapshot at local_version 0 should still see p1 */
    void *r1snap = hashmap_get(map, "key", 0);
    assert(r1snap == p1 && "Expected older payload (p1) for local_version 0");

    /* Collision test: different key should not interfere when bucket_count==1 */
    char *p3 = strdup("other");
    if (!p3) return 2;
    if (hashmap_put(map, "another-key", p3, 1, free) != 0) {
        fprintf(stderr, "hashmap_put failed\n");
        return 2;
    }
    void *r3 = hashmap_get(map, "another-key", 0);
    assert(r3 == p3 && "Expected to fetch p3 for another-key");

    /* cleanup */
    hashmap_free(map);

    printf("All hashmap tests passed.\n");
    return 0;
}

