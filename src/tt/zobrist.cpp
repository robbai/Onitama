#include "zobrist.h"

#include <random>

namespace Zobrist {
    Hash PIECES[PLAYERS_NUM][PIECE_TYPES_NUM][SQUARE_NUM];
    Hash CARDS[CARDS_EACH_NUM][PLAYERS_NUM + 1];
    Hash TURN;
}  // namespace Zobrist

void init_zobrist() {
    std::mt19937_64 gen(std::random_device{}());
    for (int i = 0; i < PLAYERS_NUM; ++i) {
        for (int j = 0; j < PIECE_TYPES_NUM; ++j) {
            for (int k = 0; k < SQUARE_NUM; ++k)
                Zobrist::PIECES[i][j][k] = gen();
        }
    }
    for (int i = 0; i < CARD_NUM; ++i) {
        for (int j = 0; j < (PLAYERS_NUM + 1); ++j) {
            Zobrist::CARDS[i][j] = gen();
        }
    }
    Zobrist::TURN = gen();
}

void set_hash(Board *board) {
    board->hash = 0;

    // Pieces.
    for (int player = 0; player < PLAYERS_NUM; ++player) {
        for (int piece_type = 0; piece_type < PIECE_TYPES_NUM; ++piece_type) {
            Bitboard pieces = board->pieces[player][piece_type];
            while (pieces) {
                uint8_t sq = __builtin_popcount(pieces);
                board->hash ^= Zobrist::PIECES[player][piece_type][sq];
                pieces ^= (1u << sq);
            }
        }
    }

    // Cards.
    board->hash ^= Zobrist::CARDS[board->cards[WHITE][0]][WHITE];
    board->hash ^= Zobrist::CARDS[board->cards[WHITE][1]][WHITE];
    board->hash ^= Zobrist::CARDS[board->cards[BLACK][0]][BLACK];
    board->hash ^= Zobrist::CARDS[board->cards[BLACK][1]][BLACK];
    board->hash ^= Zobrist::CARDS[board->side_card][2];

    // Turn.
    if (board->turn)
        board->hash ^= Zobrist::TURN;
}
