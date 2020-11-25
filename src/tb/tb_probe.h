#ifndef ONITAMA_TB_PROBE_H
#define ONITAMA_TB_PROBE_H


#include "tb_gen.h"
#include "../board.h"

Entry probe_tb(Board *board);

void setup_and_generate_tb(Board *board);


#endif  // ONITAMA_TB_PROBE_H
