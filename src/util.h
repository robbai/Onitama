#ifndef ONITAMA_UTIL_H
#define ONITAMA_UTIL_H


#include <string>
#include "types.h"
#include "board.h"


using namespace std;


string pretty_bitboard(Bitboard bitboard);
string pretty_board(Board *board);


#endif //ONITAMA_UTIL_H
