#ifndef ONITAMA_MOVE_H
#define ONITAMA_MOVE_H

#include "types.h"

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

    constexpr Move create_move(uint32_t mask_1, uint32_t mask_2, bool card_index,
                               bool capture, bool piece_type) {
        return mask_1 | mask_2 | (card_index << SQUARE_NUM) | (capture << 26) |
               (piece_type << 27);
    }
}  // namespace MoveBits

#endif  // ONITAMA_MOVE_H
