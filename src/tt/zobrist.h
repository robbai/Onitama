#ifndef ONITAMA_ZOBRIST_H
#define ONITAMA_ZOBRIST_H


#include "../types.h"
#include "../board.h"

namespace Zobrist {
    extern Hash PIECES[PLAYERS_NUM][PIECE_TYPES_NUM][SQUARE_NUM];
    extern Hash CARDS[CARDS_EACH_NUM][PLAYERS_NUM + 1];
    extern Hash TURN;
}  // namespace Zobrist

void init_zobrist();

void set_hash(Board *board);


#endif  // ONITAMA_ZOBRIST_H
