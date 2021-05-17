#include <ctime>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <random>
#include "tune.h"
#include "search.h"
#include "evaluate.h"
#include "make_move.h"
#include "tt/ttable.h"

constexpr uint16_t MAX_GAME_LENGTH = 96;

constexpr float SEARCH_TIME = 0.005;

float run_match(Board board, float params1[], float params2[], const bool reinit_search);

// https://www.jhuapl.edu/spsa/PDF-SPSA/Spall_Implementation_of_the_Simultaneous.pdf
int run_tune(const float magnitude, const float initial_delta, const bool reinit_search,
             const float A, const float c) {
    if (!reinit_search)
        init_search();

    const int N = A * 10;
    const float alpha = 0.602, gamma = 0.101;
    const float a = (initial_delta * pow(A + 1, alpha)) / magnitude;
    float params1[PARAM_NUM], params2[PARAM_NUM], deltas[PARAM_NUM];

    // Setup the board and cards.
    Board board = {};
    Card cards[CARD_NUM];
    for (int i = 0; i < CARD_NUM; ++i)
        cards[i] = (Card) i;

    // Setup randomisation.
    auto rng = std::default_random_engine{};
    std::mt19937 mt(rng());
    std::uniform_real_distribution<double> dist(0, 1);

    // Run loop.
    clock_t start = clock();
    for (int k = 0; k < N; k++) {
        std::cout << (k + 1) << ": [";

        // Calculate deltas.
        float ak = a / pow(k + 1 + A, alpha);
        float ck = c / pow(k + 1, gamma);
        for (int i = 0; i < PARAM_NUM; ++i) {
            deltas[i] = 2 * round(dist(mt)) - 1;
            params1[i] = PARAMS[i].value + ck * deltas[i];
            params2[i] = PARAMS[i].value - ck * deltas[i];

            params1[i] =
                    std::max(PARAMS[i].minimum, std::min(PARAMS[i].maximum, params1[i]));
            params2[i] =
                    std::max(PARAMS[i].minimum, std::min(PARAMS[i].maximum, params2[i]));
        }

        // Run the match.
        std::shuffle(std::begin(cards), std::end(cards), rng);
        board = {};
        board.cards[WHITE][0] = cards[0];
        board.cards[WHITE][1] = cards[1];
        board.cards[BLACK][0] = cards[2];
        board.cards[BLACK][1] = cards[3];
        board.side_card = cards[4];
        board.turn = cards[5] % 2;
        std::fill(USED_PARAM, USED_PARAM + PARAM_NUM, 0);
        float match = run_match(board, params1, params2, reinit_search);

        // Apply change.
        for (int i = 0; i < PARAM_NUM; ++i) {
            if (USED_PARAM[i]) {
                PARAMS[i].value += ak * match / (ck * deltas[i]);
                PARAMS[i].value = std::max(PARAMS[i].minimum,
                                           std::min(PARAMS[i].maximum, PARAMS[i].value));
            }
            std::cout << PARAMS[i].value << (i == PARAM_NUM - 1 ? "]" : ", ");
        }

        double elapsed = (std::clock() - start) / static_cast<double>(CLOCKS_PER_SEC);
        double fraction = static_cast<double>(k + 1) / N;
        std::cout << " (" << int(elapsed / fraction - elapsed) << "s)" << std::endl;
    }

    return 0;
}

float run_game(Board board, float params1[], float params2[], const bool reinit_search) {
    float params_prev[PARAM_NUM];
    for (int i = 0; i < PARAM_NUM; ++i)
        params_prev[i] = PARAMS[i].value;

    while (1) {
        // Termination.
        if (board.game_over()) {
            for (int i = 0; i < PARAM_NUM; ++i)
                PARAMS[i].value = params_prev[i];
            return (board.turn ? 1 : -1);
        }
        if (board.move_count == MAX_GAME_LENGTH) {
            for (int i = 0; i < PARAM_NUM; ++i)
                PARAMS[i].value = params_prev[i];
            return 0;
        }

        float *params = board.turn ? params2 : params1;
        for (int i = 0; i < PARAM_NUM; ++i)
            PARAMS[i].value = params[i];
        if (reinit_search)
            init_search();
        Move move = start_search(&board, SEARCH_TIME, true, 1);
        make_move(&board, move);
        TTABLE.clear();
    }
}

float run_match(Board board, float params1[], float params2[], const bool reinit_search) {
    return (run_game(board, params1, params2, reinit_search) -
            run_game(board, params2, params1, reinit_search)) /
           2;
}
