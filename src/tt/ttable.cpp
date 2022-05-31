#include "ttable.h"
#include <math.h>

// The minimum size in MB for the TT to use.
const uint32_t MIN_MB = 5000;
const uint64_t KEY_BITS = ceil(log((1048576 * MIN_MB) / sizeof(TTEntry)) / log(2));
// Global transposition table.
TTable TTABLE(KEY_BITS);


TTEntry *TTable::probe(Board *board) {
    return &table[board->hash & key_mask];
}

void TTable::clear() {
    std::fill_n(this->table, this->size(), TTEntry());
}
