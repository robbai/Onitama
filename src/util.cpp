#include "util.h"


string pretty_bitboard(Bitboard bitboard) {
    string str;
    for (int rank = (BOARD_LENGTH - 1); rank >= 0; --rank) {
        for (int file = 0; file < BOARD_LENGTH; ++file) {
            str += (bitboard & (1 << (file + rank * BOARD_LENGTH)) ? "X " : ". ");
        }
        str += "\n";
    }
    return str;
}


string pretty_board(Board *board) {
    const string CARD_NAMES[] = {
            "Rabbit",
            "Monkey",
            "Boar",
            "Goose",
            "Cobra",
            "Crab",
            "Horse",
            "Dragon",
            "Rooster",
            "Crane",
            "Elephant",
            "Mantis",
            "Tiger",
            "Frog",
            "Ox",
            "Eel",
    };

    string str;
    for (int rank = (BOARD_LENGTH - 1); rank >= 0; --rank) {
        for (int file = 0; file < BOARD_LENGTH; ++file) {
            Bitboard mask = (1 << (file + rank * BOARD_LENGTH));
            char character;
            if (mask & board->pieces[0][STUDENT]) {
                character = 'x';
            } else if (mask & board->pieces[0][MASTER]) {
                character = 'X';
            } else if (mask & board->pieces[1][STUDENT]) {
                character = 'o';
            } else if (mask & board->pieces[1][MASTER]) {
                character = 'O';
            } else {
                character = '.';
            }
            str += character;
            str += " ";
        }
        str += "\n";
    }
    str += "Side to move: ";
    str += (board->turn ? "Black" : "White");
    str += "\nWhite's cards: ";
    for (Card card : board->cards[0]) str += CARD_NAMES[card] + ", ";
    str = str.substr(0, str.length() - 2) + "\n";
    str += "Black's cards: ";
    for (Card card : board->cards[1]) str += CARD_NAMES[card] + ", ";
    str = str.substr(0, str.length() - 2) + "\nSide card: " + CARD_NAMES[board->side_card];
    return str;
}
