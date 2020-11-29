#include "evaluate.h"
#include "move_tables.h"

constexpr int MATERIAL = 30, TEMPO = 3;

int evaluate(Board *board) {
    int eval = __builtin_popcount(board->pieces[0][STUDENT] | board->pieces[0][MASTER]) -
               __builtin_popcount(board->pieces[1][STUDENT] | board->pieces[1][MASTER]);
    eval *= MATERIAL;

    for (uint8_t turn = 0; turn < PLAYERS_NUM; ++turn) {
        Bitboard squares = 0;
        Bitboard pieces = board->pieces[turn][STUDENT] | board->pieces[turn][MASTER];
        Bitboard targets = ~pieces;
        while (pieces != 0) {
            uint32_t mask = pieces & -pieces;

            // Iterate through cards.
            for (int card_index = 0; card_index < CARDS_EACH_NUM; ++card_index) {
                squares |= MOVE_TABLES[(board->cards[turn][card_index] * SQUARE_NUM +
                                        __builtin_ctz(mask)) *
                                               PLAYERS_NUM +
                                       turn] &
                           targets;
            }

            pieces ^= mask;
        }
        if (turn) {
            eval -= __builtin_popcount(squares);
        } else {
            eval += __builtin_popcount(squares);
        }
    }

    if (board->turn) {
        eval -= TEMPO;
    } else {
        eval += TEMPO;
    }

    return eval;
}
