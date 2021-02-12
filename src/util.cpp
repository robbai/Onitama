#include <utility>
#include <algorithm>

#include "util.h"
#include "move_bits.h"

std::string pretty_bitboard(Bitboard bitboard, bool card) {
    std::string str = "+-----------+\n";
    for (int rank = (BOARD_LENGTH - 1); rank >= 0; --rank) {
        str += "| ";
        for (int file = 0; file < BOARD_LENGTH; ++file) {
            if (card && rank == BOARD_LENGTH / 2 && file == BOARD_LENGTH / 2) {
                str += "O ";
                continue;
            }
            str += (bitboard & (1 << (file + rank * BOARD_LENGTH)) ? "X " : ". ");
        }
        str += "|\n";
    }
    return str + "+-----------+";
}

std::string pretty_board(Board *board) {
    std::string str = "  +---+---+---+---+---+\n";
    for (int rank = (BOARD_LENGTH - 1); rank >= 0; --rank) {
        for (int file = 0; file < BOARD_LENGTH; ++file) {
            Bitboard mask = ((Bitboard) 1 << (file + rank * BOARD_LENGTH));
            char character;
            if (mask & board->pieces[0][STUDENT]) {
                character = 'x';
            } else if (mask & board->pieces[1][STUDENT]) {
                character = 'o';
            } else if (mask & board->pieces[0][MASTER]) {
                character = 'X';
            } else if (mask & board->pieces[1][MASTER]) {
                character = 'O';
            } else {
                character = '.';
            }
            str += (!file ? std::to_string(rank + 1) + " | " : " | ");
            str += character;
        }
        str += " | \n  +---+---+---+---+---+\n";
    }
    str += "    A   B   C   D   E\n";
    str += "Side to move:  ";
    str += (board->turn ? "Black" : "White");
    str += "\nWhite's cards: ";
    for (Card card : board->cards[0])
        str += CARD_NAMES[card] + ", ";
    str = str.substr(0, str.length() - 2) + "\n";
    str += "Black's cards: ";
    for (Card card : board->cards[1])
        str += CARD_NAMES[card] + ", ";
    str = str.substr(0, str.length() - 2) +
          "\nSide card:     " + CARD_NAMES[board->side_card];
    return str;
}

std::string move_string(Board *board, Move move) {
    int from = MoveBits::from(move);
    int to = MoveBits::to(move);
    return CARD_NAMES[board->cards[board->turn][MoveBits::card_index(move)]] + ":" +
           SQUARE_NAMES[from] + SQUARE_NAMES[to];
}

std::string to_lower(std::string string) {
    std::transform(string.begin(), string.end(), string.begin(), [](unsigned char c) {
        return std::tolower(c);
    });
    return string;
}

bool bump_move(Move *moves, uint8_t size, Move move, uint8_t to) {
    for (uint8_t i = to; i < size; ++i) {
        if (moves[i] == move) {
            std::swap(moves[to], moves[i]);
            return true;
        }
    }
    return false;
}

int partition(Move *moves, int *values, int p, int q) {
    int x = values[p];
    int i = p;
    int j;

    for (j = p + 1; j < q; j++) {
        if (values[j] >= x) {
            i = i + 1;
            std::swap(values[i], values[j]);
            std::swap(moves[i], moves[j]);
        }
    }

    std::swap(values[i], values[p]);
    std::swap(moves[i], moves[p]);
    return i;
}

void quicksort(Move *moves, int *values, int p, int q) {
    int r;
    if (p < q) {
        r = partition(moves, values, p, q);
        quicksort(moves, values, p, r);
        quicksort(moves, values, r + 1, q);
    }
}
