#include "zobrist.h"

namespace Zobrist {
    Hash PIECES[PLAYERS_NUM][PIECE_TYPES_NUM][SQUARE_NUM];
    Hash CARDS[CARDS_EACH_NUM][PLAYERS_NUM + 1];
    Hash TURN;
}  // namespace Zobrist

/*
 * https://stackoverflow.com/a/39675616
 */
class XRS_64 {
 public:
    XRS_64() {
        seed = 6394358446697381921;
    }
    uint64_t generate(void) {
        seed ^= seed >> 12;
        seed ^= seed << 25;
        seed ^= seed >> 27;
        return seed * UINT64_C(2685821657736338717);
    }

 private:
    uint64_t seed;
};

void init_zobrist() {
    XRS_64 rng = XRS_64();
    for (int i = 0; i < PLAYERS_NUM; ++i) {
        for (int j = 0; j < PIECE_TYPES_NUM; ++j) {
            for (int k = 0; k < SQUARE_NUM; ++k)
                Zobrist::PIECES[i][j][k] = rng.generate();
        }
    }
    for (int i = 0; i < CARD_NUM; ++i) {
        for (int j = 0; j < (PLAYERS_NUM + 1); ++j) {
            Zobrist::CARDS[i][j] = rng.generate();
        }
    }
    Zobrist::TURN = rng.generate();
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
