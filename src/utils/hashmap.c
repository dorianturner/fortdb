hashmap hashmap_create(size_t initial_size) {
    hashmap map = malloc(sizeof(struct hashmap));
    if (!map) return NULL;

    map->buckets = calloc(initial_size, sizeof(kv_entry));
    if (!map->buckets) {
        free(map);
        return NULL;
    }

    map->size = initial_size;
    return map;
}

void hashmap_destroy(hashmap map, void (*free_cb)(void *)) {
    if (!map) return;

    for (size_t i = 0; i < map->size; ++i) {
        kv_entry entry = map->buckets[i];
        while (entry) {
            kv_entry next = entry->next;
            if (free_cb) free_cb(entry->value);
            free(entry->key); // maybe not needed but you could've strdup'd
            free(entry);
            entry = next;
        }
    }

    free(map->buckets);
    free(map);
}
