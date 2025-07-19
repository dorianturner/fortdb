#ifndef DECODE_AND_EXECUTE_H
#define DECODE_AND_EXECUTE_H

#include "ir.h"
#include "table.h"

void *decode_and_execute(Table root, Instr instr);

#endif
