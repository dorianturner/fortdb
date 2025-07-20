#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "table.h"
#include "ir.h"
#include "parser.h"
#include "decode_and_execute.h"

#define INPUT_BUFFER_SIZE 1024
#define MAX_ARGS 32

int main(void) {
    Table root = table_create();
    if (!root) {
        fprintf(stderr, "Failed to initialize database.\n");
        return 1;
    }

    printf("fortdb started. Type 'exit' to quit.\n");

    char input[INPUT_BUFFER_SIZE];
    uint64_t global_version = 0;

    while (1) {
        // Fetch
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
        // Parse and Tokenise instructions
        Instr instr = parse_args(argc, args, global_version);
        if (!instr) {
            fprintf(stderr, "Invalid command or arguments.\n");
            continue;
        }
        
        // Decode and execute
        int status = decode_and_execute(root, instr);
        if (status != 0) {
            fprintf(stderr, "Error decoding and executing Instr");
        }

        global_version++;
    }

    table_free(root);
    return 0;
}

