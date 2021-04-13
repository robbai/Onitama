#ifndef ONITAMA_TTABLE_H
#define ONITAMA_TTABLE_H

#include <iostream>
#include <bitset>

#include "../types.h"
#include "../board.h"

enum NodeType : uint8_t { EXACT, UPPER, LOWER };

struct TTEntry {
    uint32_t remaining_hash;
    Move move = 0;
    uint8_t depth = 0;
    int value;
    NodeType type;
};

class TTable {
 private:
    const uint64_t key_mask;
    uint64_t size() const {
        return key_mask + 1;
    }
    TTEntry *table;

 public:
    explicit TTable(const uint64_t key_bits) : key_mask((1ull << key_bits) - 1) {
        std::cout << "Initialising TT (" << size() << ")" << std::endl;
        table = new TTEntry[size()];
    }
    void clear();
    TTEntry *probe(Board *board);
};

extern TTable TTABLE;

#endif  // ONITAMA_TTABLE_H
