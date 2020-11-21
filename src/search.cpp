#include <algorithm>
#include <ctime>
#include <string>
#include <cmath>
#include <utility>
#include <cstring>
#include <iostream>

#include "board.h"
#include "make_move.h"
#include "move_bits.h"
#include "move_gen.h"
#include "move_tables.h"
#include "search.h"

constexpr int MIN_EVAL = -1000, TEMPO = 3, WINDOW = 6;

uint8_t root_move_count = 0;
uint64_t nodes = 0;

Line pv_line;

Move ALL_MOVES[MAX_DEPTH][MAX_MOVES];

int q_search(Board *board, int alpha, int beta, int ply);

std::string verify_pv(Board *board, Line *line, int depth) {
    int count = 0;
    std::string pv = "";

    Move moves[MAX_MOVES];
    depth = std::min(depth, line->length);
    for (int i = 0; i < depth; ++i) {
        Move move = line->moves[i];
        uint8_t size = gen_moves(board, moves);
        for (int j = 0; j < size; j++) {
            if (moves[j] == move) {
                pv += move_string(board, move) + ", ";
                make_move(board, move);
                ++count;
                break;
            }
        }
        if (count == i)
            break;
    }
    for (int i = (count - 1); i >= 0; --i)
        undo_move(board, line->moves[i]);

    line->length = count;

    return pv.substr(0, pv.length() - 2);
}

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

int search(Board *board, int depth, int alpha, int beta, int ply, bool following_pv,
           Line *curr_line) {
    // Leaf.
    if (board->game_over()) {
        ++nodes;
        return MIN_EVAL + ply;
    }
    if (depth == 0)
        return q_search(board, alpha, beta, ply);

    Line line;
    Move *moves = ALL_MOVES[ply];
    uint8_t size = gen_moves(board, moves);

    // PV ordering.
    if (following_pv) {
        following_pv = false;
        for (int i = 0; i < size; ++i) {
            if (moves[i] == pv_line.moves[ply]) {
                std::swap(moves[0], moves[i]);
                following_pv = true;
                goto move_loop;
            }
        }
    }

    // IID ordering.
    if (depth > 4) {
        search(board, depth / 4, alpha, beta, ply, following_pv, &line);
        for (int i = 0; i < size; ++i) {
            if (moves[i] == line.moves[0]) {
                std::swap(moves[0], moves[i]);
                break;
            }
        }
    }

    // Move loop.
move_loop:
    for (int i = 0; i < size; ++i) {
        const Move move = moves[i];
        make_move(board, move);
        int value =
                -search(board, depth - 1, -beta, -alpha, ply + 1, following_pv, &line);
        undo_move(board, move);

        if (value >= beta)
            return beta;
        if (value > alpha) {
            alpha = value;
            curr_line->moves[0] = move;
            memcpy(curr_line->moves + 1, line.moves, line.length * sizeof(Move));
            curr_line->length = line.length + 1;
        }
    }

    return alpha;
}

int q_search(Board *board, int alpha, int beta, int ply) {
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
        value = -q_search(board, -beta, -alpha, ply + 1);
        undo_move(board, move);

        if (value >= beta)
            return beta;
        if (value > alpha)
            alpha = value;
    }

    return alpha;
}

Move start_search(Board *board) {
    // Setup.
    uint8_t moves_made = (board->move_count - root_move_count);
    root_move_count += moves_made;
    pv_line.length -= moves_made;
    for (int i = 0; i < pv_line.length; ++i)
        pv_line.moves[i] = pv_line.moves[i + moves_made];
    std::cout << "Found PV: [" << verify_pv(board, &pv_line, pv_line.length) << "]"
              << std::endl;
    nodes = 0;
    clock_t start = clock();
    int depth = 1, alpha = MIN_EVAL, beta = -MIN_EVAL;

    // Iterative deepening.
    while (depth <= MAX_DEPTH) {
        Line line;
        int value = search(board, depth, alpha, beta, 0, pv_line.length, &line);

        double elapsed = (std::clock() - start) / static_cast<double>(CLOCKS_PER_SEC);

        // Window.
        if (value <= alpha || value >= beta) {
            alpha = MIN_EVAL, beta = -MIN_EVAL;
        } else {
            alpha = value - WINDOW;
            beta = value + WINDOW;

            // Replace PV.
            if (line.moves[0] != pv_line.moves[0] || line.length > pv_line.length)
                pv_line = line;

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
            printf("Depth %2i: Evaluation =%5s, Nodes = %10llu, %.3fs, PV = [%s]\n",
                   depth, value_str.c_str(), nodes, elapsed,
                   verify_pv(board, &pv_line, depth).c_str());

            // End search by mate detection.
            if (mate_plies <= depth)
                break;
            ++depth;
        }

        // End search by timeout.
        if (depth > 1 && elapsed > 6)
            break;
    }
    return pv_line.moves[0];
}
