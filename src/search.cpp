#include <algorithm>
#include <ctime>
#include <string>
#include <cmath>
#include <thread>
#include <vector>
#include <utility>

#include "board.h"
#include "make_move.h"
#include "move_bits.h"
#include "move_gen.h"
#include "search.h"
#include "tb/tb_gen.h"
#include "tb/tb_probe.h"
#include "tt/ttable.h"
#include "evaluate.h"

constexpr int MIN_EVAL = -100000, WINDOW = 117;

uint64_t nodes = 0;
uint64_t tb_hits = 0;

uint8_t root_size = 0;

enum Stage : uint8_t { TT, CAPTURE, KILLER, QUIET, STAGE_NUM };
enum QStage : uint8_t { Q_RECAPTURE, Q_CAPTURE, Q_ALL, Q_STAGE_NUM };

int LMR_TABLE[MAX_DEPTH][MAX_MOVES];

namespace ProbCut {
    const int D = 5;
    const float a = 0.804429;
    const int T_sigma = 1369;
}  // namespace ProbCut

void init_search() {
    for (int depth = 0; depth < MAX_DEPTH; depth++) {
        for (int move_num = 0; move_num < MAX_MOVES; move_num++)
            LMR_TABLE[depth][move_num] =
                    (0.295232 + log(depth * 2.19754) * log(move_num * 2.89954) / 2.20876);
    }
}

Move get_tt_move(Board *board) {
    TTEntry *entry = TTABLE.probe(board);
    uint32_t remaining_hash = (board->hash >> 32);
    bool hash_match = (entry->remaining_hash == remaining_hash);
    return hash_match && move_exists(board, entry->move) ? entry->move : 0;
}

std::string verify_pv(Board *board, uint8_t depth) {
    // Make moves.
    std::string pv;
    Move moves[depth];
    uint8_t count = 0;
    for (uint8_t i = 0; i < depth; ++i) {
        const Move move = get_tt_move(board);
        if (!move_exists(board, move))
            break;
        moves[i] = move;
        pv += (i ? ", " : "") + move_string(board, move);
        make_move(board, move);
        ++count;
    }

    // Undo moves.
    for (uint8_t i = count; i > 0; --i) {
        const Move move = moves[i - 1];
        undo_move(board, move);
    }

    return pv;
}

bool is_mate_value(int value) {
    value = -abs(value);
    return value < MIN_EVAL + MAX_DEPTH + 255;
}

int Thread::search(Board *board, int depth, int alpha, int beta, int ply, bool cut_node,
                   Move last_move, bool check_tb) {
    if (stop)
        return 0;

    int alpha_original = alpha;
    bool root = !ply;
    bool pv_node = beta - alpha != 1;
    pv_ply[ply] = pv_node;

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
    if (check_tb && board->student_count <= Tablebase::STUDENT_MEN) {
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

    // Score repetitions in search as a draw.
    for (uint8_t p = (ply & 1); p < ply; p += 2) {
        if (this->hash_line[p] == board->hash && pv_ply[p] == pv_node) {
            ++nodes;
            return 0;
        }
    }
    this->hash_line[ply] = board->hash;

    // Mate-distance pruning.
    if (!root) {
        alpha = std::max(alpha, MIN_EVAL + board->move_count);
        beta = std::min(beta, -MIN_EVAL - board->move_count - 1);
        if (alpha >= beta) {
            ++nodes;
            return alpha;
        }
    }

    // Leaf.
    if (depth == 0)
        return q_search(board, alpha, beta, ply, last_move);
    ++nodes;

    // Probe TT.
    TTEntry *entry = TTABLE.probe(board);
    uint32_t remaining_hash = (board->hash >> 32);
    bool hash_match = (entry->remaining_hash == remaining_hash);
    if (hash_match && !root) {
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

    // ProbCut.
    if (!pv_node && depth == ProbCut::D) {
        int bound = (beta + ProbCut::T_sigma) / ProbCut::a;
        if (!is_mate_value(beta) && q_search(board, bound - 1, bound, ply) >= bound)
            return beta;

        bound = (alpha - ProbCut::T_sigma) / ProbCut::a;
        if (!is_mate_value(alpha) && q_search(board, bound, bound + 1, ply) <= bound)
            return alpha;
    }

    // Prepare branching.
    Move best_move;
    int best_value = MIN_EVAL, static_value = MIN_EVAL;

    if (depth > 5 && !(hash_match && move_exists(board, entry->move)))
        depth -= 2;

    // Stage loop.
    Move *moves = move_lists[ply];
    int8_t move_num = -1;
    bool searched_tt_move = false;
    for (uint8_t stage = TT; stage < STAGE_NUM; ++stage) {
        uint8_t size = 0;
        switch (stage) {
            case TT:
                if (hash_match && move_exists(board, entry->move)) {
                    moves[0] = entry->move;
                    size = 1;
                    searched_tt_move = true;
                }
                break;
            case KILLER:
                for (uint8_t k = 0; k < KILLER_NUM; ++k) {
                    Move killer_move = killers[ply][k];
                    if (move_exists(board, killer_move) &&
                        !is_killer(ply, killer_move, k)) {
                        moves[size] = killer_move;
                        ++size;
                    }
                }
                break;
            default:
                // Captures and quiets.
                Bitboard targets = (board->pieces[!board->turn][STUDENT] |
                                    board->pieces[!board->turn][MASTER]);
                if (stage == QUIET)
                    targets = ~targets;
                size = gen_moves(board, moves, targets);

                // Sort moves.
                sort_moves(board, moves, size, stage == CAPTURE, last_move);

                if (stage == QUIET && depth < 7)
                    static_value = evaluate(board, false);

                break;
        }

        // Move loop.
        for (uint8_t move_index = 0; move_index < size; ++move_index) {
            const Move move = moves[move_index];

            // Filter out PV and TT moves.
            if (stage >= CAPTURE) {
                if (move == entry->move)
                    continue;
                if (stage > KILLER && is_killer(ply, move))
                    continue;
            }

            ++move_num;

            // Reductions and pruning.
            int8_t reduction = 0;
            if (move_num) {
                // Futility prune.
                if (!pv_node && !is_mate_value(alpha) && !is_mate_value(beta) &&
                    stage == QUIET && MIN_EVAL != static_value &&
                    static_value < alpha - 568 * (depth + 1))
                    break;

                // Late-move reduction.
                if (depth > 2 && (stage == QUIET || !pv_node)) {
                    reduction = LMR_TABLE[depth][move_num];

                    Square from = MoveBits::from(move);
                    Square to = MoveBits::to(move);
                    bool piece_type = MoveBits::piece_type(move);
                    if (history[board->turn][from][to][piece_type] > 60)
                        reduction -= 1;

                    if (!pv_node)
                        reduction += 1;

                    if (cut_node)
                        reduction += 1;

                    if (reduction < 0)
                        reduction = 0;
                }
            }
            if (!board->student_delta && MoveBits::capture(move)) {
                // Capture extension.
                reduction -= 1;
            }
            if (reduction > depth - 1)
                reduction = depth - 1;

            make_move(board, move);

            int value;
            bool now_check_tb = (root || MoveBits::capture(move)) && GENERATED_TB;
            if (!move_num) {
                value = -search(board, depth - 1 - reduction, -beta, -alpha, ply + 1,
                                false, move, now_check_tb);
            } else {
                // Reductions and null window.
                value = -search(board, depth - 1 - reduction, -alpha - 1, -alpha, ply + 1,
                                true, move, now_check_tb);

                // Null window.
                if (value > alpha && reduction > 0) {
                    value = -search(board, depth - 1, -alpha - 1, -alpha, ply + 1,
                                    !cut_node, move, now_check_tb);
                }

                // Full search.
                if (value > alpha) {
                    value = -search(board, depth - 1 - (reduction > 0 ? 0 : reduction),
                                    -beta, -alpha, ply + 1, false, move, now_check_tb);
                }
            }

            undo_move(board, move);

            if (value > best_value) {
                best_value = value;
                best_move = move;
                if (value > alpha) {
                    // Cut-off.
                    if (value >= beta) {
                        if (!MoveBits::capture(move)) {
                            // History.
                            Square from = MoveBits::from(move);
                            Square to = MoveBits::to(move);
                            bool piece_type = MoveBits::piece_type(move);
                            history[board->turn][from][to][piece_type] += depth * depth;

                            // Counter-card.
                            counters[board->cards[board->turn][0]]
                                    [board->cards[board->turn][1]][board->side_card] =
                                            board->cards[board->turn]
                                                        [MoveBits::card_index(move)];

                            // Killer move.
                            for (uint8_t k = (KILLER_NUM - 1); k > 0; --k)
                                killers[ply][k] = killers[ply][k - 1];
                            killers[ply][0] = move;
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

    if (stop)
        return 0;

    // Store in TT.
    if (entry->depth <= depth || root) {
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

int Thread::q_search(Board *board, int alpha, int beta, int ply, Move last_move) {
    if (stop)
        return 0;

    ++nodes;

    // Terminal.
    if (board->game_over())
        return MIN_EVAL + board->move_count;

    // Win in one.
    if (board->has_winning_move())
        return -(MIN_EVAL + board->move_count + 1);

    // Evaluate.
    bool win_threat = board->has_winning_move(!board->turn);
    int value = (win_threat ? MIN_EVAL + board->move_count + 2
                            : evaluate(board) + 2 * (nodes & 5) - 5);
    if (value >= beta)
        return beta;

    // Delta prune (futility).
    if (!win_threat && value < alpha - 3512 && !is_mate_value(beta))
        return alpha;

    if (value > alpha)
        alpha = value;

    if (ply >= MAX_DEPTH)
        return alpha;

    Bitboard recapture = last_move ? (1u << MoveBits::to(last_move)) : 0;

    Move *moves = move_lists[ply];
    uint8_t size;
    for (uint8_t stage = Q_RECAPTURE; stage <= (win_threat ? Q_ALL : Q_CAPTURE);
         ++stage) {
        switch (stage) {
            case Q_RECAPTURE:
                size = gen_moves(board, moves, recapture);
                sort_moves(board, moves, size, true);
                break;
            case Q_CAPTURE:
                size = gen_moves(board, moves,
                                 board->pieces[!board->turn][STUDENT] & ~recapture);
                sort_moves(board, moves, size, true);
                break;
            default:
                size = gen_moves(board, moves, ~board->pieces[!board->turn][STUDENT]);
                sort_moves(board, moves, size, false);
                break;
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
    }

    return alpha;
}

void Thread::reset() {
    // History.
    for (auto &sq1 : history)
        for (auto &sq2 : sq1)
            for (auto &piece_type : sq2)
                for (uint16_t &hist : piece_type)
                    hist = 0;

    // Killers.
    for (auto &killer : killers)
        for (Move &move : killer)
            move = 0;

    // Counter-cards.
    for (auto &card1 : counters)
        for (auto &card2 : card1)
            for (auto &card3 : card2)
                card3 = CARD_NUM;
}

void Thread::sort_moves(Board *board, Move *moves, uint8_t size, bool captures,
                        Move last_move) {
    Square recapture_sq = last_move ? MoveBits::to(last_move) : SQUARE_NUM;

    // Assign scores.
    for (uint8_t i = 0; i < size; ++i) {
        const Move move = moves[i];
        Square to = MoveBits::to(move);

        if (captures) {
            sort_values[i] = evaluate_move(board, move);
            if (to == recapture_sq)
                sort_values[i] += 100;
            continue;
        }

        // History.
        Square from = MoveBits::from(move);
        bool piece_type = MoveBits::piece_type(move);
        sort_values[i] = history[board->turn][from][to][piece_type];

        // Counter-card.
        if (board->cards[board->turn][MoveBits::card_index(move)] ==
            counters[board->cards[board->turn][0]][board->cards[board->turn][1]]
                    [board->side_card])
            sort_values[i] += 1000;
    }

    // Sort.
    insertion_sort(moves, sort_values, size);
}

bool Thread::is_killer(uint8_t ply, Move move, uint8_t killer_num) {
    for (uint8_t k = 0; k < killer_num; ++k) {
        if (killers[ply][k] == move)
            return true;
    }
    return false;
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
            Board thread_board = board->copy();
            thread->search(&thread_board, depth + (th - 1) / 2, alpha, beta);
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
    Move best_move;

    auto search = [&board, silent, num_threads, &best_move](Thread *threads) {
        // Setup threads.
        std::vector<std::thread> helpers;
        for (uint8_t th = 0; th < num_threads; ++th) {
            Thread *thread = &threads[th];
            thread->th = th;
            thread->stop = false;
            thread->reset();
        }

        // Setup main search.
        nodes = tb_hits = 0;
        clock_t start = clock();
        int depth = 1, alpha = MIN_EVAL, beta = -MIN_EVAL, delta = WINDOW;

        // Iterative deepening.
        while (depth <= MAX_DEPTH) {
            int value;

            if (depth == 1) {
                value = threads[0].search(board, depth, alpha, beta);
            } else {
                start_helpers(threads, &helpers, num_threads, board, depth, alpha, beta);
                value = threads[0].search(board, depth, alpha, beta);
                end_helpers(threads, &helpers, num_threads);
            }

            // End search by timeout.
            if (threads[0].stop) {
                if (depth == 1)
                    best_move = get_tt_move(board);
                break;
            }

            double elapsed = (std::clock() - start) / static_cast<double>(CLOCKS_PER_SEC);

            // Window.
            delta += delta / 4;
            if (value <= alpha) {
                alpha = std::max(value - delta, MIN_EVAL);
            } else if (value >= beta) {
                beta = std::min(value + delta, -MIN_EVAL);
            } else {
                delta = WINDOW;
                if (depth >= 6) {
                    alpha = std::max(value - WINDOW, MIN_EVAL);
                    beta = std::min(value + WINDOW, -MIN_EVAL);
                }

                best_move = get_tt_move(board);

                // Output.
                if (!silent) {
                    std::string value_str;
                    if (is_mate_value(value)) {
                        int mate_plies =
                                std::abs(MIN_EVAL + board->move_count + std::abs(value));
                        int mate_depth = static_cast<int>(
                                std::copysign((mate_plies + 1) / 2, value));
                        value_str = "#" + std::to_string(mate_depth);
                    } else {
                        char value_buff[6];
                        snprintf(value_buff, sizeof(value_buff), "%2.2f",
                                 to_centi(value) / 100.0);
                        value_str = value_buff;
                        if (to_centi(value) > 0)
                            value_str = "+" + value_str;
                    }
                    printf("Depth %2i: Eval = %6s, Nodes = %10llu, TB-hits = "
                           "%8llu, %.3fs, Nodes/s = %8llu, "
                           "PV = [%s]\n",
                           depth, value_str.c_str(), nodes, tb_hits, elapsed,
                           uint64_t(nodes / elapsed), verify_pv(board, depth).c_str());
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

    return best_move;
}
