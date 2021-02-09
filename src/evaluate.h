#ifndef ONITAMA_EVALUATE_H
#define ONITAMA_EVALUATE_H


#include "board.h"

constexpr int TOTAL_PARAMETERS = (PIECE_TYPES_NUM * CARD_NUM * SQUARE_NUM * PLAYERS_NUM);

void init_evaluation_parameters();

int evaluate(Board *board);

void get_evaluation_parameters(int *parameters);

void set_evaluation_parameters(int *parameters);

bool is_parameter_used(Board *board, int p);

#endif  // ONITAMA_EVALUATE_H
