typedef struct kv_entry *kv_entry;
typedef struct kv_entry {
    char *key;
    void *value;
    kv_entry next;
};

typedef struct hashmap *hashmap;
typedef struct hashmap {
    kv_entry *buckets;
    size_t size;
};

