#include <algorithm>
#include <ctime>
#include <string>
#include <cmath>
#include <utility>
#include <cstring>
#include <cassert>

#include "board.h"
#include "make_move.h"
#include "move_bits.h"
#include "move_gen.h"
#include "search.h"
#include "tb/tb_gen.h"
#include "tb/tb_probe.h"
#include "tt/ttable.h"
#include "evaluate.h"

constexpr int MIN_EVAL = -100000, WINDOW = 610;

uint64_t nodes = 0;
uint64_t tb_hits = 0;
uint64_t tt_hits = 0;

Line pv_line;

Move ALL_MOVES[MAX_DEPTH][MAX_MOVES];

uint16_t HISTORY[PLAYERS_NUM][SQUARE_NUM][SQUARE_NUM][PIECE_TYPES_NUM];
int SORT_VALUE[MAX_MOVES];

int q_search(Board *board, int alpha, int beta, int ply);
void sort_moves(Board *board, Move *moves, uint8_t size, uint8_t bump);

enum Stage : uint8_t { PV, TT, CAPTURE, QUIET, STAGE_NUM };

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
           bool pv_node, bool following_pv, Line *curr_line) {
    int alpha_original = alpha;
    bool root = !ply;

    // Terminal.
    if (board->game_over()) {
        ++nodes;
        return MIN_EVAL + board->move_count;
    }

    // Win in one.
    if (!root && board->has_winning_move()) {
        ++nodes;
        return -(MIN_EVAL + board->move_count + 1);
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

    // Leaf.
    if (depth == 0)
        return q_search(board, alpha, beta, ply);
    ++nodes;

    // Mate-distance pruning.
    if (!root) {
        alpha = std::max(alpha, MIN_EVAL + board->move_count);
        beta = std::min(beta, -MIN_EVAL - board->move_count - 1);
        if (alpha >= beta)
            return alpha;
    }

    // Probe TT.
    TTEntry *entry = TTABLE.probe(board);
    uint32_t remaining_hash = (board->hash >> 32);
    bool hash_match = (entry->remaining_hash == remaining_hash);
    if (hash_match) {
        ++tt_hits;
        if (!pv_node && entry->depth >= depth) {
            switch (entry->type) {
                case EXACT:
                    return entry->value;
                case LOWER:
                    alpha = std::max(alpha, entry->value);
                    break;
                case UPPER:
                    beta = std::min(beta, entry->value);
                    break;
            }
            if (alpha >= beta)
                return entry->value;
        }
    }

    // Prepare branching.
    Line line;
    Move best_move;
    int best_value = MIN_EVAL;

    // Stage loop.
    Move *moves = ALL_MOVES[ply];
    int8_t move_num = -1;
    bool filtered_pv_move = true, filtered_tt_move = true;
    for (uint8_t stage = CAPTURE; stage < STAGE_NUM; ++stage) {
        uint8_t size = 0;
        switch (stage) {
            case PV:
                if (following_pv) {
                    if (move_exists(board, pv_line.moves[ply])) {
                        moves[0] = pv_line.moves[ply];
                        size = 1;
                        filtered_pv_move = false;
                    } else {
                        following_pv = false;
                    }
                }
                break;
            case TT:
                if (hash_match && entry->move != pv_line.moves[ply] &&
                    move_exists(board, entry->move)) {
                    moves[0] = entry->move;
                    size = 1;
                    filtered_tt_move = false;
                }
                break;
            default:
                // Captures and quiets.
                Bitboard targets = (board->pieces[!board->turn][STUDENT] |
                                    board->pieces[!board->turn][MASTER]);
                if (stage == QUIET)
                    targets = ~targets;
                size = gen_moves(board, moves, targets);

                // Filter out PV and TT moves.
                for (uint8_t i = 0; i < size; ++i) {
                    if (filtered_pv_move && filtered_tt_move)
                        break;
                    if (!filtered_pv_move && moves[i] == pv_line.moves[ply]) {
                        std::swap(moves[i], moves[size - 1]);
                        --size;
                        filtered_pv_move = true;
                    } else if (!filtered_tt_move && moves[i] == entry->move) {
                        std::swap(moves[i], moves[size - 1]);
                        --size;
                        filtered_tt_move = true;
                    }
                }
                assert(filtered_pv_move);
                assert(filtered_tt_move);

                // Sort quiets.
                if (stage == QUIET)
                    sort_moves(board, moves, size, 0);

                break;
        }

        // Move loop.
        for (uint8_t move_index = 0; move_index < size; ++move_index) {
            ++move_num;
            const Move move = moves[move_index];
            assert(move_exists(board, move));

            uint8_t reduction = (move_num < 5 || pv_node ? 0 : 1 + depth / 3);  // LMR.
            if (reduction && reduction >= depth - 1) {
                // History pruning.
                if (!following_pv) {
                    uint8_t from = MoveBits::from(move);
                    uint8_t to = MoveBits::to(move);
                    bool piece_type = MoveBits::piece_type(move);
                    if (!HISTORY[board->turn][from][to][piece_type])
                        continue;
                }
            }

            if (reduction > depth - 1)
                reduction = depth - 1;

            make_move(board, move);

            int value;
            if (!move_num) {
                value = -search(board, depth - 1, -beta, -alpha, ply + 1,
                                root || MoveBits::capture(move), pv_node, following_pv,
                                &line);
            } else {
                value = -search(board, depth - 1 - reduction, -alpha - 1, -alpha, ply + 1,
                                root || MoveBits::capture(move), false, following_pv,
                                &line);
                if (value > alpha) {
                    if (reduction)
                        value = -search(board, depth - 1, -alpha - 1, -alpha, ply + 1,
                                        root || MoveBits::capture(move), pv_node,
                                        following_pv, &line);
                    if (value > alpha)
                        value = -search(board, depth - 1, -beta, -alpha, ply + 1,
                                        root || MoveBits::capture(move), pv_node,
                                        following_pv, &line);
                }
            }

            undo_move(board, move);

            if (value > best_value) {
                best_value = value;
                best_move = move;
                if (value > alpha) {
                    curr_line->moves[0] = move;
                    memcpy(curr_line->moves + 1, line.moves, line.length * sizeof(Move));
                    curr_line->length = line.length + 1;

                    // Cut-off.
                    if (value >= beta) {
                        // History heuristic.
                        if (!MoveBits::capture(move)) {
                            uint8_t from = MoveBits::from(move);
                            uint8_t to = MoveBits::to(move);
                            bool piece_type = MoveBits::piece_type(move);
                            HISTORY[board->turn][from][to][piece_type] += depth * depth;
                        }

                        break;
                    }

                    alpha = value;
                }
            }
        }

        if (best_value >= beta)
            break;
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

    // Winning-move ordering.
    int bump = 0;
    for (uint8_t i = bump; i < size; ++i) {
        // Winning-move ordering.
        if (board->winning_move(moves[i]) && bump_move(moves, size, moves[i], bump))
            ++bump;
    }

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

void reset_history() {
    for (uint8_t i = 0; i < PLAYERS_NUM; ++i)
        for (uint8_t j = 0; j < SQUARE_NUM; ++j)
            for (uint8_t k = 0; k < SQUARE_NUM; ++k)
                for (uint8_t l = 0; l < PIECE_TYPES_NUM; ++l)
                    HISTORY[i][j][k][l] = 0;
}

void sort_moves(Board *board, Move *moves, uint8_t size, uint8_t bump) {
    for (uint8_t i = bump; i < size; ++i) {
        Move move = moves[i];

        // History heuristic.
        uint8_t from = MoveBits::from(move);
        uint8_t to = MoveBits::to(move);
        bool piece_type = MoveBits::piece_type(move);
        SORT_VALUE[i] = HISTORY[board->turn][from][to][piece_type];
    }

    quicksort(moves, SORT_VALUE, bump, size);
}

Move start_search(Board *board, bool silent, float max_time) {
    // Setup.
    pv_line = {};
    nodes = 0;
    tb_hits = 0;
    tt_hits = 0;
    reset_history();
    clock_t start = clock();
    int depth = 1, alpha = MIN_EVAL, beta = -MIN_EVAL;

    // Iterative deepening.
    while (depth <= MAX_DEPTH) {
        Line line;
        int value =
                search(board, depth, alpha, beta, 0, false, true, pv_line.length, &line);

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
                    value_str = std::to_string(to_centi(value));
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
