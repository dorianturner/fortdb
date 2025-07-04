struct list_entry;
typedef struct list_entry *list_entry;
struct list_entry {
    void *value;
    list_entry prev;
    list_entry next;
};


typedef struct list *list;
struct list {
    list_entry head;
    list_entry tail;

};

