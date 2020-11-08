#include "move_gen.h"
#include "move_tables.h"


uint8_t gen_moves(Board *board, Move *moves) {
    uint8_t total = 0;

    // Iterate through pieces.
    Bitboard pieces = board->pieces[board->turn][STUDENT] | board->pieces[board->turn][MASTER];
    Bitboard targets = ~pieces;
    while (pieces != 0) {
        uint32_t mask = pieces & -pieces;

        // Iterate through cards.
        for (int card_index = 0; card_index < CARDS_EACH_NUM; ++card_index) {
            // Iterate through move squares.
            Bitboard squares = MOVE_TABLES[board->cards[board->turn][card_index]][__builtin_ctz(mask)][board->turn] & targets;
            while (squares != 0) {
                uint32_t mask_2 = squares & -squares;

                Move move = (mask | mask_2) | (card_index << SQUARE_NUM);
                moves[total] = move;
                ++total;

                squares ^= mask_2;
            }
        }

        pieces ^= mask;
    }

    return total;
}
