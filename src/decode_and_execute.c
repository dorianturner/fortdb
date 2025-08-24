#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdint.h>

#include "decode_and_execute.h"
#include "ir.h"
#include "document.h"

int decode_and_execute(Document root, Instr instr) {
    if (!instr) return -1;
    int ret;

    switch (instr->instr_type) {
        case SET:
            ret = document_set_field_path(
                root,
                instr->set.path,
                instr->set.value,
                instr->global_version
            );
            if (ret != 0) {
                fprintf(stderr, "Error: document_set_field_path returned %d\n", ret);
                return ret;
            }
            printf("OK\n");
            return 0;

        case GET: {
            char *val = document_get_path(root, instr->get.path, instr->get.version);

            if (!val || val == (char*)1) {
                printf("Value not found.\n");
                return 0;
            }

            printf("%s\n", val);
            return 0;
        }

        case DELETE:
            ret = document_delete_path(root, instr->delete.path, instr->global_version);
            if (ret != 0) {
                fprintf(stderr, "Error in document_delete_path: %d\n", ret);
                return ret;
            }
            printf("OK\n");
            return 0;

        case VERSIONS:
            ret = document_list_versions(root, instr->versions.path);
            if (ret != 0) {
                fprintf(stderr, "Error in document_list_versions: %d\n", ret);
                return ret;
            }
            return 0;

        case COMPACT:
            ret = document_compact(root, instr->compact.path);
            if (ret != 0) {
                fprintf(stderr, "Error in document_compact: %d\n", ret);
                return ret;
            }
            printf("Compacted %s\n", instr->compact.path ? instr->compact.path : "");
            return 0;

        case LOAD:
            ret = document_load(root, instr->load.path);
            if (ret != 0) {
                fprintf(stderr, "Error in document_load: %d\n", ret);
                return ret;
            }
            printf("Loaded database from %s\n", instr->load.path ? instr->load.path : "");
            return 0;

        case SAVE:
            ret = document_save(root, instr->save.filename, instr->save.path);
            if (ret != 0) {
                fprintf(stderr, "Error in document_save: %d\n", ret);
                return ret;
            }
            printf("Saved database to %s\n", instr->save.filename ? instr->save.filename : "");
            return 0;

        default:
            fprintf(stderr, "Unknown instruction type.\n");
            return -1;
    }
}

