#include "generate_data.h"
#include <ctime>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <fstream>
#include <random>
#include "../board.h"
#include "../search.h"
#include "../make_move.h"
#include "../move_gen.h"
#include "../move_bits.h"
#include "../tt/ttable.h"
#include "../evaluate.h"

using std::ofstream;
using std::ostringstream;
using std::string;
using std::vector;

constexpr uint16_t MAX_GAME_LENGTH = 96;

constexpr float SEARCH_TIME = 0.01;

std::default_random_engine RNG;

string to_string(Board *board, float value) {
    ostringstream stream;

    // Cards.
    stream << board->cards[board->turn][0] << " ";
    stream << board->cards[board->turn][1] << " ";
    stream << board->cards[!board->turn][0] << " ";
    stream << board->cards[!board->turn][1] << " ";
    stream << board->side_card << " ";

    // Pieces.
    for (uint8_t turn = 0; turn < 2; ++turn) {
        uint32_t square = __builtin_ctz(board->pieces[turn ^ board->turn][MASTER]);
        stream << (board->turn ? SQUARE_NUM - 1 - square : square) << " ";
        Bitboard pieces = board->pieces[turn ^ board->turn][STUDENT];
        for (int _ = 0; _ < 4; ++_) {
            if (!pieces) {
                stream << "-1 ";
                continue;
            }
            square = __builtin_ctz(pieces);
            stream << (board->turn ? SQUARE_NUM - 1 - square : square) << " ";
            pieces ^= (1u << square);
        }
    }

    stream << value;

    return stream.str();
}

void setup_cards(Board *board, Card *cards) {
    std::shuffle(cards, cards + CARD_NUM, RNG);
    board->cards[WHITE][0] = cards[0];
    board->cards[WHITE][1] = cards[1];
    board->cards[BLACK][0] = cards[2];
    board->cards[BLACK][1] = cards[3];
    board->side_card = cards[4];
    board->turn = RNG() % 2;
}

float to_winrate(int value) {
    return tanh(0.001 * value);
}

uint8_t write_game(Board *board, ofstream *file) {
    latest_search_value = 0;

    // Make random first move.
    Move moves[MAX_MOVES];
    uint8_t size = gen_moves(board, moves);
    make_move(board, moves[RNG() % size]);

    float result = 0;
    vector<string> lines = vector<string>();
    while (board->move_count < MAX_GAME_LENGTH) {
        // Terminate by game-end.
        if (board->game_over()) {
            result = 1;
            break;
        }

        Move move = start_search(board, SEARCH_TIME, true, 1);
        if (!move_exists(board, move)) {
            lines.clear();
            break;
        }

        // Terminate by mate-search.
        if (is_mate_value(latest_search_value)) {
            result = (latest_search_value > 0 ? -1 : 1);
            break;
        }

        // Skip captures (loud positions).
        if (!MoveBits::capture(move)) {
            float value = to_winrate(latest_search_value);
            lines.push_back(to_string(board, value));
        } else {
            lines.push_back("");
        }

        make_move(board, move);
    }

    uint8_t count = 0;
    if (lines.size() % 2 == 0)
        result *= -1;
    for (string line : lines) {
        if (line.size()) {
            *file << line << " " << result << "\n";
            count++;
        }
        result *= -1;
    }

    TTABLE.clear();
    return count;
}

int generate_data(int TARGET_NUM) {
    ofstream file;
    file.open("example.txt", ofstream::app);

    // Setup the board and randomisation.
    RNG = std::default_random_engine{};
    Board board = {};
    Card cards[CARD_NUM];
    for (int i = 0; i < CARD_NUM; ++i)
        cards[i] = (Card)((i + 3) % CARD_NUM);

    clock_t start = clock();
    uint64_t count = 0;
    //    while (count < TARGET_NUM) {
    while (1) {
        board = {};
        setup_cards(&board, cards);
        count += write_game(&board, &file);

        double elapsed = (std::clock() - start) / static_cast<double>(CLOCKS_PER_SEC);
        //        double fraction = static_cast<double>(count) / TARGET_NUM;
        //        std::cout << int(fraction * 1000) / 10.0 << "% ("
        //                  << int(elapsed / fraction - elapsed) << "s)" << std::endl;
        std::cout << count << " (" << int(elapsed) << "s)" << std::endl;
    }

    file.close();

    return 0;
}
