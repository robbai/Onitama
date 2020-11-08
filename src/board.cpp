#include "board.h"

#include <cstring>

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
