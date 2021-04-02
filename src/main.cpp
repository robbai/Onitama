#include <iostream>
#include <algorithm>
#include <random>
#include "move_tables.h"
#include "tt/zobrist.h"
#include "evaluate.h"
#include "search.h"
#include "make_move.h"

constexpr uint16_t MAX_GAME_LENGTH = 96;

// Tune setup.
constexpr float DEFAULT_PARAM = 0.6, MIN_PARAM = 0.1, MAX_PARAM = 5, DELTA = 0.2,
                APPLY_FACTOR = (0.05 / 0.2), SEARCH_TIME = 0.05;

float run_match(Board board, float param1, float param2);
float run_game(Board board, float param1, float param2);

int main(int argc, char *argv[]) {
    init_zobrist();
    init_move_tables();
    init_evaluation_parameters();
    init_search();

    // Setup the board and card randomisation.
    Board board = {};
    auto rng = std::default_random_engine{};
    Card cards[CARD_NUM];
    for (int i = 0; i < CARD_NUM; ++i)
        cards[i] = (Card) i;

    // Run the tests.
    PARAM = DEFAULT_PARAM;
    int iter = 1;
    while (1) {
        // Randomise the board's cards.
        std::shuffle(std::begin(cards), std::end(cards), rng);
        board.cards[WHITE][0] = cards[0];
        board.cards[WHITE][1] = cards[1];
        board.cards[BLACK][0] = cards[2];
        board.cards[BLACK][1] = cards[3];
        board.side_card = cards[4];

        // Get result.
        float result = run_match(board, PARAM + DELTA, PARAM - DELTA);

        // Apply change.
        result = ((result > 0) - (result < 0));  // Change to sign.
        float param_change = DELTA * APPLY_FACTOR * result;
        std::cout << iter << ": " << PARAM << " + " << param_change << " = "
                  << PARAM + param_change << " (" << result << ")" << std::endl;
        PARAM += param_change;
        if (PARAM < MIN_PARAM) {
            PARAM = MIN_PARAM;
        } else if (PARAM > MAX_PARAM) {
            PARAM = MAX_PARAM;
        }

        ++iter;
    }

    return 0;
}

float run_game(Board board, float param1, float param2) {
    const float param_prev = PARAM;

    while (1) {
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
        init_search();
        Move move = start_search(&board, SEARCH_TIME, true, 1);
        make_move(&board, move);
    }
}

float run_match(Board board, float param1, float param2) {
    return (run_game(board, param1, param2) - run_game(board, param2, param1)) / 2;
}
