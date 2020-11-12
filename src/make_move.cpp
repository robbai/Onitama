#include <utility>

#include "make_move.h"
#include "move_bits.h"

void make_move(Board *board, Move move) {
    Bitboard xor_board = MoveBits::xor_board(move);

    // Move the piece.
    board->pieces[board->turn][MoveBits::piece_type(move)] ^= xor_board;

    // Optionally capture a student.
    board->pieces[!board->turn][STUDENT] &= ~xor_board;

    // Swap the used-card and side-card.
    std::swap(board->cards[board->turn][MoveBits::card_index(move)], board->side_card);

    // Swap the turn.
    board->turn = !board->turn;
    ++board->move_count;
}

void undo_move(Board *board, Move move) {
    Bitboard xor_board = MoveBits::xor_board(move);
    bool piece_type = MoveBits::piece_type(move);

    // Swap the turn.
    board->turn = !board->turn;
    --board->move_count;

    // Optionally un-capture a student.
    if (MoveBits::capture(move))
        board->pieces[!board->turn][STUDENT] |=
                board->pieces[board->turn][piece_type] & xor_board;

    // Move the piece.
    board->pieces[board->turn][piece_type] ^= xor_board;

    // Swap the used-card and side-card.
    std::swap(board->cards[board->turn][MoveBits::card_index(move)], board->side_card);
}
