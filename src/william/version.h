#ifndef VERSION_H
#define VERSION_H

#include <stdint.h>
#include <pthread.h>

/**
 * version.h
 * Global version counter management for FortDB
 * 
 * Provides thread-safe operations for managing the global version counter
 * used throughout the database for versioning and snapshot isolation.
 */

/**
 * Initialize the global version counter system
 * 
 * Must be called before any other version_* functions.
 * 
 * 0 on success, -1 on failure
 */
int version_init();

/**
 * Get the current global version
 * 
 * The current global version number
 */
uint64_t version_current();

/**
 * Increment and get the next global version
 * 
 * Thread-safe operation that increments the global version counter
 * and returns the new value.
 * 
 * The new global version number
 */
uint64_t version_next();

/**
 * Clean up version system resources
 */
void version_cleanup();

#endif