#ifndef ONITAMA_SEARCH_H
#define ONITAMA_SEARCH_H


#include "board.h"

constexpr uint8_t MAX_DEPTH = 64;

extern int PARAM;

Move start_search(Board *board, bool silent = false, float max_time = 1);

Board get_pv_leaf(Board board);

struct Line {
    int length = 0;         // Number of moves in the line.
    Move moves[MAX_DEPTH];  // The line.
};

#endif  // ONITAMA_SEARCH_H
