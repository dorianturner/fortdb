#include <stdlib.h>
#include "list.h"

list_t list_create(void) {
    list_t list = malloc(sizeof(struct list_t));
    if (list == NULL) return NULL;

    list->head = NULL;
    list->tail = NULL;

    return list;
}

void list_destroy(list_t list, void (*free_cb)(void *)) {
    if (!list) return;

    list_entry current = list->head;
    while (current) {
        list_entry next = current->next;
        if (free_cb) free_cb(current->value);
        free(current);
        current = next;
    }

    free(list);
}
