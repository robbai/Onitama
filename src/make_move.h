#ifndef ONITAMA_MAKE_MOVE_H
#define ONITAMA_MAKE_MOVE_H


#include "board.h"


void make_move(Board *board, Move &move);
void undo_move(Board *board, Move &move);


#endif //ONITAMA_MAKE_MOVE_H
