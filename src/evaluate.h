#ifndef ONITAMA_EVALUATE_H
#define ONITAMA_EVALUATE_H


#include "board.h"

constexpr int evaluate(Board *board) {
    return 1000 * board->student_delta;
}

constexpr int to_centi(int evaluation) {
    return evaluation / 10;
}

#endif  // ONITAMA_EVALUATE_H
