#ifndef ONITAMA_TB_GEN_H
#define ONITAMA_TB_GEN_H

#include <cstdint>
#include "../types.h"

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

void generate_tb();

#endif  // ONITAMA_TB_GEN_H
