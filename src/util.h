#ifndef ONITAMA_UTIL_H
#define ONITAMA_UTIL_H

#include <string>

#include "board.h"
#include "types.h"

const std::string CARD_NAMES[] = {
        "Rabbit",  "Monkey", "Boar",     "Goose",  "Cobra", "Crab", "Horse", "Dragon",
        "Rooster", "Crane",  "Elephant", "Mantis", "Tiger", "Frog", "Ox",    "Eel",
};

const std::string SQUARE_NAMES[] = {
        "a1", "b1", "c1", "d1", "e1", "a2", "b2", "c2", "d2", "e2", "a3", "b3", "c3",
        "d3", "e3", "a4", "b4", "c4", "d4", "e4", "a5", "b5", "c5", "d5", "e5",
};

std::string pretty_bitboard(Bitboard bitboard, bool card = false);
std::string pretty_board(Board *board);
std::string move_string(Board *board, Move move);

#endif  // ONITAMA_UTIL_H
