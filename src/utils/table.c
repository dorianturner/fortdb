#include <stdlib.h>
#include <stdint.h>
#include "table.h"
#include "common.h"
#include "collection.h"

// Memory management
Table table_create(void) {
    Table table = malloc(sizeof(struct Table));
    if (!table) return NULL;

    pthread_rwlock_init(&table->lock, NULL);
    if (!table->lock) {
        free(table);
        return NULL;
    }

    table->collections = hashmap_create(DEFAULT_BUCKET_COUNT); 
    if (!table->collections) {
        pthread_rwlock_destroy(&table->lock);
        free(table);
        return NULL;
    }
    
    return table;
}

void table_free(Table table) {
    if (!table) return;
    hashmap_free(table->collections);
    pthread_rwlock_destroy(&table->lock);
    free(table);
}

Collection table_get_collection(Table table, const char *key, uint64_t local_version) {
    if (!table || !key) return NULL;
    if (pthread_rwlock_rdlock(&table->lock) != 0) return NULL;
    Document doc = hashmap_get(table->collections, key, local_version); 
    pthread_rwlock_unlock(&table->lock);
    return doc;
}

int table_set_collection(Table table, const char *key, Collection coll, uint64_t global_version) {
    if (!table || !key || !coll) return -1;
    if (pthread_rwlock_rdlock(&table->lock) != 0) return -1;
    if (hashmap_put(table->collections, key, coll, global_version, (*collection_free)) != 0) return -1;
    pthread_rwlock_unlock(&table->lock);
    return 0;
}
