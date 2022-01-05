#include "tb_gen.h"

#include <algorithm>
#include <iostream>
#include <cassert>
#include <string>

#include "../move_tables.h"
#include "../util.h"

using std::cout;
using std::endl;
using std::max;
using std::string;
using std::to_string;

namespace Tablebase {
    // Card list.
    Card CARD_LIST[] = {TIGER, CRAB, RABBIT, BOAR, CRANE};
    // Students.
    uint8_t STUDENT_MEN = 2;
}  // namespace Tablebase

// Card setups.
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

Index get_max_index();
bool is_legal(Position *pos);
bool is_game_over(Position *pos);
Position from_index(Index index);
uint8_t gen_forward(Position *pos, Position *forward);
uint8_t gen_backward(Position *pos, Position *backward);
string pretty_position(Position *pos);

TBEntry *generate_tb() {
    clock_t start = clock();

    // Populate.
    const Index MAX_INDEX = get_max_index();
    cout << "Array size: " << MAX_INDEX << endl;
    TBEntry *entries = new TBEntry[MAX_INDEX];
    for (Index index = 0; index < MAX_INDEX; ++index) {
        TBEntry *entry = &entries[index];
        entry->iter = 0;
        Position pos = from_index(index);
        if (is_legal(&pos)) {
            entry->state = (is_game_over(&pos) ? LOSS : UNKNOWN);
            assert(index == get_index(&pos));
        } else {
            entry->state = ILLEGAL;
        }
    }

    // Generate.
    Position next_pos[MAX_MOVES];
    uint8_t next_size;
    bool change_made = true;
    for (uint8_t iter = 0; change_made; ++iter) {
        change_made = false;
        cout << "Iter: " << to_string(iter) << endl;
        for (Index index = 0; index < MAX_INDEX; ++index) {
            TBEntry *entry = &entries[index];
            if (entry->iter != iter)
                continue;
            Position pos;
            switch (entry->state) {
                case LOSS:
                    pos = from_index(index);
                    assert(pos.masters);
                loss:  // For branching from a self-loss.
                    next_size = gen_backward(&pos, next_pos);
                    if (!next_size) {
                        entry->state = ILLEGAL;
                        entry->iter = 0;
                        break;
                    }
                    for (uint8_t i = 0; i < next_size; ++i) {
                        assert(is_legal(&next_pos[i]));
                        assert(!is_game_over(&next_pos[i]));
                        Index new_index = get_index(&next_pos[i]);
                        TBEntry *new_entry = &entries[new_index];
                        if (new_entry->state != ILLEGAL &&
                            (new_entry->state != WIN || new_entry->iter > iter + 1)) {
                            new_entry->state = WIN;
                            new_entry->iter = iter + 1;
                            change_made = true;
                        }
                    }
                    break;
                case WIN:
                    pos = from_index(index);
                    assert(pos.masters);
                    next_size = gen_backward(&pos, next_pos);
                    if (!next_size) {
                        entry->state = ILLEGAL;
                        entry->iter = 0;
                        break;
                    }
                    for (uint8_t i = 0; i < next_size; ++i) {
                        assert(is_legal(&next_pos[i]));
                        assert(!is_game_over(&next_pos[i]));
                        Index new_index = get_index(&next_pos[i]);
                        TBEntry *new_entry = &entries[new_index];
                        if (new_entry->state != ILLEGAL && new_entry->state != WIN) {
                            new_entry->state = SELF_LOSS;
                            new_entry->iter = iter + 1;
                            change_made = true;
                        }
                    }
                    break;
                case SELF_LOSS:
                    pos = from_index(index);
                    assert(pos.masters);
                    next_size = gen_forward(&pos, next_pos);
                    assert(next_size);
                    entry->iter = 0;
                    for (uint8_t i = 0; i < next_size; ++i) {
                        Index new_index = get_index(&next_pos[i]);
                        TBEntry *new_entry = &entries[new_index];
                        assert(new_entry->state != ILLEGAL);
                        if (new_entry->state != WIN) {
                            entry->iter = 0;
                            break;
                        } else if (new_entry->iter >= entry->iter) {
                            entry->iter = new_entry->iter + 1;
                        }
                    }
                    if (entry->iter) {
                        entry->state = LOSS;
                        if (entry->iter == iter) {
                            goto loss;
                        } else if (entry->iter > iter) {
                            change_made = true;
                        }
                    } else {
                        entry->state = UNKNOWN;
                    }
                    break;
                default:
                    break;
            }
        }
        if (iter == 255) {
            cout << "Had to exit early" << endl;
            break;
        }
    }

    double duration = (clock() - start) / static_cast<double>(CLOCKS_PER_SEC);
    printf("Took %.6s seconds\n", to_string(duration).c_str());

    // Verify.
#ifdef DEBUG
    for (Index index = 0; index < MAX_INDEX; ++index) {
        TBEntry *entry = &entries[index];
        Position pos = from_index(index);
        bool loss_found = false;
        switch (entry->state) {
            case LOSS:
                next_size = gen_backward(&pos, next_pos);
                for (uint8_t i = 0; i < next_size; ++i) {
                    Index new_index = get_index(&next_pos[i]);
                    TBEntry *new_entry = &entries[new_index];
                    if (new_entry->state == ILLEGAL)
                        continue;
                    assert(new_entry->iter <= entry->iter + 1);
                    assert(new_entry->state == WIN);
                }
                if (entry->iter) {
                    next_size = gen_forward(&pos, next_pos);
                    for (uint8_t i = 0; i < next_size; ++i) {
                        Index new_index = get_index(&next_pos[i]);
                        TBEntry *new_entry = &entries[new_index];
                        assert(new_entry->iter <= entry->iter - 1);
                        assert(new_entry->state == WIN);
                    }
                }
                break;
            case WIN:
                assert(entry->iter);
                next_size = gen_forward(&pos, next_pos);
                for (uint8_t i = 0; i < next_size; ++i) {
                    Index new_index = get_index(&next_pos[i]);
                    TBEntry *new_entry = &entries[new_index];
                    if (new_entry->state != LOSS)
                        continue;
                    loss_found = true;
                    assert(new_entry->iter >= entry->iter - 1);
                }
                assert(loss_found);
                break;
            default:
                assert(entry->state != SELF_LOSS);
                break;
        }
    }
#endif

    return entries;
}

Index get_max_index() {
    Index max = SQUARE_NUM * (SQUARE_NUM + 1) - 1;  // Masters.
    for (uint8_t men = 0; men < Tablebase::STUDENT_MEN; ++men)
        max = (max * (SQUARE_NUM + 1 - men) + SQUARE_NUM - men) * 2 + 1;  // Students.
    return SETUPS_NUM * (max + 1) - 1;                                    // Cards.
}

bool is_legal(Position *pos) {
    // Fast fail (produced when an illegal position is found by from_index).
    if (!pos->masters)
        return false;
    if ((pos->pieces[WHITE] | pos->pieces[BLACK] | pos->masters) & 4261412864u)
        return false;
    if (pos->pieces[WHITE] & pos->pieces[BLACK])
        return false;
    if (!(pos->pieces[!pos->turn] & pos->masters))
        return false;
    if (pos->masters & pos->pieces[pos->turn] & (pos->turn ? 4u : 4194304u))
        return false;
    // Rare case in which our last move must've captured the opponent's master on our home.
    if (!(pos->pieces[pos->turn] & pos->masters) &&
        (pos->masters & (pos->turn ? 4u : 4194304u)) &&
        pos->masters == pos->pieces[!pos->turn])
        return false;
    if (Tablebase::STUDENT_MEN <
        __builtin_popcount((pos->pieces[WHITE] | pos->pieces[BLACK]) ^ pos->masters))
        return false;
    return true;
}

bool is_game_over(Position *pos) {
    if (!(pos->pieces[pos->turn] & pos->masters))
        return true;
    if (pos->masters & pos->pieces[!pos->turn] & (pos->turn ? 4194304u : 4u))
        return true;
    return false;
}

Index get_index(Position *pos) {
    Index index;
    if (pos->turn) {
        // Masters.
        Bitboard bitboard = (pos->masters & pos->pieces[BLACK]);
        index = (bitboard ? SQUARE_NUM - 1 - __builtin_ctz(bitboard) : SQUARE_NUM);
        bitboard = (pos->masters & pos->pieces[WHITE]);
        index += (SQUARE_NUM + 1) * (SQUARE_NUM - 1 - __builtin_ctz(bitboard));

        // Students.
        if (Tablebase::STUDENT_MEN) {
            bitboard = ((pos->pieces[WHITE] | pos->pieces[BLACK]) ^ pos->masters);
            Square prev_sq = SQUARE_NUM;
            for (uint8_t men = 0; men < Tablebase::STUDENT_MEN; ++men) {
                if (!bitboard) {
                    index = (index * (SQUARE_NUM + 1 - men) + SQUARE_NUM - men) * 2;
                    continue;
                }
                Square sq = (31 - __builtin_clz(bitboard));
                index = (index * (SQUARE_NUM + 1 - men) - sq + prev_sq - 1);
                index = (index * 2 + ((pos->pieces[WHITE] & (1u << sq)) != 0));
                bitboard ^= (1u << sq);
                prev_sq = sq;
            }
        }
    } else {
        // Masters.
        Bitboard bitboard = (pos->masters & pos->pieces[WHITE]);
        index = (bitboard ? __builtin_ctz(bitboard) : SQUARE_NUM);
        bitboard = (pos->masters & pos->pieces[BLACK]);
        index += (SQUARE_NUM + 1) * __builtin_ctz(bitboard);

        // Students.
        if (Tablebase::STUDENT_MEN) {
            bitboard = ((pos->pieces[WHITE] | pos->pieces[BLACK]) ^ pos->masters);
            int8_t prev_sq = -1;
            for (uint8_t men = 0; men < Tablebase::STUDENT_MEN; ++men) {
                if (!bitboard) {
                    index = (index * (SQUARE_NUM + 1 - men) + SQUARE_NUM - men) * 2;
                    continue;
                }
                Square sq = __builtin_ctz(bitboard);
                index = (index * (SQUARE_NUM + 1 - men) + sq - prev_sq - 1);
                index = (index * 2 + ((pos->pieces[BLACK] & (1u << sq)) != 0));
                bitboard ^= (1u << sq);
                prev_sq = sq;
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

Position from_index(Index index) {
    Position pos = {};
    pos.turn = false;

    // Cards.
    pos.cards = 0;
    uint64_t cards = 0;
    for (auto &card : Tablebase::CARD_LIST)
        cards |= 1ull << card;
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
    if (Tablebase::STUDENT_MEN) {
        Square furthest_sq = 0;
        for (int8_t men = (Tablebase::STUDENT_MEN - 1); men >= 0; --men) {
            uint8_t student = (index % (2 * (SQUARE_NUM + 1 - men)));
            Square sq = (student / 2);
            bool player = (student % 2);
            if (sq != SQUARE_NUM - men) {
                if (men)
                    ++sq;
                if (furthest_sq + sq >= SQUARE_NUM) {
                    pos.masters = 0;
                    return pos;  // Illegal.
                }
                furthest_sq += sq;
                if (player) {
                    pos.pieces[BLACK] |= 1u;
                } else {
                    pos.pieces[WHITE] |= 1u;
                }
                pos.pieces[BLACK] <<= sq;
                pos.pieces[WHITE] <<= sq;
            } else if (player || pos.pieces[WHITE] || pos.pieces[BLACK]) {
                pos.masters = 0;
                return pos;  // Illegal.
            }
            index /= 2 * (SQUARE_NUM + 1 - men);
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
        Square from = __builtin_ctz(pieces);
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

    // Our master must leave the enemy's home.
    if (pos->masters & pieces & (pos->turn ? 4194304u : 4u))
        pieces &= pos->masters;

    // If the opponent has no master, we cannot undo a move "from" our home.
    if (!(pos->masters & ~pieces))
        pieces &= ~(pos->turn ? 4u : 4194304u);

    while (pieces) {
        Square from = __builtin_ctz(pieces);
        Bitboard from_mask = (1u << from);

        uint64_t cards = pos->cards;
        if (!pos->turn)
            cards >>= CARD_NUM;
        Bitboard card_squares =
                MOVE_TABLES[(side_index + from) * PLAYERS_NUM + pos->turn] & targets;

        bool master_move = (pos->masters & from_mask);

        // Our master cannot move backwards onto the opponent's home.
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
                    can_capture_student = (Tablebase::STUDENT_MEN >
                                           __builtin_popcount((pos->pieces[WHITE] |
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

string pretty_position(Position *pos) {
    string str = "  +---+---+---+---+---+\n";
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
            str += (!file ? to_string(rank + 1) + " | " : " | ");
            str += character;
        }
        str += " | \n  +---+---+---+---+---+\n";
    }
    str += "    A   B   C   D   E\n";
    str += "Side to move:  ";
    str += (pos->turn ? "Black" : "White");
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
