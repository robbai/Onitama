#include <utility>

#include "make_move.h"
#include "move_bits.h"
#include "tt/zobrist.h"

void make_move(Board *board, Move move) {
    Bitboard xor_board = MoveBits::xor_board(move);

    // Move the piece.
    bool piece_type = MoveBits::piece_type(move);
    board->hash ^= Zobrist::PIECES[board->turn][piece_type][__builtin_ctz(
            board->pieces[board->turn][piece_type] & xor_board)];
    board->pieces[board->turn][piece_type] ^= xor_board;
    board->hash ^= Zobrist::PIECES[board->turn][piece_type][__builtin_ctz(
            board->pieces[board->turn][piece_type] & xor_board)];

    // Optionally capture a student.
    Bitboard captured = (board->pieces[!board->turn][STUDENT] & xor_board);
    if (captured) {
        board->hash ^= Zobrist::PIECES[!board->turn][STUDENT][__builtin_ctz(captured)];
        board->pieces[!board->turn][STUDENT] ^= captured;
    }

    // Swap the used-card and side-card.
    bool card_index = MoveBits::card_index(move);
    board->hash ^= Zobrist::CARDS[board->cards[board->turn][card_index]][board->turn];
    board->hash ^= Zobrist::CARDS[board->cards[board->turn][card_index]][2];
    board->hash ^= Zobrist::CARDS[board->side_card][board->turn];
    board->hash ^= Zobrist::CARDS[board->side_card][2];
    std::swap(board->cards[board->turn][card_index], board->side_card);

    // Swap the turn.
    board->turn = !board->turn;
    board->hash ^= Zobrist::TURN;
    ++board->move_count;
}

void undo_move(Board *board, Move move) {
    Bitboard xor_board = MoveBits::xor_board(move);
    bool piece_type = MoveBits::piece_type(move);

    // Swap the turn.
    board->turn = !board->turn;
    board->hash ^= Zobrist::TURN;
    --board->move_count;

    // Optionally un-capture a student.
    if (MoveBits::capture(move)) {
        Bitboard captured = board->pieces[board->turn][piece_type] & xor_board;
        board->pieces[!board->turn][STUDENT] |= captured;
        board->hash ^= Zobrist::PIECES[!board->turn][STUDENT][__builtin_ctz(captured)];
    }

    // Move the piece.
    board->hash ^= Zobrist::PIECES[board->turn][piece_type][__builtin_ctz(
            board->pieces[board->turn][piece_type] & xor_board)];
    board->pieces[board->turn][piece_type] ^= xor_board;
    board->hash ^= Zobrist::PIECES[board->turn][piece_type][__builtin_ctz(
            board->pieces[board->turn][piece_type] & xor_board)];

    // Swap the used-card and side-card.
    bool card_index = MoveBits::card_index(move);
    board->hash ^= Zobrist::CARDS[board->cards[board->turn][card_index]][board->turn];
    board->hash ^= Zobrist::CARDS[board->cards[board->turn][card_index]][2];
    board->hash ^= Zobrist::CARDS[board->side_card][board->turn];
    board->hash ^= Zobrist::CARDS[board->side_card][2];
    std::swap(board->cards[board->turn][card_index], board->side_card);
}
