#ifndef ONITAMA_MOVE_GEN_H
#define ONITAMA_MOVE_GEN_H

#include "board.h"
#include "types.h"

uint8_t gen_moves(Board *board, Move *moves);
uint8_t count_moves(Board *board);

#endif  // ONITAMA_MOVE_GEN_H
