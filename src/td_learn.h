#ifndef ONITAMA_TD_LEARN_H
#define ONITAMA_TD_LEARN_H

#include "board.h"

void init_td_learn();

void learn_game(Board *board, bool silent = false);

#endif  // ONITAMA_TD_LEARN_H
