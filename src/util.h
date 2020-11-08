#ifndef ONITAMA_UTIL_H
#define ONITAMA_UTIL_H

#include <string>

#include "board.h"
#include "types.h"

std::string pretty_bitboard(Bitboard bitboard);
std::string pretty_board(Board *board);

#endif  // ONITAMA_UTIL_H
