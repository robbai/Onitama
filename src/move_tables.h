#ifndef ONITAMA_MOVE_TABLES_H
#define ONITAMA_MOVE_TABLES_H


#include "types.h"

extern Bitboard MOVE_TABLES[CARD_NUM][SQUARE_NUM][PLAYERS_NUM];

void init_move_tables();


#endif //ONITAMA_MOVE_TABLES_H
