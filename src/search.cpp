#include <algorithm>
#include <ctime>
#include <string>
#include <cmath>
#include <utility>
#include <cstring>

#include "board.h"
#include "make_move.h"
#include "move_bits.h"
#include "move_gen.h"
#include "search.h"
#include "tb/tb_gen.h"
#include "tb/tb_probe.h"
#include "tt/ttable.h"
#include "evaluate.h"

constexpr int MIN_EVAL = -10000, WINDOW = 20;

uint64_t nodes = 0;
uint64_t tb_hits = 0;
uint64_t tt_hits = 0;

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

Board get_pv_leaf(Board board) {
    Move moves[MAX_MOVES];
    for (int i = 0; i < pv_line.length; ++i) {
        Move move = pv_line.moves[i];
        uint8_t size = gen_moves(&board, moves);
        for (int j = 0; j < size; j++) {
            if (moves[j] == move) {
                make_move(&board, move);
                break;
            }
        }
    }
    return board;
}

int search(Board *board, int depth, int alpha, int beta, int ply, bool check_tb,
           bool following_pv, Line *curr_line) {
    int alpha_original = alpha;

    // Terminal.
    if (board->game_over()) {
        ++nodes;
        return MIN_EVAL + board->move_count;
    }

    // Probe TB.
    if (check_tb) {
        uint8_t students_left = __builtin_popcount(board->pieces[WHITE][STUDENT] |
                                                   board->pieces[BLACK][STUDENT]);
        if (students_left <= Tablebase::STUDENT_MEN) {
            TBEntry entry = probe_tb(board);
            ++nodes;
            ++tb_hits;
            switch (entry.state) {
                case WIN:
                    return -MIN_EVAL - board->move_count - entry.iter;
                case LOSS:
                    return MIN_EVAL + board->move_count + entry.iter;
                default:
                    return 0;
            }
        }
    }

    // Probe TT.
    TTEntry *entry = TTABLE.probe(board);
    uint32_t remaining_hash = (board->hash >> 32);
    bool hash_match = (entry->remaining_hash == remaining_hash);
    if ((ply || curr_line->length) && hash_match && entry->depth >= depth) {
        ++tt_hits;
        switch (entry->type) {
            case EXACT:
                ++nodes;
                return entry->value;
            case LOWER:
                alpha = std::max(alpha, entry->value);
                break;
            case UPPER:
                beta = std::min(beta, entry->value);
                break;
        }
        if (alpha >= beta) {
            ++nodes;
            return entry->value;
        }
    }

    // Leaf.
    if (depth == 0)
        return q_search(board, alpha, beta, ply);

    ++nodes;

    Line line;
    Move *moves = ALL_MOVES[ply];
    uint8_t size = gen_moves(board, moves);

    // Move ordering.
    uint8_t bump = 0;
    if (following_pv) {
        // PV ordering.
        following_pv = bump_move(moves, size, pv_line.moves[ply]);
        if (following_pv)
            ++bump;
    }
    if (entry->remaining_hash == remaining_hash) {
        // TT ordering.
        if (bump_move(moves, size, entry->move, bump))
            ++bump;
    }
    if (depth > 4 && !bump) {
        // IID ordering.
        search(board, depth / 4, alpha, beta, ply, false, following_pv, &line);
        bump_move(moves, size, line.moves[0], bump);
    }

    // Move loop.
    Move best_move;
    int best_value = MIN_EVAL;
    for (uint8_t i = 0; i < size; ++i) {
        const Move move = moves[i];
        make_move(board, move);
        int value = -search(board, depth - 1, -beta, -alpha, ply + 1,
                            MoveBits::capture(move) || !ply, following_pv, &line);
        undo_move(board, move);

        if (value > best_value) {
            best_value = value;
            best_move = move;
            if (value > alpha) {
                if (value >= beta)
                    break;  // Cut-off.
                alpha = value;
                curr_line->moves[0] = move;
                memcpy(curr_line->moves + 1, line.moves, line.length * sizeof(Move));
                curr_line->length = line.length + 1;
            }
        }
    }

    // Store in TT.
    if (!hash_match || entry->depth < depth) {
        entry->value = best_value;
        if (best_value <= alpha_original) {
            entry->type = UPPER;
        } else if (best_value >= beta) {
            entry->type = LOWER;
        } else {
            entry->type = EXACT;
        }
        entry->depth = depth;
        entry->remaining_hash = remaining_hash;
        entry->move = best_move;
        entry->value = best_value;
    }

    return best_value;
}

int q_search(Board *board, int alpha, int beta, int ply) {
    ++nodes;

    // Terminal.
    if (board->game_over())
        return MIN_EVAL + board->move_count;

    // Evaluate.
    int value = evaluate(board) * (board->turn ? -1 : 1);
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
    for (uint8_t i = 0; i < size; ++i) {
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

Move start_search(Board *board, bool silent, float max_time) {
    // Setup.
    pv_line = {};
    nodes = 0;
    tb_hits = 0;
    tt_hits = 0;
    clock_t start = clock();
    int depth = 1, alpha = MIN_EVAL, beta = -MIN_EVAL;

    // Iterative deepening.
    while (depth <= MAX_DEPTH) {
        Line line;
        int value = search(board, depth, alpha, beta, 0, false, pv_line.length, &line);

        double elapsed = (std::clock() - start) / static_cast<double>(CLOCKS_PER_SEC);

        // Window.
        if (value <= alpha || value >= beta) {
            alpha = MIN_EVAL, beta = -MIN_EVAL;
        } else {
            alpha = value - WINDOW;
            beta = value + WINDOW;

            // Replace PV.
            pv_line = line;

            int mate_plies = std::abs(MIN_EVAL + board->move_count + std::abs(value));

            // Output.
            if (!silent) {
                std::string value_str;
                if (mate_plies > MAX_DEPTH + 255) {
                    value_str = std::to_string(value);
                } else {
                    int mate_depth =
                            static_cast<int>(std::copysign((mate_plies + 1) / 2, value));
                    value_str = "#" + std::to_string(mate_depth);
                }
                printf("Depth %2i: Evaluation =%5s, Nodes = %10llu, TB-hits = %8llu, "
                       "TT-hits "
                       "= %8llu, %.3fs, "
                       "PV = [%s]\n",
                       depth, value_str.c_str(), nodes, tb_hits, tt_hits, elapsed,
                       verify_pv(board, &pv_line, depth).c_str());
            }

            // End search by mate detection.
            if (mate_plies <= depth)
                break;
            ++depth;
        }

        // End search by timeout.
        if (elapsed > max_time)
            break;
    }
    return pv_line.moves[0];
}
