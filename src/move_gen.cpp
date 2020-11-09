#include "move_gen.h"

#include "move_bits.h"
#include "move_tables.h"

uint8_t gen_moves(Board *board, Move *moves) {
    uint8_t total = 0;

    // Iterate through pieces.
    Bitboard pieces =
            board->pieces[board->turn][STUDENT] | board->pieces[board->turn][MASTER];
    Bitboard targets = ~pieces;
    while (pieces) {
        uint32_t mask_1 = pieces & -pieces;

        // Iterate through cards.
        for (int card_index = 0; card_index < CARDS_EACH_NUM; ++card_index) {
            // Iterate through move squares.
            Bitboard squares = MOVE_TABLES[board->cards[board->turn][card_index]]
                                          [__builtin_ctz(mask_1)][board->turn] &
                               targets;
            while (squares) {
                uint32_t mask_2 = squares & -squares;

                moves[total] = MoveBits::create_move(
                        mask_1, mask_2, card_index,
                        mask_2 & board->pieces[!board->turn][STUDENT],
                        mask_1 & board->pieces[board->turn][MASTER]);
                ++total;

                squares ^= mask_2;
            }
        }

        pieces ^= mask_1;
    }

    return total;
}

uint8_t count_moves(Board *board) {
    uint8_t total = 0;

    // Iterate through pieces.
    Bitboard pieces =
            board->pieces[board->turn][STUDENT] | board->pieces[board->turn][MASTER];
    Bitboard targets = ~pieces;
    while (pieces) {
        uint32_t mask = pieces & -pieces;

        // Iterate through cards.
        for (int card_index = 0; card_index < CARDS_EACH_NUM; ++card_index) {
            // Add move squares.
            total += __builtin_popcount(MOVE_TABLES[board->cards[board->turn][card_index]]
                                                   [__builtin_ctz(mask)][board->turn] &
                                        targets);
        }

        pieces ^= mask;
    }

    return total;
}
