#include "move_gen.h"

#include "move_bits.h"
#include "move_tables.h"

uint8_t gen_moves(Board *board, Move *moves, Bitboard targets) {
    uint8_t total = 0;

    // Iterate through pieces.
    Bitboard pieces =
            board->pieces[board->turn][STUDENT] | board->pieces[board->turn][MASTER];
    targets &= ~pieces;
    while (pieces) {
        int from = __builtin_ctz(pieces);
        Bitboard mask_1 = (1u << from);

        // Iterate through cards.
        for (int card_index = 0; card_index < CARDS_EACH_NUM; ++card_index) {
            // Iterate through move squares and create moves.
            Bitboard squares = MOVE_TABLES[board->cards[board->turn][card_index]][from]
                                          [board->turn] &
                               targets;
            Move move = MoveBits::half_create_move(
                    mask_1, card_index, mask_1 & board->pieces[board->turn][MASTER]);
            while (squares) {
                Bitboard mask_2 = squares & -squares;

                moves[total] = MoveBits::finish_create_move(
                        move, mask_2, mask_2 & board->pieces[!board->turn][STUDENT]);
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
        int from = __builtin_ctz(pieces);

        // Iterate through cards.
        for (int card_index = 0; card_index < CARDS_EACH_NUM; ++card_index) {
            // Add move squares.
            total += __builtin_popcount(MOVE_TABLES[board->cards[board->turn][card_index]]
                                                   [from][board->turn] &
                                        targets);
        }

        pieces ^= (1u << from);
    }

    return total;
}
