#ifndef ONITAMA_TYPES_H
#define ONITAMA_TYPES_H

#include <cstdint>

typedef uint32_t Bitboard;  // 25 bits for squares.
// 25 bits for XOR-bitboard, 1 bit for card index used, 1 bit for student
// captured (applied post-make).
typedef uint32_t Move;

const int BOARD_LENGTH = 5, SQUARE_NUM = (BOARD_LENGTH * BOARD_LENGTH);
const int PLAYERS_NUM = 2;
const int CARDS_EACH_NUM = 2;
const int MAX_MOVES = 40;
const Bitboard FULL_BITBOARD = ((Bitboard) 1 << SQUARE_NUM) - 1;

enum Piece { STUDENT, MASTER, PIECE_TYPES_NUM };

enum Card {
    RABBIT,
    MONKEY,
    BOAR,
    GOOSE,
    COBRA,
    CRAB,
    HORSE,
    DRAGON,
    ROOSTER,
    CRANE,
    ELEPHANT,
    MANTIS,
    TIGER,
    FROG,
    OX,
    EEL,
    CARD_NUM,
    NO_CARD
};

#endif  // ONITAMA_TYPES_H
