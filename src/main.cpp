#include <iostream>
#include <algorithm>
#include <bitset>
#include <ctime>
#include <random>

#include "make_move.h"
#include "move_gen.h"
#include "move_tables.h"
#include "search.h"
#include "util.h"

int main() {
    init_move_tables();

    // Setup board.
    Board board;
    Card rand_cards[CARD_NUM];
    for (int i = 0; i < CARD_NUM; ++i)
        rand_cards[i] = (Card) i;
    unsigned int seed = (unsigned int) std::time(nullptr);
    std::shuffle(rand_cards, rand_cards + CARD_NUM, std::default_random_engine(seed));
    board.cards[0][0] = rand_cards[0];
    board.cards[0][1] = rand_cards[1];
    board.cards[1][0] = rand_cards[2];
    board.cards[1][1] = rand_cards[3];
    board.side_card = rand_cards[4];

    const bool PLAY_AS_WHITE = true;

    while (true) {
        // User move.
        std::cout << std::endl << pretty_board(&board) << std::endl;
        Move moves[MAX_MOVES];
        uint8_t size = gen_moves(&board, moves);
        while (board.turn != PLAY_AS_WHITE) {
            std::string inp;
            std::cout << "Your move:";
            std::cin >> inp;  // Get user input from the keyboard.
            for (int i = 0; i < size; ++i) {
                if (inp == move_string(&board, moves[i])) {
                    make_move(&board, moves[i]);
                    break;
                }
            }
        }
        if (board.game_over())
            break;

        // AI move.
        std::cout << std::endl << pretty_board(&board) << std::endl;
        Move move = search(&board);
        std::cout << "AI move: " << move_string(&board, move) << std::endl;
        make_move(&board, move);
        if (board.game_over())
            break;
    }

    // Game end.
    std::cout << std::endl
              << pretty_board(&board) << std::endl
              << (board.turn ? "White wins!" : "Black wins!");

    return 0;
}
