#include <algorithm>
#include <ctime>
#include <string>
#include <cmath>
#include <utility>

#include "board.h"
#include "make_move.h"
#include "move_bits.h"
#include "move_gen.h"
#include "move_tables.h"
#include "search.h"

constexpr uint8_t MAX_DEPTH = 30;
constexpr int MIN_EVAL = -1000, TEMPO = 3, WINDOW = 6;

uint64_t nodes = 0;

Move ALL_MOVES[MAX_DEPTH][MAX_MOVES];

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

int quiescence(Board *board, int alpha, int beta, int ply) {
    ++nodes;

    // Leaf.
    if (board->game_over())
        return MIN_EVAL + ply;

    // Evaluate.
    int value = eval(board) * (board->turn ? -1 : 1);
    if (value >= beta)
        return beta;
    if (value > alpha)
        alpha = value;

    if (ply >= MAX_DEPTH)
        return alpha;

    Move *moves = ALL_MOVES[ply];
    uint8_t size = gen_moves(
            board, moves,
            (board->pieces[!board->turn][STUDENT] | board->pieces[!board->turn][MASTER]));
    for (int i = 0; i < size; ++i) {
        const Move move = moves[i];
        make_move(board, move);
        value = -quiescence(board, -beta, -alpha, ply + 1);
        undo_move(board, move);

        if (value >= beta)
            return beta;
        if (value > alpha)
            alpha = value;
    }

    return alpha;
}

int negamax(Board *board, int depth, int alpha, int beta, int ply, Move *best_move) {
    // Leaf.
    if (board->game_over()) {
        ++nodes;
        return MIN_EVAL + ply;
    }
    if (depth == 0)
        return quiescence(board, alpha, beta, ply);

    int value = MIN_EVAL;
    Move *moves = ALL_MOVES[ply];
    uint8_t size = gen_moves(board, moves);

    // IID.
    if (depth > 4) {
        Move *iid_move = new Move;
        negamax(board, depth / 4, alpha, beta, ply, iid_move);
        for (int i = 0; i < size; ++i) {
            if (moves[i] == *iid_move) {
                std::swap(moves[0], moves[i]);
                break;
            }
        }
    }

    // Move loop.
    for (int i = 0; i < size; ++i) {
        const Move move = moves[i];
        make_move(board, move);
        int new_value = -negamax(board, depth - 1, -beta, -alpha, ply + 1, nullptr);
        undo_move(board, move);

        if (value < new_value) {
            value = new_value;
            if (best_move) {
                *best_move = move;
            }
        }

        if (value > alpha)
            alpha = value;
        if (alpha >= beta)
            break;
    }

    return value;
}


Move search(Board *board) {
    clock_t start = clock();

    Move best_move;

    int depth = 1, alpha = MIN_EVAL, beta = -MIN_EVAL;
    while (depth <= MAX_DEPTH) {
        Move *root_move = new Move;
        int value = negamax(board, depth, alpha, beta, 0, root_move);

        float elapsed = (std::clock() - start) / static_cast<float>(CLOCKS_PER_SEC);

        // Window.
        if (value <= alpha || value >= beta) {
            alpha = MIN_EVAL, beta = -MIN_EVAL;
        } else {
            alpha = value - WINDOW;
            beta = value + WINDOW;

            best_move = *root_move;

            int mate_plies = std::abs(MIN_EVAL + std::abs(value));

            // Output.
            std::string value_str;
            if (mate_plies > MAX_DEPTH) {
                value_str = std::to_string(value);
            } else {
                int mate_depth =
                        static_cast<int>(std::copysign((mate_plies + 1) / 2, value));
                value_str = "#" + std::to_string(mate_depth);
            }
            printf("Depth %2i: Move = %13s, Evaluation =%5s (%.3fs)\n", depth,
                   move_string(board, best_move).c_str(), value_str.c_str(), elapsed);

            // End search by mate detection.
            if (mate_plies <= depth)
                break;
            ++depth;
        }

        // End search by timeout.
        if (depth > 1 && elapsed > 6)
            break;
    }
    return best_move;
}
