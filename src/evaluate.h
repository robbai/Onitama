#ifndef ONITAMA_EVALUATE_H
#define ONITAMA_EVALUATE_H


#include "board.h"

constexpr int TOTAL_PARAMETERS = (PIECE_TYPES_NUM * CARD_NUM * SQUARE_NUM * PLAYERS_NUM);

void init_evaluation_parameters();

int evaluate(Board *board);

#endif  // ONITAMA_EVALUATE_H
