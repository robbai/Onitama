#ifndef ONITAMA_TYPES_H
#define ONITAMA_TYPES_H

#include <cstdint>

typedef uint32_t Bitboard;  // 25 bits for squares.
typedef uint16_t
        Move;  // 6 bits for from-square, 6 bits for to-square, 1 bit for card index used, 1 bit for student captured, 1 bit for piece type.
typedef uint64_t Hash;
typedef uint8_t Square;

const int BOARD_LENGTH = 5, SQUARE_NUM = (BOARD_LENGTH * BOARD_LENGTH);
const int PLAYERS_NUM = 2;
const int CARDS_EACH_NUM = 2;
const int MAX_MOVES = 40;
const Bitboard FULL_BITBOARD = (1u << SQUARE_NUM) - 1;

enum Piece { STUDENT, MASTER, PIECE_TYPES_NUM };

enum Turn : bool { WHITE, BLACK };

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
    CARD_NONE
};

#endif  // ONITAMA_TYPES_H
