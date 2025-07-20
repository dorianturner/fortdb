#include <stdio.h>
#include "decode_and_execute.h"
#include "ir.h"
#include "table.h"

int decode_and_execute(Table root, Instr instr) {
    if (!instr) return -1;

    int ret;

    switch (instr->instr_type) {
        case SET:
            ret = table_set_field(
                root,
                instr->set.path,
                instr->set.value,
                instr->global_version
            );
            if (ret != 0) {
                fprintf(stderr, "Error in table_set_field: %d\n", ret);
            }
            return ret;

        case GET: {
            void *result = table_get_field(
                root,
                instr->get.path,
                instr->get.version
            );
            if (result) {
                printf("%s\n", (char *)result);
            } else {
                fprintf(stderr, "Value not found.\n");
            }
            return 0;
        }

        case DELETE:
            ret = table_delete_path(
                root,
                instr->delete.path,
                instr->global_version
            );
            if (ret != 0) {
                fprintf(stderr, "Error in table_delete_path: %d\n", ret);
            }
            return ret;

        case VERSIONS:
            ret = table_list_versions(
                root,
                instr->versions.path
            );
            if (ret != 0) {
                fprintf(stderr, "Error in table_list_versions: %d\n", ret);
            }
            return ret;

        case COMPACT:
            ret = table_compact(
                root,
                instr->compact.path
            );
            if (ret != 0) {
                fprintf(stderr, "Error in table_compact: %d\n", ret);
            }
            return ret;

        case LOAD:
            ret = table_load(
                root,
                instr->load.path
            );
            if (ret != 0) {
                fprintf(stderr, "Error in table_load: %d\n", ret);
            }
            return ret;

        case SAVE:
            ret = table_save(
                root,
                instr->save.filename,
                instr->save.path
            );
            if (ret != 0) {
                fprintf(stderr, "Error in table_save: %d\n", ret);
            }
            return ret;

        default:
            fprintf(stderr, "Unknown instruction type.\n");
            return -1;
    }
}

