#include <iostream>
#include <algorithm>
#include <random>
#include "move_tables.h"
#include "tt/zobrist.h"
#include "evaluate.h"
#include "search.h"
#include "make_move.h"

constexpr uint16_t MAX_GAME_LENGTH = 96, NUM_TESTS = 200, DELTA = 50;

constexpr float APPLY_FACTOR = 0.2;

float run_match(Board board, int param1, int param2);
float run_game(Board board, int param1, int param2);

int main(int argc, char *argv[]) {
    init_zobrist();
    init_move_tables();
    init_evaluation_parameters();

    // Setup the board and card randomisation.
    Board board = {};
    auto rng = std::default_random_engine{};
    Card cards[CARD_NUM];
    for (int i = 0; i < CARD_NUM; ++i)
        cards[i] = (Card) i;

    // Run the tests.
    PARAM = 610;
    for (uint16_t i = 0; i < NUM_TESTS; ++i) {
        // Randomise the board's cards.
        std::shuffle(std::begin(cards), std::end(cards), rng);
        board.cards[WHITE][0] = cards[0];
        board.cards[WHITE][1] = cards[1];
        board.cards[BLACK][0] = cards[2];
        board.cards[BLACK][1] = cards[3];
        board.side_card = cards[4];

        float result = run_match(board, PARAM + DELTA, PARAM - DELTA);
        result = ((result > 0) - (result < 0));  // Change to sign.
        int param_change = DELTA * APPLY_FACTOR * result;
        std::cout << (i + 1) << ": " << PARAM << " + " << param_change << " = "
                  << PARAM + param_change << " (" << result << ")" << std::endl;
        PARAM += param_change;
    }

    return 0;
}

float run_game(Board board, int param1, int param2) {
    float param_prev = PARAM;
    while (true) {
        // Termination.
        if (board.game_over()) {
            PARAM = param_prev;
            return (board.turn ? 1 : -1);
        }
        if (board.move_count == MAX_GAME_LENGTH) {
            PARAM = param_prev;
            return 0;
        }

        PARAM = (board.turn ? param2 : param1);
        Move move = start_search(&board, 0.01, true, 1);
        make_move(&board, move);
    }
}

float run_match(Board board, int param1, int param2) {
    return (run_game(board, param1, param2) - run_game(board, param2, param1)) / 2;
}
