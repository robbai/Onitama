#include <iostream>
#include "move_tables.h"
#include "client.h"
#include "tt/zobrist.h"
#include "td_learn.h"
#include "evaluate.h"
#include "util.h"
#include "tt/ttable.h"

int main(int argc, char *argv[]) {
    init_zobrist();
    init_move_tables();
    init_evaluation_parameters();

    init_td_learn();

    // Array of ways to order cards.
    int setups[30][5] = {
            {0, 1, 2, 3, 4}, {0, 1, 2, 4, 3}, {0, 1, 3, 4, 2}, {0, 2, 1, 3, 4},
            {0, 2, 1, 4, 3}, {0, 2, 3, 4, 1}, {0, 3, 1, 2, 4}, {0, 3, 1, 4, 2},
            {0, 3, 2, 4, 1}, {0, 4, 1, 2, 3}, {0, 4, 1, 3, 2}, {0, 4, 2, 3, 1},
            {1, 2, 0, 3, 4}, {1, 2, 0, 4, 3}, {1, 2, 3, 4, 0}, {1, 3, 0, 2, 4},
            {1, 3, 0, 4, 2}, {1, 3, 2, 4, 0}, {1, 4, 0, 2, 3}, {1, 4, 0, 3, 2},
            {1, 4, 2, 3, 0}, {2, 3, 0, 1, 4}, {2, 3, 0, 4, 1}, {2, 3, 1, 4, 0},
            {2, 4, 0, 1, 3}, {2, 4, 0, 3, 1}, {2, 4, 1, 3, 0}, {3, 4, 0, 1, 2},
            {3, 4, 0, 2, 1}, {3, 4, 1, 2, 0}};

    // Go through every card combination.
    uint8_t setup_index = 7;
    const uint32_t TOTAL_ITER = 4368;
    uint32_t iter = 0;
    for (uint8_t card_1 = 0; card_1 < CARD_NUM - 4; ++card_1) {
        for (uint8_t card_2 = (card_1 + 1); card_2 < CARD_NUM - 3; ++card_2) {
            for (uint8_t card_3 = (card_2 + 1); card_3 < CARD_NUM - 2; ++card_3) {
                for (uint8_t card_4 = (card_3 + 1); card_4 < CARD_NUM - 1; ++card_4) {
                    for (uint8_t card_5 = (card_4 + 1); card_5 < CARD_NUM; ++card_5) {
                        setup_index = (setup_index + 1) % 30;
                        int *setup = setups[setup_index];
                        Card cards[5] = {(Card) card_1, (Card) card_2, (Card) card_3,
                                         (Card) card_4, (Card) card_5};
                        Board board = {};
                        board.cards[WHITE][0] = cards[setup[0]];
                        board.cards[WHITE][1] = cards[setup[1]];
                        board.cards[BLACK][0] = cards[setup[2]];
                        board.cards[BLACK][1] = cards[setup[3]];
                        board.side_card = cards[setup[4]];
                        ++iter;
                        std::cout << int(float(iter * 1000) / TOTAL_ITER) / 10.0 << "%"
                                  << std::endl;
                        learn_game(&board, iter % 10 && iter != TOTAL_ITER);
                        TTABLE.clear();
                    }
                }
            }
        }
    }

    return 0;
}
