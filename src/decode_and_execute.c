#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "decode_and_execute.h"
#include "ir.h"
#include "document.h"
#include "field.h"

/* Robust decode+execute:
 * - SET uses document_set_field_path(...) and prints "OK" on success
 * - GET tries the requested local-version, then falls back to the latest local version
 * - LOAD/SAVE/COMPACT/VERSIONS/DELETE print the single user-facing messages expected
 */
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
            Field f = document_get_field(root, instr->get.path, 0);
            if (!f) {
                printf("Value not found.\n");
                return 0;
            }

            /* Try requested local version first (instr->get.version).
               If no value, read the latest local_version and use that. */
            void *val = field_get(f, instr->get.version);
            if (!val) {
                uint64_t latest = 0;
                if (pthread_rwlock_rdlock(&f->lock) == 0) {
                    if (f->versions) latest = f->versions->local_version;
                    pthread_rwlock_unlock(&f->lock);
                }
                val = field_get(f, latest);
            }

            if (val) {
                printf("string:%s\n", (char *)val);
            } else {
                printf("Value not found.\n");
            }
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

