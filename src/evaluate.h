#ifndef ONITAMA_EVALUATE_H
#define ONITAMA_EVALUATE_H


#include "board.h"

constexpr int TOTAL_PARAMETERS =
        (PIECE_TYPES_NUM * CARD_NUM * SQUARE_NUM * PLAYERS_NUM * 2);

void init_evaluation_parameters();

int evaluate(Board *board);

int to_centi(int evaluation);

#endif  // ONITAMA_EVALUATE_H
