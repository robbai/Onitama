#include <vector>
#include "nnue.h"
#include "network.h"

vector<int> input;

float buffer[LAYER_SIZE[1] + 1];

int to_feature(int square, bool master, bool player, Card card, int card_pos) {
    return (((square * PIECE_TYPES_NUM + master) * PLAYERS_NUM + player) * CARD_NUM +
            card) * 3 +
           card_pos;
}

void populate_input(Board *board) {
    input.clear();
    Bitboard pieces = (board->pieces[0][0] | board->pieces[0][1] | board->pieces[1][0] |
                       board->pieces[1][1]);
    while (pieces) {
        uint8_t square = __builtin_ctz(pieces);
        Bitboard mask = 1u << square;
        bool player =
                mask & (board->pieces[!board->turn][0] | board->pieces[!board->turn][1]);
        bool master = mask & board->pieces[player ^ board->turn][MASTER];
        if (board->turn)
            square = SQUARE_NUM - 1 - square;

        for (int i_card = 0; i_card < 5; ++i_card) {
            Card card;
            int card_pos;
            switch (i_card) {
                case 0:
                case 1:
                    card = board->cards[board->turn][i_card];
                    card_pos = 0;
                    break;
                case 2:
                case 3:
                    card = board->cards[!board->turn][i_card - 2];
                    card_pos = 1;
                    break;
                case 4:
                    card = board->side_card;
                    card_pos = 2;
                    break;
            }

            int index = to_feature(square, master, player, card, card_pos);
            input.push_back(index);
        }
        pieces ^= mask;
    }
}

float nnue(Board *board) {
    populate_input(board);

    L0.pass(&input, buffer);
    float *curr_input = buffer;
    float *next_output = CR0.pass(curr_input, buffer);
    float *curr_output = next_output;

    L1.pass(curr_input, curr_output);
    return *curr_output;
}

int evaluate_nnue(Board *board) {
    float value = nnue(board);
    return 890 * (value * value * value) + 859 * value;
    //    return 2500 * value;
}
