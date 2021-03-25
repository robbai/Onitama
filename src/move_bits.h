#ifndef ONITAMA_MOVE_H
#define ONITAMA_MOVE_H

#include "types.h"
#include "util.h"

namespace MoveBits {
    constexpr uint8_t from(Move move) {
        return move & 31;
    }

    constexpr uint8_t to(Move move) {
        return (move >> 5) & 31;
    }

    constexpr Bitboard xor_board(Move move) {
        return (1u << from(move)) | (1u << to(move));
    }

    constexpr bool card_index(Move move) {
        return move & 1024;
    }

    constexpr bool capture(Move move) {
        return move & 2048;
    }

    constexpr bool piece_type(Move move) {
        return move & 4096;
    }

    constexpr bool lower_swap(Move move) {
        return move & 8192;
    }

    constexpr Move half_create_move(uint8_t from, bool card_index, bool piece_type,
                                    bool lower_swap) {
        return from | (card_index << 10) | (piece_type << 12) | (lower_swap << 13);
    }

    constexpr Move finish_create_move(Move move, uint8_t to, bool capture) {
        return move | (to << 5) | (capture << 11);
    }
}  // namespace MoveBits

#endif  // ONITAMA_MOVE_H
