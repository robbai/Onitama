#ifndef ONITAMA_EVALUATE_H
#define ONITAMA_EVALUATE_H


#include "board.h"

namespace Evaluation {
    extern const int TOTAL_PARAMETERS;
    extern int PARAMETERS[];
}  // namespace Evaluation


void init_evaluation_parameters();

int evaluate(Board *board);

int to_centi(int evaluation);

int evaluate_move(Board *board, Move move);

#endif  // ONITAMA_EVALUATE_H
