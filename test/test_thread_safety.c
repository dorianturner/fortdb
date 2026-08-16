#include <assert.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include "../src/storage/compactor.h"
#include "../src/storage/deserializer.h"
#include "../src/storage/serializer.h"
#include "../src/utils/document.h"
#include "../src/utils/version_node.h"

struct state {
    Document doc;
    VersionNode root;
    const char *file;
    atomic_int errors;
};

static void fail(struct state *s) {
    atomic_fetch_add_explicit(&s->errors, 1, memory_order_relaxed);
}

static int valid_value(const char *value) {
    return value && value != (const char *)DELETED &&
           (strcmp(value, "initial") == 0 || strncmp(value, "value-", 6) == 0);
}

static void *writer(void *arg) {
    struct state *s = arg;
    char value[64];
    for (uint64_t i = 1; i <= 1200; i++) {
        snprintf(value, sizeof(value), "value-%llu", (unsigned long long)i);
        if (document_set_field(s->doc, "key", value, i) != 0) fail(s);
    }
    return NULL;
}

static void *reader(void *arg) {
    struct state *s = arg;
    for (int i = 0; i < 1800; i++) {
        char *value = document_get_field(s->doc, "key", UINT64_MAX);
        if (!valid_value(value)) {
            fail(s);
        }
        if (value != (char *)DELETED) free(value);
    }
    return NULL;
}

static void *compactor(void *arg) {
    struct state *s = arg;
    for (int i = 0; i < 240; i++) {
        if (compactor_compact(s->root) != 0) fail(s);
    }
    return NULL;
}

static void *saver(void *arg) {
    struct state *s = arg;
    for (int i = 0; i < 160; i++) {
        if (serialize_db(s->root, s->file) != 0) fail(s);
    }
    return NULL;
}

static void *loader(void *arg) {
    struct state *s = arg;
    for (int i = 0; i < 220; i++) {
        VersionNode snapshot = NULL;
        int rc = deserialize_db(s->file, &snapshot);
        if (rc == 0) {
            char *value = document_get_field((Document)snapshot->value, "key", UINT64_MAX);
            if (!valid_value(value)) {
                fail(s);
            }
            if (value != (char *)DELETED) free(value);
            version_node_free(snapshot);
        }
    }
    return NULL;
}

static void test_immutable_reads_and_pinned_documents(void) {
    Document cycle_a = document_create();
    Document cycle_b = document_create();
    assert(cycle_a && cycle_b);
    assert(document_set_subdocument(cycle_a, "b", cycle_b, 1) == 0);
    assert(document_set_subdocument(cycle_b, "a", cycle_a, 2) != 0);
    document_free(cycle_b);
    document_free(cycle_a);

    Document root = document_create();
    assert(root);
    assert(document_set_field(root, "key", "first", 1) == 0);
    char *snapshot = document_get_field(root, "key", 1);
    assert(snapshot && strcmp(snapshot, "first") == 0);
    assert(document_set_field(root, "key", "second", 2) == 0);
    assert(strcmp(snapshot, "first") == 0);
    free(snapshot);

    Document child1 = document_create();
    assert(child1);
    assert(document_set_field(child1, "child-key", "child-value", 1) == 0);
    assert(document_set_subdocument(root, "child", child1, 1) == 0);
    document_free(child1);
    Document pinned = document_get_subdocument(root, "child", UINT64_MAX);
    assert(pinned);

    Document child2 = document_create();
    assert(child2);
    assert(document_set_field(child2, "child-key", "new-child-value", 2) == 0);
    assert(document_set_subdocument(root, "child", child2, 2) == 0);
    document_free(child2);

    VersionNode root_node = version_node_create(root, 1, 1, NULL,
                                                  (void (*)(void *))document_free);
    assert(root_node);
    assert(compactor_compact(root_node) == 0);
    snapshot = document_get_field(pinned, "child-key", UINT64_MAX);
    assert(snapshot && strcmp(snapshot, "child-value") == 0);
    free(snapshot);
    document_free(pinned);
    version_node_free(root_node);
}

int main(void) {
    test_immutable_reads_and_pinned_documents();

    const char *file = "thread-safety.fortdb";
    unlink(file);
    Document doc = document_create();
    assert(doc);
    assert(document_set_field(doc, "key", "initial", 0) == 0);
    VersionNode root = version_node_create(doc, 0, 1, NULL,
                                            (void (*)(void *))document_free);
    assert(root);

    struct state state = {.doc = doc, .root = root, .file = file};
    atomic_init(&state.errors, 0);
    pthread_t threads[5];
    assert(pthread_create(&threads[0], NULL, writer, &state) == 0);
    assert(pthread_create(&threads[1], NULL, reader, &state) == 0);
    assert(pthread_create(&threads[2], NULL, compactor, &state) == 0);
    assert(pthread_create(&threads[3], NULL, saver, &state) == 0);
    assert(pthread_create(&threads[4], NULL, loader, &state) == 0);
    for (size_t i = 0; i < 5; i++) assert(pthread_join(threads[i], NULL) == 0);

    assert(atomic_load_explicit(&state.errors, memory_order_relaxed) == 0);
    char *value = document_get_field(doc, "key", UINT64_MAX);
    assert(value && strncmp(value, "value-", 6) == 0);
    free(value);

    version_node_free(root);
    unlink(file);
    puts("Thread-safety, atomic-write, and immutability tests passed.");
    return 0;
}
