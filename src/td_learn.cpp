#include <math.h>
#include <iostream>
#include "td_learn.h"
#include "evaluate.h"
#include "search.h"
#include "make_move.h"
#include "util.h"

// Alpha controls size of the parameter updates.
constexpr float ALPHA = 10.0;

/*
 * Lambda controls how much the later score differences in a game influence the
 * contribution of a particular position's score derivative.
*/
constexpr float LAMBDA = 0.9;

constexpr float CENTISTUDENT_FACTOR = 0.005;

constexpr int MAX_GAME_LENGTH = 64;

void write_parameters(int *parameters);

// https://www.stmintz.com/ccc/index.php?id=117970
void learn_parameters(float game_result, Board *leaves, uint8_t game_length) {
    // Game result is -1, 0, or 1.

    int parameters[TOTAL_PARAMETERS];
    get_evaluation_parameters(parameters);
    float parameter_updates[TOTAL_PARAMETERS] = {};

    // Loop to setup position scores and score differences.
    float s[game_length];  // Array of position scores.
    float d[game_length];  // Array of position score differences.
    for (int i = 0; i < game_length; i++) {
        s[i] = tanh(CENTISTUDENT_FACTOR * evaluate(&leaves[i]));
        if (i)
            d[i - 1] = s[i] - s[i - 1];
        if (i == game_length - 1)
            d[i] = game_result - s[i];
    }

    // Loop over eval parameters.
    for (int j = 0; j < TOTAL_PARAMETERS; j++) {
        float sum1 = 0;

        // Loop over game positions.
        for (int i = 0; i < game_length; i++) {
            // Compute the scoring derivative by adding 1/100 of student.
            parameters[j] += 1;
            set_evaluation_parameters(parameters);
            float ds = (tanh(CENTISTUDENT_FACTOR * evaluate(&leaves[i])) - s[i]) / 0.01;
            parameters[j] -= 1;
            set_evaluation_parameters(parameters);
            if (ds == 0)
                continue;
            //            std::cout << CARD_NAMES[(j / 2) % CARD_NUM] << ": " << ds << std::endl;

            // Now sum over all score differences to end of game,
            // weighting down the later positions by lambda^(m-i).
            float sum2 = 0;
            for (int m = i; m < game_length; m++)
                sum2 += pow(LAMBDA, m - i) * d[m];

            // Add in the contribution of this position to the parameter update.
            sum1 += ds * sum2;
        }

        // Update the scoring parameter.
        parameter_updates[j] += ALPHA * sum1;
    }

    // Update the parameters.
    for (int j = 0; j < TOTAL_PARAMETERS; j++)
        parameters[j] += parameter_updates[j];

    // Write the updated parameters out to a file.
    write_parameters(parameters);

    set_evaluation_parameters(parameters);
}

void write_parameters(int *parameters) {
    for (int i = 0; i < TOTAL_PARAMETERS; i++)
        std::cout << (i ? ", " : "{") << parameters[i];
    std::cout << "}" << std::endl << std::endl;
}

void learn_game(Board *board) {
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

        //        std::cout << std::endl
        //                  << pretty_board(board) << std::endl << std::endl;
        std::cout << board->move_count << " ";

        Board leaf = get_pv_leaf(*board);
        leaves[board->move_count] = leaf;

        make_move(board, move);
    }

    std::cout << std::endl
              << pretty_board(board) << std::endl
              << "Done (" << game_result << ")" << std::endl;

    learn_parameters(game_result, leaves, board->move_count);
}
