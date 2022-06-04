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
    uint16_t move_count = 0;
    Hash hash = 0;
    uint8_t student_count = MAX_STUDENTS;
    int8_t student_delta = 0;

    bool game_over();
    Board copy();
    bool operator==(const Board &other);

    Bitboard get_checkers(bool turn);
    inline Bitboard get_checkers() {
        return this->get_checkers(this->turn);
    }
    bool get_runner(bool turn);
    inline bool get_runner() {
        return this->get_runner(this->turn);
    }
    bool has_winning_move(bool turn);
    inline bool has_winning_move() {
        return this->has_winning_move(this->turn);
    }
};

#endif  // ONITAMA_BOARD_H
