#include <stdio.h>
#include "decode_and_execute.h"
#include "ir.h"
#include "table.h"

void *decode_and_execute(Table root, Instr instr) {
    if (!instr) return NULL;

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
                return NULL;
            }
            return (void *)(intptr_t)ret;  // or just return NULL since success is 0?

        case GET:
            // Returns pointer, just return it
            return table_get_field(
                root,
                instr->get.path,
                instr->get.version
            );

        case DELETE:
            ret = table_delete_path(
                root,
                instr->delete.path,
                instr->global_version
            );
            if (ret != 0) {
                fprintf(stderr, "Error in table_delete_path: %d\n", ret);
                return NULL;
            }
            return (void *)(intptr_t)ret;

        case VERSIONS:
            ret = table_list_versions(
                root,
                instr->versions.path
            );
            if (ret != 0) {
                fprintf(stderr, "Error in table_list_versions: %d\n", ret);
                return NULL;
            }
            return (void *)(intptr_t)ret;

        case COMPACT:
            ret = table_compact(
                root,
                instr->compact.path
            );
            if (ret != 0) {
                fprintf(stderr, "Error in table_compact: %d\n", ret);
                return NULL;
            }
            return (void *)(intptr_t)ret;

        case LOAD:
            ret = table_load(
                root,
                instr->load.path
            );
            if (ret != 0) {
                fprintf(stderr, "Error in table_load: %d\n", ret);
                return NULL;
            }
            return (void *)(intptr_t)ret;

        case SAVE:
            ret = table_save(
                root,
                instr->save.filename,
                instr->save.path
            );
            if (ret != 0) {
                fprintf(stderr, "Error in table_save: %d\n", ret);
                return NULL;
            }
            return (void *)(intptr_t)ret;

        default:
            fprintf(stderr, "Unknown instruction type.\n");
            return NULL;
    }
}

