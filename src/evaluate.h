#ifndef ONITAMA_EVALUATE_H
#define ONITAMA_EVALUATE_H


#include "board.h"

namespace Evaluation {
    constexpr int TOTAL_PARAMETERS =
            (PIECE_TYPES_NUM * CARD_NUM * SQUARE_NUM * PLAYERS_NUM * 2);
    extern int PARAMETERS[];
}  // namespace Evaluation


void init_evaluation_parameters();

int evaluate(Board *board);

void get_evaluation_parameters(int *parameters);

void set_evaluation_parameters(int *parameters);

bool is_parameter_used(Board *board, int p);

int to_centi(int evaluation);

#endif  // ONITAMA_EVALUATE_H
