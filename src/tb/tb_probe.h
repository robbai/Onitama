#ifndef ONITAMA_TB_PROBE_H
#define ONITAMA_TB_PROBE_H

#include "tb_gen.h"
#include "../board.h"

extern bool GENERATED_TB;

TBEntry probe_tb(Board *board);

void setup_and_generate_tb(Board *board, uint8_t student_men = 2);

void drop_tb();

#endif  // ONITAMA_TB_PROBE_H
