#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdint.h>

#include "decode_and_execute.h"
#include "ir.h"
#include "document.h"
#include "./storage/compactor.h"
#include "./storage/deserializer.h"
#include "./storage/serializer.h"
#include "./utils/visualiser.h"
int decode_and_execute(VersionNode v_root, Instr instr) {
    if (!instr) return -1;
    int ret;
    Document root = v_root->value;
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
            char *val = document_get_field(root, instr->get.path, instr->get.version);

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
            //root is a vnode
            VersionNode node = find_version_node_by_path(v_root, instr->compact.path);
            ret = version_node_compact(node);

            if (ret != 0) {
                fprintf(stderr, "version_node_compact: %d\n", ret);
                return ret;
            }
            printf("Compacted %s\n", instr->compact.path ? instr->compact.path : "");
            return 0;
            
        case COMPACT_DB:
            //root is a vnode

            ret = compactor_compact(v_root);

            if (ret != 0) {
                fprintf(stderr, "Error in document_compact: %d\n", ret);
                return ret;
            }
            printf("Compacted %s\n", instr->compact.path ? instr->compact.path : "");
            return 0;

        case LOAD: {
            // Deserialize into a temporary VersionNode pointer
            VersionNode new_root = NULL;
            int ret = deserialize_db(instr->load.path, &new_root);
            if (ret != 0 || !new_root) {
                fprintf(stderr, "Error in deserialize_db: %d\n", ret);
                return ret;
            }

            // Free old document
            Document old_doc = (Document)v_root->value;
            if (old_doc) document_free(old_doc);

            // Replace current root document with loaded one
            v_root->value = new_root->value;
            new_root->value = NULL; // prevent double-free

            // Free temporary VersionNode chain except the first node
            version_node_free(new_root->prev);

            printf("Successfully loaded database from '%s'\n", instr->load.path);
            return 0;
        }


        case SAVE:
        //root, filename
            const char *path = instr->save.path;
            const char *file = instr->save.filename;

            size_t len = strlen(path) + strlen(file) + 2; // +1 for '/' +1 for '\0'
            char *fullpath = malloc(len);
            if (!fullpath) {
                // handle allocation failure
            }

            if (path[strlen(path) - 1] == '/')
                snprintf(fullpath, len, "%s%s", path, file);
            else
                snprintf(fullpath, len, "%s/%s", path, file);

            ret = serialize_db(v_root, fullpath);

            free(fullpath);

            if (ret != 0) {
                fprintf(stderr, "Error in serialize_db: %d\n", ret);
                return ret;
            }
            printf("Saved database to %s\n", instr->save.filename ? instr->save.filename : "");
            return 0;

        case DUMP:
            visualize_db(v_root);
        return 0;

        default:
            fprintf(stderr, "Unknown instruction type.\n");
            return -1;
    }
}

