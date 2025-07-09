#include "hash.h"

// https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
// FNV-1a hash
static uint64_t hash(const char *key) {
    uint64_t hash = 14695981039346656037ULL; // Offset Basis
    while (*key) {
        hash ^= (unsigned char)(*key);
        hash *= 1099511628211ULL; // FNV Prime
        key++;
    }
    return hash;
}
