#ifndef PARSER_H
#define PARSER_H

#include "ir.h"
#include <stdint.h>

Instr parse_args(int argc, char *args[], uint64_t global_version);

#endif
