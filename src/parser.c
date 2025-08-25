#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "parser.h"
#include "ir.h"

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
    else if (strcmp(args[0], "compact_db") == 0)     op = COMPACT_DB;
    else return NULL;

    Instr instr = malloc(sizeof *instr);
    if (!instr) return NULL;
    instr->instr_type = op;
    instr->global_version = global_version;

    switch (op) {
      case SET:
        if (argc != 3) { free(instr); return NULL; }
        instr->set.path = args[1];
        instr->set.value = args[2]; // Interpret all data as a string
        break;

      case GET:
        if (argc < 2 || argc > 3) { free(instr); return NULL; }
        instr->get.path = args[1];
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

      case COMPACT_DB:
        if (argc != 1) { free(instr); return NULL; }
        break;

      case LOAD:
        if (argc != 2) { free(instr); return NULL; }
        instr->load.path = args[1];
        break;

      case SAVE:
        if (argc != 3) { free(instr); return NULL; }
        instr->save.filename = args[1];
        instr->save.path = args[2];
        break;

      default:
        free(instr);
        return NULL;
    }

    return instr;
}

