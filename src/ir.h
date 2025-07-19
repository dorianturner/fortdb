#ifndef IR_H
#define IR_H

#include <stdint.h>

typedef enum {
    SET,
    GET,
    DELETE,
    VERSIONS,
    COMPACT,
    LOAD,
    SAVE
} INSTR_TYPE;

// depracated
typedef enum {
    STRING,
    INT,
    FLOAT,
    BOOLEAN
} DATA_TYPE;

typedef struct Instr *Instr;
struct Instr {
    INSTR_TYPE instr_type;
    uint64_t global_version;
    union {
        struct {
            char *value;
            const char *path;
            // DATA_TYPE type;
        } set;
        struct {
            char *key;
            const char *path;
            int version;
        } get;
        struct {
            const char *path;
        } delete;
        struct {
            const char *path;
        } versions;
        struct {
            const char *path;
        } compact;
        struct {
            const char *path;
        } load;
        struct {
            char *filename;
            const char *path;
        } save;
    };
};

#endif

