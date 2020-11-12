#ifndef ONITAMA_BOARD_H
#define ONITAMA_BOARD_H

#include "types.h"

constexpr Bitboard HOMES[PLAYERS_NUM] = {4, 4194304};

class Board {
 public:
    // Bitboard represent both players students and masters.
    Bitboard pieces[PLAYERS_NUM][PIECE_TYPES_NUM] = {{27, HOMES[0]},
                                                     {28311552, HOMES[1]}};
    Card cards[CARDS_EACH_NUM][CARDS_EACH_NUM] = {
            {CARD_NONE, CARD_NONE},
            {CARD_NONE, CARD_NONE}};  // Cards for both players.
    Card side_card = CARD_NONE;       // Card at the side.
    bool turn = WHITE;                // Turn to move.
    uint8_t move_count = 0;

    bool game_over();
    Board copy();
    bool operator==(const Board &other);
};

#endif  // ONITAMA_BOARD_H
