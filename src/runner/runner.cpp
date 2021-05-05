#include <string>
#include <iostream>
#include <sstream>
#include "runner.h"
#include "../board.h"
#include "../search.h"
#include "../util.h"
#include "../move_gen.h"
#include "../make_move.h"
#include "../version.h"
#include "../tt/ttable.h"
#include "../tt/zobrist.h"

using std::string;


int use_runner() {
    Board board;

    // Input loop.
    string command, input;
    while (1) {
        // Wait for input.
        while (!getline(std::cin, input)) {
        }

        // Get input.
        std::istringstream stream(input);
        command.clear();
        stream >> std::skipws >> command;

        if (command == "quit") {
            break;
        } else if (command == "name") {
            std::cout << "name " << (GIT_BRANCH[0] == 0 ? "unknown" : string(GIT_BRANCH))
                      << std::endl;
        } else if (command == "new") {
            board = Board();
            TTABLE.clear();
            string card_name;
            for (uint8_t i = 0; i < 5; ++i) {
                stream >> card_name;
                Card card = static_cast<Card>(std::stoi(card_name));
                switch (i) {
                    case 0:
                    case 1:
                        board.cards[WHITE][i] = card;
                        break;
                    case 2:
                    case 3:
                        board.cards[BLACK][i - 2] = card;
                        break;
                    case 4:
                        board.side_card = card;
                        break;
                }
            }
            set_hash(&board);
        } else if (command == "get") {
            string search_time;
            stream >> search_time;
            Move move = start_search(&board, std::stof(search_time), false, 1);
            std::cout << "get " << move_string(&board, move) << std::endl;
        } else if (command == "give") {
            Move moves[MAX_MOVES];
            string given_move;
            while (stream >> given_move) {
                uint8_t size = gen_moves(&board, moves);
                for (uint8_t i = 0; i < size; ++i) {
                    const Move move = moves[i];
                    if (move_string(&board, move) == given_move) {
                        make_move(&board, move);
                        break;
                    } else if (i == size - 1) {
                        std::cout << "Couldn't find given move: " << given_move
                                  << std::endl;
                    }
                }
            }
        } else if (command == "moves") {
            Move moves[MAX_MOVES];
            uint8_t size = gen_moves(&board, moves);
            for (uint8_t i = 0; i < size; ++i) {
                const Move move = moves[i];
                std::cout << int(i + 1) << ": " << move_string(&board, move) << std::endl;
            }
        } else if (command == "print") {
            std::cout << pretty_board(&board) << std::endl;
        } else {
            std::cout << "Unknown command: " << command << std::endl;
        }
    }

    return 0;
}
