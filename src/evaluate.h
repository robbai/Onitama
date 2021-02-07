#ifndef ONITAMA_EVALUATE_H
#define ONITAMA_EVALUATE_H


#include "board.h"

constexpr int TOTAL_PARAMETERS = (PIECE_TYPES_NUM * CARD_NUM * SQUARE_NUM);

void init_evaluation_parameters();

int evaluate(Board *board);

void get_evaluation_parameters(int *parameters);

void set_evaluation_parameters(int *parameters);

#endif  // ONITAMA_EVALUATE_H
