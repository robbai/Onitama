#include <ctime>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <random>
#include <vector>
#include "tune.h"
#include "search.h"
#include "make_move.h"
#include "tt/ttable.h"

constexpr uint16_t MAX_GAME_LENGTH = 256;

constexpr float SEARCH_TIME = 0.1;

float run_match(Board board, float params1[], float params2[], const bool reinit_search);

// https://www.jhuapl.edu/spsa/PDF-SPSA/Spall_Implementation_of_the_Simultaneous.pdf
int run_tune(const float magnitude, const float initial_delta, const int iterations,
             const bool reinit_search) {
    if (!reinit_search)
        init_search();

    const float alpha = 0.602, gamma = 0.101;

    const float A = iterations / 10.0;
    const float c = 0.5;
    const float a = pow(A + 1, alpha) * initial_delta / magnitude;

    // Setup the board and cards.
    Board board;
    Card cards[CARD_NUM];
    for (int i = 0; i < CARD_NUM; ++i)
        cards[i] = (Card) i;

    // Setup randomisation.
    auto rng = std::default_random_engine{};
    std::mt19937 mt(rng());
    std::uniform_real_distribution<double> dist(0, 1);

    // Run loop.
    clock_t start = clock();
    float params1[PARAM_NUM], params2[PARAM_NUM], delta_k[PARAM_NUM];
    for (int k = 1; k <= iterations; ++k) {
        std::cout << k << ": ";

        // Calculate delta.
        float a_k = a / pow(k + A, alpha);
        float c_k = c / pow(k, gamma);
        for (int i = 0; i < PARAM_NUM; ++i) {
            delta_k[i] = (2 * round(dist(mt)) - 1) * magnitude;
            params1[i] = PARAMS[i].value + c_k * delta_k[i];
            params2[i] = PARAMS[i].value - c_k * delta_k[i];

            params1[i] =
                    std::max(PARAMS[i].minimum, std::min(PARAMS[i].maximum, params1[i]));
            params2[i] =
                    std::max(PARAMS[i].minimum, std::min(PARAMS[i].maximum, params2[i]));

            std::cout << (i ? ", (" : "(") << params1[i] << ", " << params2[i] << ")";
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
        float L = run_match(board, params1, params2, reinit_search);

        // Apply change.
        std::cout << " -> [";
        for (int i = 0; i < PARAM_NUM; ++i) {
            PARAMS[i].value += a_k * L * delta_k[i] / (2 * c_k);
            PARAMS[i].value = std::max(PARAMS[i].minimum,
                                       std::min(PARAMS[i].maximum, PARAMS[i].value));
            std::cout << PARAMS[i].value << (i == PARAM_NUM - 1 ? "]" : ", ");
        }

        double elapsed = (std::clock() - start) / static_cast<double>(CLOCKS_PER_SEC);
        double fraction = static_cast<double>(k) / iterations;
        std::cout << " (" << int(elapsed / fraction - elapsed) << "s)" << std::endl;
    }

    return 0;
}

float run_game(Board board, float params1[], float params2[], const bool reinit_search) {
    float params_prev[PARAM_NUM];
    for (int i = 0; i < PARAM_NUM; ++i)
        params_prev[i] = PARAMS[i].value;

    std::vector<Hash> history = std::vector<Hash>();
    while (1) {
        // Terminate by game-end.
        if (board.game_over()) {
            for (int i = 0; i < PARAM_NUM; ++i)
                PARAMS[i].value = params_prev[i];
            return board.turn ? 1 : -1;
        }

        // Terminate by repetition.
        if (board.move_count == MAX_GAME_LENGTH ||
            std::find(history.begin(), history.end(), board.hash) != history.end()) {
            for (int i = 0; i < PARAM_NUM; ++i)
                PARAMS[i].value = params_prev[i];
            return 0;
        }
        history.push_back(board.hash);

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
    return run_game(board, params1, params2, reinit_search) -
           run_game(board, params2, params1, reinit_search);
}
