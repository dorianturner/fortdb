#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "parser.h"
#include "ir.h"

// Helper: string → DATA_TYPE
static DATA_TYPE get_datatype(const char *s) {
    if (strcmp(s, "string") == 0)  return STRING;
    if (strcmp(s, "int") == 0)     return INT;
    if (strcmp(s, "float") == 0)   return FLOAT;
    if (strcmp(s, "boolean") == 0) return BOOLEAN;
    return -1;
}

Instr parse_args(int argc, char *args[], uint64_t global_version) {
    if (argc < 1) return NULL;

    INSTR_TYPE op;
    if      (strcmp(args[0], "set") == 0)            op = SET;
    else if (strcmp(args[0], "get") == 0)            op = GET;
    else if (strcmp(args[0], "delete") == 0)         op = DELETE;
    else if (strcmp(args[0], "list-versions") == 0)  op = VERSIONS;
    else if (strcmp(args[0], "compact") == 0)        op = COMPACT;
    else if (strcmp(args[0], "load") == 0)           op = LOAD;
    else if (strcmp(args[0], "save") == 0)           op = SAVE;
    else return NULL;

    // 2) Allocate instruction
    Instr instr = malloc(sizeof *instr);
    if (!instr) return NULL;
    instr->instr_type = op;
    instr->global_version = global_version;

    // 3) Parse ops into instructions 
    switch (op) {
      case SET:
        if (argc != 3) { free(instr); return NULL; }
        instr->set.path  = args[1];
        {
          char *type_tok = strtok(args[2], ":");
          char *val_tok  = strtok(NULL,    ":");
          DATA_TYPE dt = get_datatype(type_tok);
          if (dt < 0 || !val_tok) { free(instr); return NULL; }
          instr->set.type  = dt;
          instr->set.value = val_tok;
        }
        break;

      case GET:
        if (argc < 2 || argc > 3) { free(instr); return NULL; }
        instr->get.path    = args[1];
        instr->get.version = -1;
        if (argc == 3 && strncmp(args[2], "--v=", 4) == 0)
            instr->get.version = atoi(args[2] + 4);
        break;

      case DELETE:
        if (argc != 2) { free(instr); return NULL; }
        instr->delete.path = args[1];
        break;

      case VERSIONS:
        if (argc != 2) { free(instr); return NULL; }
        instr->versions.path = args[1];
        break;

      case COMPACT:
        if (argc != 2) { free(instr); return NULL; }
        instr->compact.path = args[1];
        break;

      case LOAD:
        if (argc != 2) { free(instr); return NULL; }
        instr->load.path = args[1];
        break;

      case SAVE:
        if (argc != 3) { free(instr); return NULL; }
        instr->save.filename = args[1];
        instr->save.path     = args[2];
        break;
    
      default:
        return NULL;
    }

    return instr;
}

