#include <pthread.h>
#include "field.h"
#include "hash.h"
#include "collection.h"
#include "document.h"
#include "common.h"

struct fort {

};

struct Entry {
    char *key;
    void *value;
    Entry next;
};