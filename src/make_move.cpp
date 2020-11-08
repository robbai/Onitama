#include "make_move.h"


constexpr int MOVE_MASK = 33554431, STUDENT_CAPTURE_MASK = 67108864;


// Also affects the move's capture bits.
void make_move(Board *board, Move &move) {
    Bitboard move_bits = (move & MOVE_MASK);
    bool card_index = ((move >> SQUARE_NUM) & 1);

    // Move the piece.
    if (board->pieces[board->turn][STUDENT] & move_bits) {
        board->pieces[board->turn][STUDENT] ^= move_bits;
    } else {
        board->pieces[board->turn][MASTER] ^= move_bits;
    }

    // Optionally capture a student.
    Bitboard capture = (board->pieces[!board->turn][STUDENT] & move_bits);
    if (capture) {
        board->pieces[!board->turn][STUDENT] ^= capture;
        move |= STUDENT_CAPTURE_MASK;
    }

    // Swap the used-card and side-card.
    Card temp = board->side_card;
    board->side_card = board->cards[board->turn][card_index];
    board->cards[board->turn][card_index] = temp;

    // Swap the turn.
    board->turn = !board->turn;
}


void undo_move(Board *board, Move &move) {
    Bitboard move_bits = (move & MOVE_MASK);
    bool card_index = ((move >> SQUARE_NUM) & 1);

    // Swap the turn.
    board->turn = !board->turn;

    // Optionally un-capture a student.
    if (move & STUDENT_CAPTURE_MASK) {
        board->pieces[!board->turn][STUDENT] |= (board->pieces[board->turn][STUDENT] | board->pieces[board->turn][MASTER]) & move_bits;
    }

    // Move the piece.
    if (board->pieces[board->turn][STUDENT] & move_bits) {
        board->pieces[board->turn][STUDENT] ^= move_bits;
    } else {
        board->pieces[board->turn][MASTER] ^= move_bits;
    }

    // Swap the used-card and side-card.
    Card temp = board->side_card;
    board->side_card = board->cards[board->turn][card_index];
    board->cards[board->turn][card_index] = temp;
}
