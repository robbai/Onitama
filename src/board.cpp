#include "board.h"

#include <cstring>

#include "move_bits.h"
#include "move_tables.h"

// This function only checks for whether the non-moving player has won.
bool Board::game_over() {
    return (this->pieces[this->turn][MASTER] &
            (this->pieces[!this->turn][STUDENT] | this->pieces[!this->turn][MASTER])) ||
           (this->pieces[!this->turn][MASTER] & HOMES[this->turn]);
}

Board Board::copy() {
    Board board_copy;
    memcpy(&board_copy, &*this, sizeof(*this));
    return board_copy;
}

bool Board::operator==(const Board &other) {
    // Check cards and pieces.
    for (int side = 0; side < PLAYERS_NUM; ++side) {
        for (int card_index = 0; card_index < CARDS_EACH_NUM; ++card_index) {
            if (this->cards[side][card_index] != other.cards[side][card_index])
                return false;
        }
        for (int piece_index = 0; piece_index < PIECE_TYPES_NUM; ++piece_index) {
            if (this->pieces[side][piece_index] != other.pieces[side][piece_index])
                return false;
        }
    }

    return this->side_card == other.side_card && this->turn == other.turn;
}

bool Board::winning_move(Move move) {
    Bitboard to = (1u << MoveBits::to(move));
    return (this->pieces[!this->turn][MASTER] & to) ||
           (MoveBits::piece_type(move) && (to & HOMES[!this->turn]));
}

bool Board::has_winning_move() {
    uint8_t opponent_sq = (this->turn ? 2 : 22);
    if (!(HOMES[!this->turn] & this->pieces[this->turn][STUDENT])) {
        for (int card_index = 0; card_index < CARDS_EACH_NUM; ++card_index) {
            if ((MOVE_TABLES[(this->cards[this->turn][card_index] * SQUARE_NUM +
                              opponent_sq) *
                                     PLAYERS_NUM +
                             !this->turn]) &
                this->pieces[this->turn][MASTER])
                return true;
        }
    }
    opponent_sq = __builtin_ctz(this->pieces[!this->turn][MASTER]);
    Bitboard our_pieces =
            this->pieces[this->turn][STUDENT] | this->pieces[this->turn][MASTER];
    for (int card_index = 0; card_index < CARDS_EACH_NUM; ++card_index) {
        if ((MOVE_TABLES[(this->cards[this->turn][card_index] * SQUARE_NUM +
                          opponent_sq) *
                                 PLAYERS_NUM +
                         !this->turn]) &
            our_pieces)
            return true;
    }
    return false;
}
