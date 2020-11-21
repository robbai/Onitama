#include "bitbase.h"

#include <algorithm>
#include <iostream>
#include <bitset>
#include <cassert>
#include <ctime>
#include <string>

#include "move_tables.h"
#include "util.h"

// Card setup.
constexpr Card CARD_LIST[] = {CRANE, HORSE, OX, BOAR, EEL};
// Students.
constexpr uint8_t STUDENT_MEN = 3;

constexpr uint8_t SETUPS_NUM = 30;

constexpr uint8_t SETUPS[] = {
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 0,   255,
        1,   255, 255, 255, 255, 255, 2,   255, 255, 255, 255, 255, 255, 255, 255, 255,
        3,   255, 4,   255, 255, 255, 6,   255, 255, 255, 9,   255, 255, 255, 7,   255,
        10,  255, 255, 255, 255, 255, 255, 255, 255, 255, 5,   255, 255, 255, 255, 255,
        8,   255, 11,  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 12,  255, 13,  255, 255, 255, 15,  255, 255, 255,
        18,  255, 255, 255, 16,  255, 19,  255, 255, 255, 255, 255, 255, 255, 21,  255,
        255, 255, 24,  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 27,  255,
        255, 255, 255, 255, 255, 255, 255, 255, 22,  255, 25,  255, 255, 255, 255, 255,
        28,  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 14,  255, 255, 255, 255, 255, 17,  255, 20,  255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 23,  255,
        26,  255, 255, 255, 255, 255, 29};
constexpr uint8_t SETUPS_INVERSE[] = {14,  16,  22,  32,  34,  58,  38,  46,  64,  42,
                                      48,  66,  86,  88,  166, 92,  100, 172, 96,  102,
                                      174, 110, 136, 190, 114, 138, 192, 126, 144, 198};

uint64_t get_max_index();
bool legal(Position *pos);
bool game_over(Position *pos);
uint64_t get_index(Position *pos);
Position from_index(uint64_t index);
uint8_t gen_forward(Position *pos, Position *forward);
uint8_t gen_backward(Position *pos, Position *backward);
std::string pretty_position(Position *pos);

void create_bitbase() {
    clock_t start = clock();

    // Populate array.
    const uint64_t MAX_INDEX = get_max_index();
    std::cout << "Array size: " << MAX_INDEX << std::endl;
    Entry *all = new Entry[MAX_INDEX];
    for (uint64_t i = 0; i < MAX_INDEX; ++i) {
        Entry *entry = &all[i];
        entry->iter = 0;
        Position pos = from_index(i);
        assert(__builtin_popcountll(pos.cards) == 5);
        if (legal(&pos)) {
            entry->state = (game_over(&pos) ? LOSS : UNKNOWN);
            assert(i == get_index(&pos) || STUDENT_MEN >= 2);
        } else {
            entry->state = ILLEGAL;
        }
    }

    // Continue loop.
    bool change_made = true;
    Position then[MAX_MOVES];
    uint8_t size;
    for (uint8_t iter = 0; change_made; ++iter) {
        std::cout << "Iter: " << std::to_string(iter) << std::endl;
        change_made = false;
        for (uint64_t i = 0; i < MAX_INDEX; ++i) {
            Position pos;
            Entry *entry = &all[i];
            if (entry->iter != iter)
                continue;
            switch (entry->state) {
                case LOSS:
                    pos = from_index(i);
                    assert(pos.masters);
                loss:  // For branching from a self-loss.
                    size = gen_backward(&pos, then);
                    if (!size) {
                        entry->state = ILLEGAL;
                        entry->iter = 0;
                        break;
                    }
                    for (uint8_t j = 0; j < size; ++j) {
                        uint64_t index = get_index(&then[j]);
                        Entry *new_entry = &all[index];
                        if (new_entry->state != ILLEGAL &&
                            (new_entry->state != WIN || new_entry->iter > iter + 1)) {
                            new_entry->state = WIN;
                            new_entry->iter = iter + 1;
                            change_made = true;
                        }
                    }
                    break;
                case WIN:
                    pos = from_index(i);
                    assert(pos.masters);
                    size = gen_backward(&pos, then);
                    if (!size) {
                        entry->state = ILLEGAL;
                        entry->iter = 0;
                        break;
                    }
                    for (int j = 0; j < size; ++j) {
                        uint64_t index = get_index(&then[j]);
                        Entry *new_entry = &all[index];
                        if (new_entry->state != ILLEGAL && new_entry->state != WIN) {
                            new_entry->state = SELF_LOSS;
                            new_entry->iter = iter + 1;
                            change_made = true;
                        }
                    }
                    break;
                case SELF_LOSS:
                    pos = from_index(i);
                    assert(pos.masters);
                    size = gen_forward(&pos, then);
                    assert(size);
                    entry->iter = 0;
                    for (int j = 0; j < size; ++j) {
                        uint64_t index = get_index(&then[j]);
                        Entry *new_entry = &all[index];
                        assert(new_entry->state != ILLEGAL);
                        if (new_entry->state != WIN) {
                            entry->iter = 0;
                            break;
                        } else if (new_entry->iter >= entry->iter) {
                            entry->iter = new_entry->iter + 1;
                        }
                    }
                    if (entry->iter) {
                        change_made = true;
                        entry->state = LOSS;
                        if (entry->iter == iter)
                            goto loss;
                    } else {
                        entry->state = UNKNOWN;
                    }
                    break;
                default:
                    break;
            }
        }
    }

    double duration = (clock() - start) / static_cast<double>(CLOCKS_PER_SEC);

    std::string states[] = {"Draw", "Illegal", "Win", "Loss", "Self-Loss"};

    uint8_t greatest_iter = 0;
    for (uint64_t i = 0; i < MAX_INDEX; ++i)
        greatest_iter = std::max(greatest_iter, all[i].iter);
    for (uint64_t i = 0; i < MAX_INDEX; ++i) {
        Entry *entry = &all[i];
        assert(entry->state != SELF_LOSS);
        if (entry->iter < greatest_iter)
            continue;
        switch (entry->state) {
            case UNKNOWN:
            case ILLEGAL:
            case SELF_LOSS:
                break;
            case LOSS:
            case WIN:
                Position pos = from_index(i);
                //                while (true) {
                std::cout << std::endl
                          << "Index: " << i << std::endl
                          << pretty_position(&pos) << std::endl
                          << states[all[i].state]
                          << (all[i].iter != 0
                                      ? " in " + std::to_string(all[i].iter) + " plies"
                                      : "")
                          << std::endl;
                //                    if (game_over(&pos))
                //                        break;
                //                    size = gen_forward(&pos, then);
                //                    uint64_t windex;
                //                    for (int j = 0; j < size; ++j) {
                //                        uint64_t index = get_index(&then[j]);
                //                        std::cout << states[all[index].state] << " ("
                //                                  << std::to_string(all[index].iter)
                //                                  << (j == size - 1 ? ")" : "), ");
                //                        if (!j || all[i].iter - 1 == all[index].iter) {
                //                            windex = index;
                //                        }
                //                    }
                //                    std::cout << std::endl;
                //                    pos = from_index(windex);
                //                    i = windex;
                //                }
                //                abort();
        }
    }
    std::cout << std::endl;

    std::cout << "For the side to move:" << std::endl;
    std::uint64_t running_count[] = {0, 0, 0, 0, 0};
    for (int iter = 0; true; ++iter) {
        std::uint64_t count[] = {0, 0, 0, 0, 0};
        for (uint64_t i = 0; i < MAX_INDEX; ++i) {
            Entry *entry = &all[i];
            if (entry->iter != iter)
                continue;
            ++count[entry->state];
        }
        uint64_t total = 0;
        for (uint8_t state = UNKNOWN; state <= SELF_LOSS; ++state) {
            total += count[state];
            running_count[state] += count[state];
        }
        if (!total)
            break;
        std::cout << total << " boards at " << iter << " plies: " << count[WIN]
                  << " wins, " << count[UNKNOWN] << " draws, " << count[LOSS] << " losses"
                  << std::endl;
    }

    std::cout << std::endl;
    for (uint8_t state = UNKNOWN; state <= SELF_LOSS; ++state)
        std::cout << states[state] << ": " << running_count[state] << std::endl;

    printf("\nTook %.6s seconds\n", std::to_string(duration).c_str());

    delete[] all;
}

uint64_t get_max_index() {
    uint64_t max = SQUARE_NUM * (SQUARE_NUM + 1) - 1;  // Masters.
    for (int _ = 0; _ < STUDENT_MEN; ++_)
        max = (max * (SQUARE_NUM + 1) + SQUARE_NUM) * 2 + 1;  // Students.
    return SETUPS_NUM * (max + 1) - 1;                        // Cards.
}

bool legal(Position *pos) {
    if (!pos->masters)
        return false;
    if (pos->pieces[WHITE] & pos->pieces[BLACK])
        return false;
    if (!(pos->pieces[!pos->turn] & pos->masters))
        return false;
    if (pos->masters & pos->pieces[pos->turn] & (pos->turn ? 4u : 4194304u))
        return false;
    if (!(pos->pieces[pos->turn] & pos->masters) &&
        (pos->masters & (pos->turn ? 4u : 4194304u)) &&
        pos->masters == pos->pieces[!pos->turn])
        return false;
    if (STUDENT_MEN <
        __builtin_popcount((pos->pieces[WHITE] | pos->pieces[BLACK]) ^ pos->masters))
        return false;
    //    if (__builtin_popcountll(pos->cards & 65535u) != 2)
    //        return false;
    //    if (__builtin_popcountll(pos->cards & 4294901760u) != 2)
    //        return false;
    //    if (__builtin_popcount(pos->cards >> 32) != 1)
    //        return false;
    return true;
}

bool game_over(Position *pos) {
    if (!(pos->pieces[pos->turn] & pos->masters))
        return true;
    if (pos->masters & pos->pieces[!pos->turn] & (pos->turn ? 4194304u : 4u))
        return true;
    return false;
}

uint64_t get_index(Position *pos) {
    uint64_t index;
    if (pos->turn) {
        // Masters.
        Bitboard bitboard = (pos->masters & pos->pieces[BLACK]);
        index = (bitboard ? SQUARE_NUM - 1 - __builtin_ctz(bitboard) : SQUARE_NUM);
        bitboard = (pos->masters & pos->pieces[WHITE]);
        index += (SQUARE_NUM + 1) * (SQUARE_NUM - 1 - __builtin_ctz(bitboard));

        // Students.
        if (STUDENT_MEN) {
            bitboard = ((pos->pieces[WHITE] | pos->pieces[BLACK]) ^ pos->masters);
            for (uint8_t men = 0; men < STUDENT_MEN; ++men) {
                if (!bitboard) {
                    index = (index * (SQUARE_NUM + 1) + SQUARE_NUM) * 2;
                    continue;
                }
                uint8_t sq = __builtin_ctz(bitboard);
                index = (index * (SQUARE_NUM + 1) + (SQUARE_NUM - 1 - sq));
                index = (index * 2 + ((pos->pieces[WHITE] & (1u << sq)) != 0));
                bitboard ^= (1u << sq);
            }
        }
    } else {
        // Masters.
        Bitboard bitboard = (pos->masters & pos->pieces[WHITE]);
        index = (bitboard ? __builtin_ctz(bitboard) : SQUARE_NUM);
        bitboard = (pos->masters & pos->pieces[BLACK]);
        index += (SQUARE_NUM + 1) * __builtin_ctz(bitboard);

        // Students.
        if (STUDENT_MEN) {
            bitboard = ((pos->pieces[WHITE] | pos->pieces[BLACK]) ^ pos->masters);
            for (uint8_t men = 0; men < STUDENT_MEN; ++men) {
                if (!bitboard) {
                    index = (index * (SQUARE_NUM + 1) + SQUARE_NUM) * 2;
                    continue;
                }
                uint8_t sq = __builtin_ctz(bitboard);
                index = (index * (SQUARE_NUM + 1) + sq);
                index = (index * 2 + ((pos->pieces[BLACK] & (1u << sq)) != 0));
                bitboard ^= (1u << sq);
            }
        }
    }

    // Cards.
    uint64_t cards = pos->cards | (pos->cards >> 16) | (pos->cards >> 32);
    uint8_t setup_index = 0;
    for (uint8_t i = 0; i < 5; ++i) {
        uint64_t mask = (cards & -cards);
        if (pos->cards & mask) {
            setup_index = setup_index * 3 + pos->turn;
        } else if (pos->cards & (mask << 16)) {
            setup_index = setup_index * 3 + !pos->turn;
        } else {
            setup_index = setup_index * 3 + 2;
        }
        cards ^= mask;
    }
    index = index * SETUPS_NUM + SETUPS[setup_index];

    return index;
}

Position from_index(uint64_t index) {
    Position pos = {};
    pos.turn = false;

    // Cards.
    pos.cards = 0;
    uint64_t cards = 0;
    for (uint8_t i = 0; i < 5; ++i)
        cards |= 1ull << CARD_LIST[i];
    uint8_t setup_index = SETUPS_INVERSE[index % SETUPS_NUM];
    while (cards) {
        uint8_t card = 63 - __builtin_clzll(cards);
        switch (setup_index % 3) {
            case 0:
                pos.cards |= 1ull << card;
                break;
            case 1:
                pos.cards |= 65536ull << card;
                break;
            case 2:
                pos.cards |= 4294967296ull << card;
                break;
        }
        setup_index /= 3;
        cards ^= (1ull << card);
    }
    index /= SETUPS_NUM;

    pos.pieces[WHITE] = 0;
    pos.pieces[BLACK] = 0;

    // Students.
    if (STUDENT_MEN) {
        for (uint8_t men = 0; men < STUDENT_MEN; ++men) {
            uint8_t student = (index % (2 * (SQUARE_NUM + 1)));
            if (student / 2 != SQUARE_NUM) {
                pos.pieces[student % 2] |= (1u << (student / 2));
            } else if (student % 2) {
                pos.masters = 0;
                return pos;  // Illegal.
            }
            index /= 2 * (SQUARE_NUM + 1);
        }
    }

    // Masters.
    Bitboard master = 1u << (index % (SQUARE_NUM + 1));
    if (master & pos.pieces[WHITE]) {
        pos.masters = 0;
        return pos;  // Illegal.
    } else if (master != 1u << SQUARE_NUM) {
        pos.masters = master;
        pos.pieces[WHITE] |= master;
    } else {
        pos.masters = 0;
    }
    master = 1u << (index / (SQUARE_NUM + 1));
    if (master & pos.pieces[BLACK]) {
        pos.masters = 0;
        return pos;  // Illegal.
    }
    pos.masters |= master;
    pos.pieces[BLACK] |= master;

    return pos;
}

uint8_t gen_forward(Position *pos, Position *forward) {
    uint8_t total = 0;
    Bitboard pieces = pos->pieces[pos->turn];
    Bitboard targets = ~pieces;
    while (pieces) {
        uint8_t from = __builtin_ctz(pieces);
        Bitboard from_mask = (1u << from);
        uint64_t cards = pos->cards;
        if (pos->turn)
            cards >>= CARD_NUM;
        for (uint8_t card_index = 0; card_index < CARDS_EACH_NUM; ++card_index) {
            uint64_t card = __builtin_ctz(cards);
            Bitboard squares =
                    MOVE_TABLES[(card * SQUARE_NUM + from) * PLAYERS_NUM + pos->turn] &
                    targets;
            while (squares) {
                Bitboard to_mask = squares & -squares;

                // Create new position and make move.
                Position new_pos = {};
                Bitboard move_xor = (from_mask | to_mask);
                new_pos.pieces[pos->turn] = (pos->pieces[pos->turn] ^ move_xor);
                Bitboard not_captured = ~to_mask;
                new_pos.pieces[!pos->turn] = (pos->pieces[!pos->turn] & not_captured);
                new_pos.masters = (pos->masters & not_captured);
                if (new_pos.masters & from_mask)
                    new_pos.masters ^= move_xor;
                new_pos.cards = pos->cards;
                new_pos.cards |=
                        (new_pos.turn ? (new_pos.cards & 281470681743360ull) >> 16
                                      : new_pos.cards >> 32);
                new_pos.cards &= 4294967295ull;
                new_pos.cards |= (4294967296ull << card);
                new_pos.cards ^= ((new_pos.turn ? 65536ull : 1ull) << card);
                new_pos.turn = !pos->turn;
                forward[total] = new_pos;
                ++total;

                squares ^= to_mask;
            }
            cards ^= (1ull << card);
        }
        pieces ^= from_mask;
    }
    return total;
}

uint8_t gen_backward(Position *pos, Position *backward) {
    uint8_t total = 0;
    Bitboard pieces = pos->pieces[!pos->turn];
    Bitboard targets = ~(pieces | pos->pieces[pos->turn]);
    uint16_t side_index = __builtin_ctz(pos->cards >> 32) * SQUARE_NUM;
    if (pos->masters & pieces & (pos->turn ? 4194304u : 4u))
        pieces &= pos->masters;
    while (pieces) {
        uint8_t from = __builtin_ctz(pieces);
        Bitboard from_mask = (1u << from);
        bool master_move = (pos->masters & from_mask);
        if (!(pos->masters & ~pieces) && (from_mask & (pos->turn ? 4u : 4194304u))) {
            pieces ^= from_mask;
            continue;
        }
        uint64_t cards = pos->cards;
        if (!pos->turn)
            cards >>= CARD_NUM;
        Bitboard card_squares =
                MOVE_TABLES[(side_index + from) * PLAYERS_NUM + pos->turn] & targets;
        if (master_move)
            card_squares &= (pos->turn ? 29360127u : 33554427u);
        for (uint8_t card_index = 0; card_index < CARDS_EACH_NUM; ++card_index) {
            uint64_t card = __builtin_ctz(cards);
            Bitboard squares = card_squares;
            while (squares) {
                Bitboard to_mask = squares & -squares;

                // Create new position and make move.
                Position new_pos = {};
                new_pos.turn = !pos->turn;
                Bitboard move_xor = (from_mask | to_mask);
                new_pos.pieces[new_pos.turn] = (pos->pieces[new_pos.turn] ^ move_xor);
                if (master_move) {
                    new_pos.masters = (pos->masters ^ move_xor);
                } else {
                    new_pos.masters = pos->masters;
                }
                bool can_capture_student = false;
                if (!(new_pos.masters & pos->pieces[pos->turn])) {
                    new_pos.pieces[pos->turn] = (from_mask | pos->pieces[pos->turn]);
                    new_pos.masters |= from_mask;
                } else {
                    new_pos.pieces[pos->turn] = pos->pieces[pos->turn];
                    can_capture_student =
                            (STUDENT_MEN > __builtin_popcount((pos->pieces[WHITE] |
                                                               pos->pieces[BLACK]) &
                                                              ~pos->masters));
                }
                new_pos.cards = pos->cards;
                new_pos.cards |=
                        (new_pos.turn ? (new_pos.cards & 281470681743360ull) >> 16
                                      : new_pos.cards >> 32);
                new_pos.cards &= 4294967295ull;
                new_pos.cards |= (4294967296ull << card);
                new_pos.cards ^= ((new_pos.turn ? 65536ull : 1ull) << card);
                backward[total] = new_pos;
                ++total;
                assert(new_pos.pieces[WHITE] | new_pos.pieces[BLACK]);
                if (can_capture_student) {
                    Position capture_pos = {};
                    capture_pos.turn = new_pos.turn;
                    capture_pos.masters = new_pos.masters;
                    capture_pos.cards = new_pos.cards;
                    capture_pos.pieces[new_pos.turn] = new_pos.pieces[new_pos.turn];
                    capture_pos.pieces[pos->turn] =
                            (new_pos.pieces[pos->turn] | from_mask);
                    backward[total] = capture_pos;
                    ++total;
                }

                squares ^= to_mask;
            }
            cards ^= (1ull << card);
        }
        pieces ^= from_mask;
    }
    return total;
}

std::string pretty_position(Position *pos) {
    std::string str = "  +---+---+---+---+---+\n";
    for (int rank = (BOARD_LENGTH - 1); rank >= 0; --rank) {
        for (int file = 0; file < BOARD_LENGTH; ++file) {
            Bitboard mask = (1u << (file + rank * BOARD_LENGTH));
            char character;
            if (mask & pos->pieces[WHITE]) {
                if (mask & pos->masters) {
                    character = 'X';
                } else {
                    character = 'x';
                }
            } else if (mask & pos->pieces[BLACK]) {
                if (mask & pos->masters) {
                    character = 'O';
                } else {
                    character = 'o';
                }
            } else {
                character = '.';
            }
            str += (!file ? std::to_string(rank + 1) + " | " : " | ");
            str += character;
        }
        str += " | \n  +---+---+---+---+---+\n";
    }
    str += "    A   B   C   D   E\n";
    str += "Side to move:  ";
    str += (pos->turn ? "Black" : "White");
    //    str += "\n(" +
    //           std::bitset<32 - SQUARE_NUM>(pos->pieces[WHITE] >> SQUARE_NUM).to_string() +
    //           ") " + std::bitset<SQUARE_NUM>(pos->pieces[WHITE]).to_string() + "\n";
    //    str += "(" +
    //           std::bitset<32 - SQUARE_NUM>(pos->pieces[BLACK] >> SQUARE_NUM).to_string() +
    //           ") " + std::bitset<SQUARE_NUM>(pos->pieces[BLACK]).to_string() + "\n";
    //    str += "(" + std::bitset<32 - SQUARE_NUM>(pos->masters >> SQUARE_NUM).to_string() +
    //           ") " + std::bitset<SQUARE_NUM>(pos->masters).to_string() + "\n";
    //    str += std::bitset<CARD_NUM>(pos->cards >> 32).to_string() + " " +
    //           std::bitset<CARD_NUM>((pos->cards >> 16) & 65535).to_string() + " " +
    //           std::bitset<CARD_NUM>(pos->cards & 65535).to_string();
    str += "\nWhite's cards: ";
    uint64_t cards = (pos->cards & 65535ull);
    while (cards) {
        int card = __builtin_ctz(cards);
        str += CARD_NAMES[card] + ", ";
        cards ^= (1ull << card);
    }
    str = str.substr(0, str.length() - 2) + "\n";
    str += "Black's cards: ";
    cards = ((pos->cards >> 16) & 65535ull);
    while (cards) {
        int card = __builtin_ctz(cards);
        str += CARD_NAMES[card] + ", ";
        cards ^= (1ull << card);
    }
    str = str.substr(0, str.length() - 2) +
          "\nSide card:     " + CARD_NAMES[__builtin_ctz(pos->cards >> 32)];
    return str;
}
