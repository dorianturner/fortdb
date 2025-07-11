#ifndef STORAGE_H
#define STORAGE_H

#include <stdint.h>
#include "dbelems.h"

/**
 * storage.h
 * Crash-proof append-only log storage engine
 * 
 * Manages on-disk storage with:
 * - Atomic record appends
 * - Versioned record retrieval
 * - Background compaction
 */

typedef struct LogFile LogFile;

// File operations
LogFile* storage_open(const char* path);
void storage_close(LogFile* file);

// Core operations
int storage_append(LogFile* file, const Record* record);
int storage_read_next(LogFile* file, Record* record);
uint64_t storage_last_version(const LogFile* file);

// Compaction
int storage_compact(LogFile* source, LogFile* dest);

#endif