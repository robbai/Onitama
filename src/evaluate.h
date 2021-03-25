#ifndef ONITAMA_EVALUATE_H
#define ONITAMA_EVALUATE_H


#include "board.h"

namespace Evaluation {
    extern const int TOTAL_PARAMETERS;
    extern int PARAMETERS[];
}  // namespace Evaluation


void init_evaluation_parameters();

int evaluate(Board *board);

void get_evaluation_parameters(int *parameters);

void set_evaluation_parameters(int *parameters);

bool is_parameter_used(Board *board, int p);

int to_centi(int evaluation);

#endif  // ONITAMA_EVALUATE_H
