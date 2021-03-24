#include <algorithm>
#include <ctime>
#include <string>
#include <cmath>
#include <utility>
#include <cstring>
#include <thread>
#include <vector>
#include <chrono>

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

uint8_t root_size = 0;

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

Board Thread::get_pv_leaf(Board board) {
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

int Thread::search(Board *board, int depth, int alpha, int beta, int ply, bool check_tb,
                   bool pv_node, bool following_pv, Line *curr_line) {
    if (stop)
        return 0;

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
    if (hash_match && !root) {
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

    Line line;

    Move *moves = move_lists[ply];
    uint8_t size;
    if (root) {
        if (depth == 1) {
            size = gen_moves(board, moves);
            root_size = size;
        } else {
            size = root_size;
        }
    } else {
        size = gen_moves(board, moves);
    }

    // Move ordering.
    uint8_t bump = 0;
    if (!root || !th) {
        if (following_pv) {
            // PV ordering.
            following_pv = bump_move(moves, size, pv_line.moves[ply]);
            if (following_pv)
                ++bump;
        }
        if (hash_match) {
            // TT ordering.
            if (bump_move(moves, size, entry->move, bump))
                ++bump;
        }
        if (root) {
            for (uint8_t i = bump; i < size; ++i) {
                // Winning-move ordering.
                if (board->winning_move(moves[i]) &&
                    bump_move(moves, size, moves[i], bump))
                    ++bump;
            }
        }
        for (uint8_t i = bump; i < size; ++i) {
            // Capture ordering.
            if (MoveBits::capture(moves[i]) && bump_move(moves, size, moves[i], bump))
                ++bump;
        }
        sort_moves(board, moves, size, bump);  // Quiet ordering (history).
    }

    // Move loop.
    Move best_move;
    int best_value = MIN_EVAL;
    for (uint8_t i = 0; i < size; ++i) {
        const Move move = moves[i];

        uint8_t reduction = (i < 5 || pv_node ? 0 : 1 + depth / 3);  // LMR.
        if (reduction && reduction >= depth - 1) {
            // History pruning.
            if (!following_pv) {
                uint8_t from = MoveBits::from(move);
                uint8_t to = MoveBits::to(move);
                bool piece_type = MoveBits::piece_type(move);
                if (!history[board->turn][from][to][piece_type])
                    continue;
            }
        }

        if (reduction > depth - 1)
            reduction = depth - 1;

        make_move(board, move);

        int value;
        bool now_check_tb = (root || MoveBits::capture(move)) && GENERATED_TB;
        if (!i) {
            value = -search(board, depth - 1, -beta, -alpha, ply + 1, now_check_tb,
                            pv_node, following_pv, &line);
        } else {
            value = -search(board, depth - 1 - reduction, -alpha - 1, -alpha, ply + 1,
                            now_check_tb, false, following_pv, &line);
            if (value > alpha) {
                if (reduction)
                    value = -search(board, depth - 1, -alpha - 1, -alpha, ply + 1,
                                    now_check_tb, pv_node, following_pv, &line);
                if (value > alpha)
                    value = -search(board, depth - 1, -beta, -alpha, ply + 1,
                                    now_check_tb, pv_node, following_pv, &line);
            }
        }

        undo_move(board, move);

        if (stop)
            return 0;

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
                    if (!MoveBits::capture(moves[i])) {
                        uint8_t from = MoveBits::from(move);
                        uint8_t to = MoveBits::to(move);
                        bool piece_type = MoveBits::piece_type(move);
                        history[board->turn][from][to][piece_type] += depth * depth;
                    }

                    break;
                }

                alpha = value;
            }
        }
    }

    if (stop)
        return 0;

    // Store in TT.
    if (entry->depth <= depth) {
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

int Thread::q_search(Board *board, int alpha, int beta, int ply) {
    if (stop)
        return 0;

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

    Move *moves = move_lists[ply];
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

        if (stop)
            return 0;

        if (value >= beta)
            return beta;
        if (value > alpha)
            alpha = value;
    }

    return alpha;
}

void Thread::reset_history() {
    for (uint8_t i = 0; i < PLAYERS_NUM; ++i)
        for (uint8_t j = 0; j < SQUARE_NUM; ++j)
            for (uint8_t k = 0; k < SQUARE_NUM; ++k)
                for (uint8_t l = 0; l < PIECE_TYPES_NUM; ++l)
                    history[i][j][k][l] = 0;
}

void Thread::sort_moves(Board *board, Move *moves, uint8_t size, uint8_t bump) {
    for (uint8_t i = bump; i < size; ++i) {
        Move move = moves[i];

        // History heuristic.
        uint8_t from = MoveBits::from(move);
        uint8_t to = MoveBits::to(move);
        bool piece_type = MoveBits::piece_type(move);
        sort_values[i] = history[board->turn][from][to][piece_type];
    }

    quicksort(moves, sort_values, bump, size);
}

void start_helpers(Thread *threads, std::vector<std::thread> *helpers,
                   uint8_t num_threads, Board *board, int depth, int alpha, int beta) {
    for (uint8_t th = 1; th < num_threads; ++th) {
        Thread *thread = &threads[th];
        thread->stop = false;

        // Copy root moves.
        for (uint8_t m = 0; m < root_size; ++m)
            thread->move_lists[0][m] = threads[0].move_lists[0][m];

        auto search = [th, board, depth, alpha, beta](Thread *thread) {
            Line line = {};
            Board thread_board = board->copy();
            int value = thread->search(&thread_board, depth + (th - 1) / 2, alpha, beta,
                                       0, false, true, thread->pv_line.length, &line);
            if (!thread->stop && alpha < value && value < beta)
                thread->pv_line = line;
        };
        std::thread helper = std::thread(search, thread);
        helpers->push_back(std::move(helper));
    }
}

void end_helpers(Thread *threads, std::vector<std::thread> *helpers,
                 uint8_t num_threads) {
    for (uint8_t th = (num_threads - 1); th > 0; --th) {
        threads[th].stop = true;
        helpers->back().join();
        helpers->pop_back();
    }
}

Move start_search(Board *board, float search_time, bool silent, uint8_t num_threads) {
    auto search = [&board, silent, num_threads](Thread *threads) {
        // Setup threads.
        std::vector<std::thread> helpers;
        for (uint8_t th = 0; th < num_threads; ++th) {
            Thread *thread = &threads[th];
            thread->th = th;
            thread->stop = false;
            thread->pv_line = {};
            thread->reset_history();
        }

        // Setup main search.
        nodes = 0;
        tb_hits = 0;
        tt_hits = 0;
        clock_t start = clock();
        int depth = 1, alpha = MIN_EVAL, beta = -MIN_EVAL;

        // Iterative deepening.
        while (depth <= MAX_DEPTH) {
            Line line;
            int value;

            if (depth == 1) {
                value = threads[0].search(board, depth, alpha, beta, 0, false, true,
                                          threads[0].pv_line.length, &line);
            } else {
                start_helpers(threads, &helpers, num_threads, board, depth, alpha, beta);
                value = threads[0].search(board, depth, alpha, beta, 0, false, true,
                                          threads[0].pv_line.length, &line);
                end_helpers(threads, &helpers, num_threads);
            }

            // End search by timeout.
            if (threads[0].stop) {
                if (depth == 1)
                    threads[0].pv_line = line;
                break;
            }

            double elapsed = (std::clock() - start) / static_cast<double>(CLOCKS_PER_SEC);

            // Window.
            if (value <= alpha || value >= beta) {
                alpha = MIN_EVAL, beta = -MIN_EVAL;
            } else {
                alpha = value - WINDOW;
                beta = value + WINDOW;

                // Replace PV.
                threads[0].pv_line = line;

                int mate_plies = std::abs(MIN_EVAL + board->move_count + std::abs(value));

                // Output.
                if (!silent) {
                    std::string value_str;
                    if (mate_plies > MAX_DEPTH + 255) {
                        value_str = std::to_string(to_centi(value));
                    } else {
                        int mate_depth = static_cast<int>(
                                std::copysign((mate_plies + 1) / 2, value));
                        value_str = "#" + std::to_string(mate_depth);
                    }
                    printf("Depth %2i: Evaluation =%5s, Nodes = %10llu, TB-hits = %8llu, "
                           "TT-hits "
                           "= %8llu, %.3fs, "
                           "PV = [%s]\n",
                           depth, value_str.c_str(), nodes, tb_hits, tt_hits, elapsed,
                           verify_pv(board, &threads[0].pv_line, depth).c_str());
                }

                ++depth;
            }
        }
    };

    Thread threads[num_threads];
    std::thread search_thread(search, &threads[0]);
    auto sleep_time = std::chrono::milliseconds(static_cast<int>(search_time * 1000));
    std::this_thread::sleep_for(sleep_time);
    threads[0].stop = true;
    if (search_thread.joinable())
        search_thread.join();

    return threads[0].pv_line.moves[0];
}
