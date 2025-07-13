#ifndef DBELEMS_H
#define DBELEMS_H

#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include "hash.h"

typedef struct VersionNode *VersionNode;
typedef struct Field *Field;
typedef struct Document *Document;
typedef struct Collection *Collection;
typedef struct Table *Table;

struct VersionNode {
    void *value;
    uint64_t global_version;
    uint64_t local_version;
    VersionNode prev;
};

struct Field {
    char *name;
    VersionNode versions;
};

struct Document {
    HashMap fields; // char* → VersionNode (Field)
    HashMap subcollections; // char* → VersionNode(Collection) (optional)
};

struct Collection {
    pthread_rwlock_t lock;
    HashMap documents; // char* → VersionNode(Document)
};

struct Table {
    pthread_rwlock_t lock;
    HashMap collections; // char* → VersionNode(Collection)
};

#define RECORD_TYPE_SET     1
#define RECORD_TYPE_DELETE  2

struct Record {
    uint64_t global_version;
    uint8_t type;
    uint64_t local_version;
    uint32_t key_len;
    uint32_t val_len;
    char data[];
};

extern uint64_t GLOBAL_VERSION;
extern pthread_mutex_t GLOBAL_VERSION_LOCK;

VersionNode version_node_create(void *value, uint64_t global_version, uint64_t local_version);
void version_node_free_chain(VersionNode head);

int field_set(Field field, void *value, uint64_t global_version);
void *field_get(Field field, uint64_t global_version);
void field_free(Field field);

Document document_create(void);
void document_free(Document doc);
Field document_get_field(Document doc, const char *key, int create);

Collection collection_create(void);
void collection_free(Collection coll);
Document collection_get_document(Collection coll, const char *doc_id, int create);

Table table_create(void);
void table_free(Table table);
Collection table_get_collection(Table table, const char *name, int create);

#endif

