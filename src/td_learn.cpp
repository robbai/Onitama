#include <cmath>
#include <iostream>
#include "td_learn.h"
#include "evaluate.h"
#include "search.h"
#include "make_move.h"

// Controls the effective maximum alpha.
constexpr float LEARN_RATE = 10;

/*
 * Lambda controls how much the later score differences in a game influence the
 * contribution of a particular position's score derivative.
*/
constexpr float LAMBDA = 0.98;

// Alpha controls size of the parameter updates.
float ALPHA[TOTAL_PARAMETERS];

constexpr float CENTISTUDENT_FACTOR = 0.0005;

constexpr int MAX_GAME_LENGTH = 96;

void init_td_learn() {
    std::fill_n(ALPHA, TOTAL_PARAMETERS, 1);
}

void print_parameters(int *parameters);

// https://www.stmintz.com/ccc/index.php?id=117970
void learn_parameters(float game_result, Board *leaves, uint8_t game_length,
                      bool silent = false) {
    // Game result is -1, 0, or 1.

    // Store the current parameters and final updates.
    int parameters[TOTAL_PARAMETERS];
    get_evaluation_parameters(parameters);

    float net_change[TOTAL_PARAMETERS] = {};
    float absolute_change[TOTAL_PARAMETERS] = {};

    // Loop to setup position scores and score differences.
    float s[game_length];  // Array of position scores.
    float d[game_length];  // Array of position score differences.
    for (int m = 0; m < game_length; m++) {
        s[m] = tanh(CENTISTUDENT_FACTOR * evaluate(&leaves[m]));
        if (m)
            d[m - 1] = s[m] - s[m - 1];
        if (m == game_length - 1)
            d[m] = game_result - s[m];
    }

    // Loop over eval parameters.
    for (int p = 0; p < TOTAL_PARAMETERS; p++) {
        float net = 0;

        // Loop over game positions.
        for (int m = 0; m < game_length; m++) {
            // Compute the scoring derivative by adding 1/100 of student.
            if (!is_parameter_used(&leaves[m], p))
                continue;
            parameters[p] += 1;
            set_evaluation_parameters(parameters);
            float ds = (tanh(CENTISTUDENT_FACTOR * evaluate(&leaves[m])) - s[m]) / 0.01;
            parameters[p] -= 1;
            set_evaluation_parameters(parameters);
            if (ds == 0)
                continue;
            //            std::cout << CARD_NAMES[(p / (PLAYERS_NUM * PIECE_TYPES_NUM)) % CARD_NUM]
            //                      << ": " << ds << std::endl;

            // Now sum over all score differences to end of game,
            // weighting down the later positions by lambda^(m-i).
            float added_net = 0;
            for (int n = m; n < game_length; n++)
                added_net += pow(LAMBDA, n - m) * d[n];

            // Add in the contribution of this position to the parameter update.
            added_net *= ds;
            net += added_net;
            absolute_change[p] += abs(added_net);
        }

        // Update the scoring parameter.
        net_change[p] = net;
    }

    // Update the parameters and alpha.
    for (int p = 0; p < TOTAL_PARAMETERS; p++) {
        parameters[p] += LEARN_RATE * ALPHA[p] * net_change[p];
        if (absolute_change[p] != 0)
            ALPHA[p] = abs(net_change[p]) / absolute_change[p];
    }

    // Print the updated parameters.
    if (!silent)
        print_parameters(parameters);

    set_evaluation_parameters(parameters);
}

void print_parameters(int *parameters) {
    for (int p = 0; p < TOTAL_PARAMETERS; p++)
        std::cout << (p ? ", " : "{") << parameters[p];
    std::cout << "}" << std::endl << std::endl;
}

void learn_game(Board *board, bool silent) {
    Board leaves[MAX_GAME_LENGTH];
    float game_result = 0;

    while (true) {
        // Termination.
        if (board->game_over()) {
            game_result = (board->turn ? 1 : -1);
            break;
        }
        if (board->move_count == MAX_GAME_LENGTH)
            break;

        Move move = start_search(board, true, 0.001);

        // Print progression of game.
        if (!silent)
            std::cout << board->move_count << " ";

        // Store leaf and make move.
        Board leaf = get_pv_leaf(*board);
        leaves[board->move_count] = leaf;
        make_move(board, move);
    }

    // Print game result.
    if (!silent)
        std::cout << std::endl << "Done (" << game_result << ")" << std::endl;

    learn_parameters(game_result, leaves, board->move_count, silent);
}
