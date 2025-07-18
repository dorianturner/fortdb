#include <stdio.h>
#include "decode_and_execute.h"
#include "ir.h"
#include "table.h"

int decode_and_execute(Table root, Instr *instr) {
    if (!instr) return -1;

    switch (instr->instr_type) {
        case SET:
            return table_set_field(
                root,
                instr->set.path,
                instr->set.type,
                instr->set.value,
                instr->global_version
            );

        case GET:
            // Should be a string as fields should only ever be set as strings
            return table_get_field(
                root,
                instr->get.path,
                instr->get.version
            );

        case DELETE:
            return table_delete_path(
                root,
                instr->delete.path,
                instr->global_version
            );

        case VERSIONS:
            return table_list_versions(
                root,
                instr->versions.path
            );

        case COMPACT:
            return table_compact(
                root,
                instr->compact.path,
                instr->global_version
            );

        case LOAD:
            return table_load(
                root,
                instr->load.path
            );

        case SAVE:
            return table_save(
                root,
                instr->save.filename,
                instr->save.path
            );

        default:
            fprintf(stderr, "Unknown instruction type.\n");
            return -1;
    }
}

