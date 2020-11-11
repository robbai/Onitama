#ifndef ONITAMA_MOVE_H
#define ONITAMA_MOVE_H

#include "types.h"
#include "util.h"

namespace MoveBits {
    constexpr Bitboard xor_board(Move move) {
        return move & 33554431;
    }

    constexpr bool card_index(Move move) {
        return move & 33554432;
    }

    constexpr bool capture(Move move) {
        return move & 67108864;
    }

    constexpr bool piece_type(Move move) {
        return move & 134217728;
    }

    constexpr Move half_create_move(uint32_t mask_1, bool card_index, bool piece_type) {
        return mask_1 | (card_index << SQUARE_NUM) | (piece_type << 27);
    }

    constexpr Move finish_create_move(Move move, uint32_t mask_2, bool capture) {
        return move | mask_2 | (capture << 26);
    }
}  // namespace MoveBits

#endif  // ONITAMA_MOVE_H
