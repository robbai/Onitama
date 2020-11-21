#ifndef ONITAMA_BITBASE_H
#define ONITAMA_BITBASE_H

#include <cstdint>
#include "types.h"

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

void create_bitbase();

#endif  // ONITAMA_BITBASE_H
