#ifndef LIST_H
#define LIST_H

struct list_entry;
typedef struct list_entry *list_entry;
struct list_entry {
    void *value;
    list_entry next;
};


typedef struct list_t *list_t;
struct list_t {
    list_entry head;
    list_entry tail;

};

list_t list_create(void);
void list_destroy(list_t list, void (*free_cb)(void *));

#endif
