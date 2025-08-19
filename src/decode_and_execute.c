#include <stdio.h>
#include "decode_and_execute.h"
#include "ir.h"
#include "document.h"
#include "field.h"

int decode_and_execute(Document root, Instr instr) {
    if (!instr) return -1;
    int ret;

    switch (instr->instr_type) {
        case SET:
            /* instr->set.value is a C string in your IR; use convenience helper */
            ret = document_set_field_cstr(
                root,
                instr->set.path,
                instr->set.value,
                instr->global_version
            );
            if (ret != 0) {
                fprintf(stderr, "Error in document_set_field_cstr: %d\n", ret);
            }
            return ret;

        case GET: {
            Field f = document_get_field(
                root,
                instr->get.path,
                instr->get.version
            );
            if (!f) {
                fprintf(stderr, "Value not found.\n");
                return 0;
            }
            void *val = field_get(f, instr->get.version);
            if (val) {
                printf("%s\n", (char *)val);
            } else {
                fprintf(stderr, "Value not found.\n");
            }
            return 0;
        }

        case DELETE:
            ret = document_delete_path(
                root,
                instr->delete.path,
                instr->global_version
            );
            if (ret != 0) {
                fprintf(stderr, "Error in document_delete_path: %d\n", ret);
            }
            return ret;

        case VERSIONS:
            ret = document_list_versions(
                root,
                instr->versions.path
            );
            if (ret != 0) {
                fprintf(stderr, "Error in document_list_versions: %d\n", ret);
            }
            return ret;

        case COMPACT:
            ret = document_compact(
                root,
                instr->compact.path
            );
            if (ret != 0) {
                fprintf(stderr, "Error in document_compact: %d\n", ret);
            }
            return ret;

        case LOAD:
            ret = document_load(
                root,
                instr->load.path
            );
            if (ret != 0) {
                fprintf(stderr, "Error in document_load: %d\n", ret);
            }
            return ret;

        case SAVE:
            ret = document_save(
                root,
                instr->save.filename,
                instr->save.path
            );
            if (ret != 0) {
                fprintf(stderr, "Error in document_save: %d\n", ret);
            }
            return ret;

        default:
            fprintf(stderr, "Unknown instruction type.\n");
            return -1;
    }
}

