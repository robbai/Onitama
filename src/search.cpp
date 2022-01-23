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
#include "move_tables.h"

uint8_t LIMITED_DEPTH = 1;

constexpr int MIN_EVAL = -100000, MIN_WINDOW = 20;

uint64_t nodes = 0;
uint64_t tb_hits = 0;

uint8_t root_size = 0;

enum Stage : uint8_t { TT, CAPTURE, KILLER, QUIET, STAGE_NUM };
enum QStage : uint8_t { Q_RECAPTURE, Q_CAPTURE, Q_ALL, Q_STAGE_NUM };

int LMR_TABLE[MAX_DEPTH][MAX_MOVES];

void init_search() {
    for (int depth = 0; depth < MAX_DEPTH; depth++)
        for (int move_num = 0; move_num < MAX_MOVES; move_num++)
            LMR_TABLE[depth][move_num] =
                    (0.36904 + log(depth * 2.74693) * log(move_num * 3.62443) * 0.56593);
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

int see(Bitboard us, Square us_master, Bitboard them, Square them_master, Square to,
        bool turn, Card c1, Card c2, Card c3, Card c4, Card c5) {
    Bitboard to_mask = 1u << to;

    int value = 0;

    Bitboard pieces = us;
    while (pieces) {
        Square from = __builtin_ctz(pieces);
        Bitboard from_mask = 1u << from;

        if ((MOVE_TABLES[(c1 * SQUARE_NUM + from) * PLAYERS_NUM + turn]) & to_mask) {
            if (to == them_master)
                return 10;
            value = std::max(0, 1 - see(them & ~to_mask, them_master,
                                        us ^ from_mask | to_mask,
                                        from == us_master ? to : us_master, to, !turn, c3,
                                        c4, c5, c2, c1));
        }
        if ((MOVE_TABLES[(c2 * SQUARE_NUM + from) * PLAYERS_NUM + turn]) & to_mask) {
            if (to == them_master)
                return 10;
            value = std::max(0, 1 - see(them & ~to_mask, them_master,
                                        us ^ from_mask | to_mask,
                                        from == us_master ? to : us_master, to, !turn, c3,
                                        c4, c1, c5, c2));
        }

        pieces ^= from_mask;
    }

    return value;
}

int see_move(Board *board, Move move) {
    Square from = MoveBits::from(move);
    Square to = MoveBits::to(move);
    Bitboard mask = (1u << from) | (1u << to);
    bool card_index = MoveBits::card_index(move);
    Square us_master = __builtin_ctz(board->pieces[board->turn][1]);
    if (us_master == from)
        us_master = to;

    int value = MoveBits::capture(move);
    value -=
            see((board->pieces[!board->turn][0] | board->pieces[!board->turn][1]) & ~mask,
                __builtin_ctz(board->pieces[!board->turn][1]),
                (board->pieces[board->turn][0] | board->pieces[board->turn][1]) ^ mask,
                us_master, to, !board->turn, board->cards[!board->turn][0],
                board->cards[!board->turn][1], board->cards[board->turn][!card_index],
                board->side_card, board->cards[board->turn][card_index]);
    return value;
}

int see_all(Board *board, Move move) {
    bool capture = MoveBits::capture(move);
    Square from = MoveBits::from(move);
    Square to = MoveBits::to(move);
    Bitboard mask = (1u << from) | (1u << to);
    bool card_index = MoveBits::card_index(move);
    Bitboard us = (board->pieces[board->turn][0] | board->pieces[board->turn][1]) ^ mask;
    Square us_master = __builtin_ctz(board->pieces[board->turn][1]);
    if (us_master == from)
        us_master = to;

    int value = capture;

    Bitboard pieces = us;
    while (pieces) {
        Square sq = __builtin_ctz(pieces);
        value = std::min(
                value,
                capture - see((board->pieces[!board->turn][0] |
                               board->pieces[!board->turn][1]) &
                                      ~mask,
                              __builtin_ctz(board->pieces[!board->turn][1]), us,
                              us_master, sq, !board->turn, board->cards[!board->turn][0],
                              board->cards[!board->turn][1],
                              board->cards[board->turn][!card_index], board->side_card,
                              board->cards[board->turn][card_index]));
        pieces ^= 1u << sq;
    }
    return value;
}

int Thread::search(Board *board, int depth, int alpha, int beta, int ply, bool cut_node,
                   Move last_move, bool check_tb, int pv_dist, Move excluded_move) {
    if (stop)
        return 0;

    bool root = !ply;
    bool pv_node = beta - alpha != 1;

    // Terminal.
    if (board->game_over()) {
        ++nodes;
        return MIN_EVAL + board->move_count;
    }

    if (!root) {
        // Win in one.
        if (board->has_winning_move()) {
            ++nodes;
            return -(MIN_EVAL + board->move_count + 1);
        }

        // Lose in one.
        if (board->get_runner()) {
            ++nodes;
            return MIN_EVAL + board->move_count + 2;
        }
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
    if (depth > 2) {
        for (uint8_t p1 = (ply & 1); p1 < ply; p1 += 2) {
            if (this->hash_line[p1] == board->hash) {
                for (uint8_t p2 = p1 + 1; p2 < ply; p2 += 2) {
                    if (!pv_played[p2])
                        break;
                    if (p2 + 2 >= ply) {
                        ++nodes;
                        return 0;
                    }
                }
            }
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

    // Prepare branching.
    Move best_move;
    int best_value = MIN_EVAL;
    Bitboard checkers = board->get_checkers();

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
                sort_moves(board, moves, size, stage == CAPTURE, false, last_move);
                break;
        }

        // Move loop.
        for (uint8_t move_index = 0; move_index < size; ++move_index) {
            const Move move = moves[move_index];

            // Filter moves.
            if (move == excluded_move)
                continue;
            if (stage >= CAPTURE) {
                if (move == entry->move)
                    continue;
                if (stage > KILLER && is_killer(ply, move))
                    continue;
            }

            ++move_num;
            pv_played[ply] = !move_num;

            // Reductions and pruning.
            int8_t reduction = 0;
            if (move_num) {
                if (!excluded_move && !is_mate_value(alpha) && !is_mate_value(beta)) {
                    // Futility prune.
                    if (stage > CAPTURE && depth < 6 &&
                        entry->value < alpha - 250 * std::max(1, depth - entry->depth))
                        break;

                    // SEE prune.
                    if (!pv_node && depth < 3 && see_move(board, move) < 0)
                        continue;
                }

                // Late-move reduction.
                if (depth > 2 && (stage == QUIET || !pv_node)) {
                    reduction = LMR_TABLE[depth][move_num];

                    if (last_move) {
                        Square to = MoveBits::to(move);
                        bool piece_type = MoveBits::piece_type(move);
                        int hist = counter_hist[MoveBits::piece_type(last_move)]
                                               [MoveBits::to(last_move)][piece_type][to]
                                               [board->student_count];
                        if (hist > 195)
                            reduction -= 1;
                    }

                    if (!pv_node)
                        reduction += 1;

                    if (cut_node)
                        reduction += 1;

                    if (reduction < 0)
                        reduction = 0;
                }
            }

            // Extensions.
            bool can_extend = ply < root_depth * 2 && ply < MAX_DEPTH - 2;
            if (can_extend) {
                // Singular extension.
                if (move == entry->move && ply > 2 && !excluded_move &&
                    depth > (pv_node ? 3 : 5) && entry->type != UPPER &&
                    abs(entry->value) < 3029 && entry->depth > depth - 7) {
                    int singular_beta = entry->value - 21 * depth;
                    int singular_depth = (depth - 2) / 4;
                    int value = search(board, singular_depth, singular_beta - 1,
                                       singular_beta, ply, cut_node, last_move, false,
                                       pv_dist, move);
                    if (value < singular_beta)
                        reduction -= 1;
                }

                // Capture extension.
                if (!board->student_delta && MoveBits::capture(move)) {
                    reduction -= 1;
                }
            }

            make_move(board, move);

            // Check extension.
            if (can_extend && checkers && !pv_node && pv_dist < 12)
                reduction -= 1;

            if (reduction > depth - 1)
                reduction = depth - 1;

            int next_pv_dist = pv_dist + move_index;

            int value;
            bool now_check_tb = GENERATED_TB && (root || MoveBits::capture(move));
            if (!move_num) {
                value = -search(board, depth - 1 - reduction, -beta, -alpha, ply + 1,
                                false, move, now_check_tb, next_pv_dist);
            } else {
                // Reductions and null window.
                value = -search(board, depth - 1 - reduction, -alpha - 1, -alpha, ply + 1,
                                true, move, now_check_tb, next_pv_dist);

                // Null window.
                if (value > alpha && reduction > 0) {
                    value = -search(board, depth - 1, -alpha - 1, -alpha, ply + 1,
                                    !cut_node, move, now_check_tb, next_pv_dist);
                }

                // Full search.
                if (value > alpha) {
                    value = -search(board, depth - 1 - (reduction > 0 ? 0 : reduction),
                                    -beta, -alpha, ply + 1, false, move, now_check_tb,
                                    next_pv_dist);
                }
            }

            undo_move(board, move);

            if (value > best_value) {
                best_value = value;
                best_move = move;
                if (value > alpha) {
                    // Cut-off.
                    if (value >= beta) {
                        bool capture = MoveBits::capture(move);

                        // Counter-card.
                        counter_card[board->cards[board->turn][0]]
                                    [board->cards[board->turn][1]][board->side_card]
                                    [capture] +=
                                (MoveBits::card_index(move) ? 1 : -1) * (depth * depth);

                        if (!capture) {
                            if (!checkers) {
                                // Counter-history.
                                if (last_move) {
                                    for (int8_t p = board->student_count; p >= 0; p -= 2)
                                        counter_hist[MoveBits::piece_type(last_move)]
                                                    [MoveBits::to(last_move)]
                                                    [MoveBits::piece_type(move)]
                                                    [MoveBits::to(move)][p] +=
                                                depth * depth;
                                }

                                // Killer move.
                                for (uint8_t k = (KILLER_NUM - 1); k > 0; --k)
                                    killers[ply][k] = killers[ply][k - 1];
                                killers[ply][0] = move;
                            }
                        } else {
                            // Capture-history.
                            for (int8_t p = board->student_count; p >= 0; p -= 2)
                                capture_hist[MoveBits::piece_type(move)][MoveBits::from(
                                        move)][MoveBits::to(move)][p] += depth * depth;
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
        if (best_value >= beta) {
            entry->type = LOWER;
        } else if (pv_node && best_move) {
            entry->type = EXACT;
        } else {
            entry->type = UPPER;
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

    // Probe TT.
    TTEntry *entry = TTABLE.probe(board);
    if (entry->remaining_hash == (board->hash >> 32)) {
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

    // Threats.
    if (board->get_runner())
        return MIN_EVAL + board->move_count + 2;
    Bitboard checkers = board->get_checkers();

    // Evaluate.
    int value = (checkers ? MIN_EVAL + board->move_count + 2
                          : (evaluate(board) / 16) * 16 + 2 * (nodes & 5) - 5);
    if (value >= beta)
        return beta;

    // Delta prune (futility).
    if (!checkers && value < alpha - 3512 && !is_mate_value(beta))
        return alpha;

    if (value > alpha)
        alpha = value;

    if (ply >= MAX_DEPTH)
        return alpha;

    Bitboard recapture = last_move ? (1u << MoveBits::to(last_move)) : 0;

    Move *moves = move_lists[ply];
    uint8_t size;
    for (uint8_t stage = Q_RECAPTURE; stage <= (checkers ? Q_ALL : Q_CAPTURE); ++stage) {
        switch (stage) {
            case Q_RECAPTURE:
                size = gen_moves(board, moves, recapture);
                sort_moves(board, moves, size, true, true);
                break;
            case Q_CAPTURE:
                size = gen_moves(board, moves,
                                 board->pieces[!board->turn][STUDENT] & ~recapture);
                sort_moves(board, moves, size, true, true);
                break;
            default:
                size = gen_moves(board, moves, ~board->pieces[!board->turn][STUDENT]);
                sort_moves(board, moves, size, false, true);
                break;
        }

        for (uint8_t i = 0; i < size; ++i) {
            const Move move = moves[i];

            // Skip moves that don't attempt to evade check.
            if (checkers &&
                !(MoveBits::piece_type(move) || (1u << MoveBits::to(move)) == checkers))
                continue;

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
    // Counter-history.
    for (auto &pt1 : counter_hist)
        for (auto &sq1 : pt1)
            for (auto &pt2 : sq1)
                for (auto &sq2 : pt2)
                    for (auto &p : sq2)
                        p = 0;

    // Capture-history.
    for (auto &pt : capture_hist)
        for (auto &sq1 : pt)
            for (auto &sq2 : sq1)
                for (auto &p : sq2)
                    p = 0;

    // Counter-cards.
    for (auto &card1 : counter_card)
        for (auto &card2 : card1)
            for (auto &val : card2)
                for (auto &cap : val)
                    cap = 0;

    // Killers.
    for (auto &killer : killers)
        for (Move &move : killer)
            move = 0;
}

void Thread::sort_moves(Board *board, Move *moves, uint8_t size, bool captures,
                        bool quiescence, Move last_move) {
    Square last_to = SQUARE_NUM;
    bool last_pt = false;
    bool last_capture = MoveBits::capture(last_move);
    if (last_move) {
        last_to = MoveBits::to(last_move);
        last_pt = MoveBits::piece_type(last_move);
    }

    // Assign scores.
    for (uint8_t i = 0; i < size; ++i) {
        const Move move = moves[i];
        bool piece_type = MoveBits::piece_type(move);
        Square from = MoveBits::from(move);
        Square to = MoveBits::to(move);
        bool card_index = MoveBits::card_index(move);

        if (captures) {
            sort_values[i] = 4000 * see_move(board, move);
            sort_values[i] += capture_hist[piece_type][from][to][board->student_count];
            sort_values[i] +=
                    counter_card[board->cards[board->turn][0]]
                                [board->cards[board->turn][1]][board->side_card][1] *
                    (card_index ? 1 : -1) * 8;
            if (to == last_to)
                sort_values[i] += last_capture ? 162 : 810;
            continue;
        }

        sort_values[i] = quiescence ? 0 : 2000 * see_move(board, move);

        // Counter-history.
        if (last_move)
            sort_values[i] +=
                    counter_hist[last_pt][last_to][piece_type][to][board->student_count];

        // Counter-card.
        sort_values[i] +=
                counter_card[board->cards[board->turn][0]][board->cards[board->turn][1]]
                            [board->side_card][0] *
                (card_index ? 1 : -1);
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
            thread->root_depth = depth + (th - 1) / 2;
            thread->search(&thread_board, thread->root_depth, alpha, beta);
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

std::string to_value_str(Board *board, int value) {
    std::string value_str;
    if (is_mate_value(value)) {
        int mate_plies = std::abs(MIN_EVAL + board->move_count + std::abs(value));
        int mate_depth = static_cast<int>(std::copysign((mate_plies + 1) / 2, value));
        value_str = "#" + std::to_string(mate_depth);
    } else {
        char value_buff[6];
        snprintf(value_buff, sizeof(value_buff), "%2.2f", to_centi(value) / 100.0);
        value_str = value_buff;
        if (to_centi(value) > 0)
            value_str = "+" + value_str;
    }
    return value_str;
}

Move start_search(Board *board, bool silent, uint8_t num_threads) {
    Move best_move;

    // Setup threads.
    Thread threads[num_threads];
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
    int depth = 1, alpha = MIN_EVAL, beta = -MIN_EVAL, delta = MIN_WINDOW;

    // Iterative deepening.
    while (depth <= std::min(MAX_DEPTH, LIMITED_DEPTH)) {
        int value;

        threads[0].root_depth = depth;
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
        delta += delta / 2;
        if (value <= alpha) {
            alpha = std::max(value - delta, MIN_EVAL);
        } else if (value >= beta) {
            beta = std::min(value + delta, -MIN_EVAL);
        } else {
            delta = MIN_WINDOW + abs(value) / 2;
            if (depth >= 6) {
                alpha = std::max(value - delta, MIN_EVAL);
                beta = std::min(value + delta, -MIN_EVAL);
            }

            best_move = get_tt_move(board);

            // Output.
            if (!silent) {
                printf("Depth %2i: Eval = %6s, Nodes = %10llu, TB-hits = "
                        "%8llu, %.3fs, Nodes/s = %8llu, "
                        "PV = [%s]\n",
                        depth, to_value_str(board, value).c_str(), nodes, tb_hits,
                        elapsed, uint64_t(nodes / elapsed),
                        verify_pv(board, depth).c_str());
            }

            ++depth;
        }
    }

    return best_move;
}
