#ifndef IR_H
#define IR_H

typedef enum {
    SET,
    GET,
    DELETE,
    VERSIONS,
    COMPACT,
    LOAD,
    SAVE
} INSTR_TYPE;

typedef enum {
    STRING,
    INT,
    FLOAT,
    BOOLEAN
} DATA_TYPE;

typedef struct Instruction {
    INSTR_TYPE instr_type;
    const char *path;
    union {
        struct {
            char *value;
            DATA_TYPE type;
        } set;
        struct {
            char *key;
            int version;
        } get;
        struct {
            int dummy;
        } delete;
        struct {
            int dummy;
        } versions;
        struct {
            int dummy;
        } compact;
        struct {
            int dummy;
        } load;
        struct {
            char *filename;
        } save;
    };
} Instruction;

#endif

