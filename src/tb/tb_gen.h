#ifndef ONITAMA_TB_GEN_H
#define ONITAMA_TB_GEN_H

#include <cstdint>
#include "../types.h"

namespace Tablebase {
    // Card list.
    extern Card CARD_LIST[CARDS_EACH_NUM * PLAYERS_NUM + 1];
    // Students.
    extern uint8_t STUDENT_MEN;
}  // namespace Tablebase

typedef uint64_t Index;

enum State : uint8_t { UNKNOWN, ILLEGAL, WIN, LOSS, SELF_LOSS };

struct Entry {
    uint8_t iter;
    State state;
};

struct Position {
    bool turn;
    Bitboard pieces[PLAYERS_NUM];
    Bitboard masters;
    uint64_t cards;
};

Entry *generate_tb();

Index get_index(Position *pos);

#endif  // ONITAMA_TB_GEN_H
