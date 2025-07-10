#ifndef FORTDB_H
#define FORTDB_H

#include <stdint.h>
#include <pthread.h>
#include "fortdb_types.h"

// Opaque handles (hide internal structures)
typedef struct FortDB FortDB;
typedef struct Table Table;
typedef struct Row Row;
typedef struct Cell Cell;

// Error codes
#define FORTDB_OK 0
#define FORTDB_NOTFOUND 1
#define FORTDB_LOCKED 2
#define FORTDB_IOERR 3

// Core API
int fortdb_init(FortDB** db, const char* path);
int fortdb_shutdown(FortDB* db);

// Thread-safe operations
int fortdb_set(FortDB* db, const char* key, const char* value);
int fortdb_get(FortDB* db, const char* key, char** out_value, uint64_t version);
int fortdb_delete(FortDB* db, const char* key);

// Versioning API
int fortdb_list_versions(FortDB* db, const char* key, VersionInfo** out_versions, size_t* out_count);

// Maintenance
int fortdb_compact(FortDB* db);

// Concurrency control
int fortdb_table_lock(FortDB* db, const char* table_name, int write_mode);
int fortdb_table_unlock(FortDB* db, const char* table_name);

// Debugging
void fortdb_print_stats(const FortDB* db);

#endif // FORTDB_H