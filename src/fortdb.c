#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "document.h"
#include "ir.h"
#include "parser.h"
#include "decode_and_execute.h"

#define INPUT_BUFFER_SIZE 1024
#define MAX_ARGS 32

static void print_help(void) {
    puts(
"fortdb - interactive help\n"
"\n"
"Supported Commands\n"
"  load <path>                load /home/me/db.fort       Load database from file\n"
"  get <path> [--v=<V>]      get users/john/age           Fetch field value (optional local version V)\n"
"  set <path> <value>        set users/john/age 42       Insert or update field\n"
"  delete <path>             delete users/john/age       Tombstone an entity\n"
"  list-versions <path>      list-versions users/john/age  List all versions of an entity\n"
"  compact <path>            compact users/john          Retain only latest versions, remove tombstones\n"
"  save <path>               save /home/me/db.fort       Save current in-memory DB to file\n"
"  exit, quit                exit                        Exit the interactive shell\n"
"  help, ?                   show this help message\n"
"\n"
"Key Features\n"
"  - Append-only writes: SET/DELETE always append; no in-place updates, ensuring crash safety.\n"
"  - Hierarchical versioning: VersionNode chains at Table, Collection, Document, and Field levels.\n"
"  - Global & local versions: uint64_t counters track DB-wide and per-entity changes.\n"
"  - Time-travel reads: Query any historical state with --v flag.\n"
"  - Atomic compaction: Background process compacts data and swaps files atomically.\n"
"  - Thread-safe: RW-locks on structures and global mutex for version counter.\n"
"\n"
"Example Session\n"
"  $ ./fortdb\n"
"  fortdb started. Type 'exit' or 'quit' to quit.\n"
"  fortdb> load db.fort\n"
"  Loaded database from db.fort\n"
"  fortdb> get users/alice/email\n"
"  alice@example.com\n"
"  fortdb> set users/alice/age 30\n"
"  OK\n"
"  fortdb> list-versions users/alice/age\n"
"  v1: 25\n"
"  v2: 30\n"
"  fortdb> compact users/alice\n"
"  Compacted users/alice\n"
"  fortdb> save db.fort\n"
"  Saved database to db.fort\n"
"  fortdb> exit\n"
"\n"
"Getting Help\n"
"  Type `help` or `?` at the prompt for this summary.\n"
    );
}

int main(void) {
    Document root = document_create();
    if (!root) {
        fprintf(stderr, "Failed to initialize database.\n");
        return 1;
    }

    printf("fortdb started. Type 'exit' to quit.\n");

    char input[INPUT_BUFFER_SIZE];
    uint64_t global_version = 0;

    while (1) {
        printf("fortdb> ");
        if (!fgets(input, sizeof(input), stdin)) break;
        input[strcspn(input, "\n")] = '\0';
        if (strcmp(input, "exit") == 0 || strcmp(input, "quit") == 0) break;

        // Tokenize input like a shell (space-delimited)
        char *args[MAX_ARGS];
        int argc = 0;
        char *token = strtok(input, " ");
        while (token && argc < MAX_ARGS) {
            args[argc++] = token;
            token = strtok(NULL, " ");
        }

        if (argc == 0) continue;

        // Help short-circuit
        if (strcmp(args[0], "help") == 0 || strcmp(args[0], "?") == 0) {
            print_help();
            continue;
        }

        // Parse arguments into instruction
        Instr instr = parse_args(argc, args, global_version);
        if (!instr) {
            fprintf(stderr, "Invalid command or arguments.\n");
            continue;
        }
        
        // Decode and execute
        int status = decode_and_execute(root, instr);
        if (status != 0) {
            fprintf(stderr, "Error decoding and executing instruction.\n");
        }

        global_version++;
    }

    document_free(root);
    return 0;
}

