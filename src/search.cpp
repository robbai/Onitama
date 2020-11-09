#include <algorithm>
#include <ctime>

#include "board.h"
#include "make_move.h"
#include "move_gen.h"
#include "move_tables.h"
#include "search.h"

constexpr uint8_t MAX_PERFT = 8;
constexpr int MIN_EVAL = -1000, TEMPO = 3;

uint64_t nodes = 0;

Move PERFT_MOVES[MAX_PERFT][MAX_MOVES];

int eval(Board *board) {
    int eval = __builtin_popcount(board->pieces[0][STUDENT] | board->pieces[0][MASTER]) -
               __builtin_popcount(board->pieces[1][STUDENT] | board->pieces[1][MASTER]);
    eval *= 30;

    for (int turn = 0; turn < PLAYERS_NUM; ++turn) {
        Bitboard squares = 0;
        Bitboard pieces = board->pieces[turn][STUDENT] | board->pieces[turn][MASTER];
        Bitboard targets = ~pieces;
        while (pieces != 0) {
            uint32_t mask = pieces & -pieces;

            // Iterate through cards.
            for (int card_index = 0; card_index < CARDS_EACH_NUM; ++card_index) {
                squares |= MOVE_TABLES[board->cards[turn][card_index]]
                                      [__builtin_ctz(mask)][turn] &
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

int negamax(Board *board, int depth, int alpha, int beta, int ply = 0) {
    ++nodes;

    // Leaf.
    if (board->game_over())
        return MIN_EVAL + ply;
    if (depth == 0)
        return eval(board) * (board->turn ? -1 : 1);

    int value = MIN_EVAL;

    Move *moves = PERFT_MOVES[ply];
    uint8_t size = gen_moves(board, moves);
    for (int i = 0; i < size; ++i) {
        make_move(board, moves[i]);
        value = std::max(value, -negamax(board, depth - 1, -beta, -alpha, ply + 1));
        undo_move(board, moves[i]);

        alpha = std::max(alpha, value);
        if (alpha >= beta)
            break;
    }

    return value;
}


void search(Board *board) {
    clock_t start = clock();
    for (int depth = 0; 1; ++depth) {
        int eval = negamax(board, depth, MIN_EVAL, -MIN_EVAL);
        printf("Depth %i: Evaluation = %i\n", depth, eval);

        int elapsed = (clock() - start) / CLOCKS_PER_SEC;
        if (elapsed > 10)
            break;
    }
}
