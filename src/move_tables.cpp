#include "move_tables.h"

#include <vector>

using std::vector;

Bitboard MOVE_TABLES[CARD_NUM * SQUARE_NUM * PLAYERS_NUM];

bool on_board(int file, int rank) {
    return file >= 0 && file < BOARD_LENGTH && rank >= 0 && rank < BOARD_LENGTH;
}

void init_move_tables() {
    // Create a list of deltas, that represent each of the cards' moves from
    // origin.
    const vector<vector<vector<int>>> DELTAS = {
            {{-1, -1}, {1, 1}, {2, 0}},            // Rabbit.
            {{-1, -1}, {-1, 1}, {1, -1}, {1, 1}},  // Monkey.
            {{1, 0}, {-1, 0}, {0, 1}},             // Boar.
            {{-1, 0}, {-1, 1}, {1, 0}, {1, -1}},   // Goose.
            {{-1, 0}, {1, 1}, {1, -1}},            // Cobra.
            {{0, 1}, {-2, 0}, {2, 0}},             // Crab.
            {{0, 1}, {-1, 0}, {0, -1}},            // Horse.
            {{-1, -1}, {1, -1}, {-2, 1}, {2, 1}},  // Dragon.
            {{-1, 0}, {1, 0}, {1, 1}, {-1, -1}},   // Rooster.
            {{-1, -1}, {1, -1}, {0, 1}},           // Crane.
            {{-1, 0}, {1, 0}, {-1, 1}, {1, 1}},    // Elephant.
            {{-1, 1}, {1, 1}, {0, -1}},            // Mantis.
            {{0, 2}, {0, -1}},                     // Tiger.
            {{-1, 1}, {1, -1}, {-2, 0}},           // Frog.
            {{0, 1}, {0, -1}, {1, 0}},             // Ox.
            {{1, 0}, {-1, 1}, {-1, -1}}            // Eel.
    };

    // Create a look-up table relating each card to a square.
    for (Square square = 0; square < SQUARE_NUM; ++square) {
        int file = square % BOARD_LENGTH, rank = square / BOARD_LENGTH;
        for (int card = 0; card < CARD_NUM; ++card) {
            int index = (card * SQUARE_NUM + square) * PLAYERS_NUM;
            for (vector<int> delta : DELTAS[card]) {
                // White's perspective.
                int card_file = file + delta[0], card_rank = rank + delta[1];
                if (on_board(card_file, card_rank)) {
                    MOVE_TABLES[index] |= 1 << (card_file + card_rank * BOARD_LENGTH);
                }

                // Black's perspective.
                card_file = file - delta[0], card_rank = rank - delta[1];
                if (on_board(card_file, card_rank)) {
                    MOVE_TABLES[index + 1] |= 1 << (card_file + card_rank * BOARD_LENGTH);
                }
            }
        }
    }
}
