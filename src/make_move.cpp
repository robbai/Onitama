#include <utility>

#include "make_move.h"
#include "move_bits.h"
#include "tt/zobrist.h"

void make_move(Board *board, Move move) {
    Bitboard xor_board = MoveBits::xor_board(move);
    uint8_t from = MoveBits::from(move);
    uint8_t to = MoveBits::to(move);

    // Move the piece.
    bool piece_type = MoveBits::piece_type(move);
    board->hash ^= Zobrist::PIECES[board->turn][piece_type][from];
    board->pieces[board->turn][piece_type] ^= xor_board;
    board->hash ^= Zobrist::PIECES[board->turn][piece_type][to];

    // Optionally capture a student.
    Bitboard captured = (board->pieces[!board->turn][STUDENT] & xor_board);
    if (captured) {
        board->hash ^= Zobrist::PIECES[!board->turn][STUDENT][to];
        board->pieces[!board->turn][STUDENT] ^= captured;
        --board->student_count;
    }

    // Swap the used-card and side-card.
    bool card_index = MoveBits::card_index(move);
    board->hash ^= Zobrist::CARDS[board->cards[board->turn][card_index]][board->turn];
    board->hash ^= Zobrist::CARDS[board->cards[board->turn][card_index]][2];
    board->hash ^= Zobrist::CARDS[board->side_card][board->turn];
    board->hash ^= Zobrist::CARDS[board->side_card][2];
    std::swap(board->cards[board->turn][card_index], board->side_card);

    // Keep hands in ascending order.
    if (MoveBits::lower_swap(move))
        std::swap(board->cards[board->turn][0], board->cards[board->turn][1]);

    // Swap the turn.
    board->turn = !board->turn;
    board->hash ^= Zobrist::TURN;
    ++board->move_count;
}

void undo_move(Board *board, Move move) {
    Bitboard xor_board = MoveBits::xor_board(move);
    uint8_t from = MoveBits::from(move);
    uint8_t to = MoveBits::to(move);
    bool piece_type = MoveBits::piece_type(move);

    // Swap the turn.
    board->turn = !board->turn;
    board->hash ^= Zobrist::TURN;
    --board->move_count;

    // Optionally un-capture a student.
    if (MoveBits::capture(move)) {
        Bitboard captured = 1u << to;
        board->pieces[!board->turn][STUDENT] |= captured;
        board->hash ^= Zobrist::PIECES[!board->turn][STUDENT][to];
        ++board->student_count;
    }

    // Move the piece.
    board->hash ^= Zobrist::PIECES[board->turn][piece_type][from];
    board->pieces[board->turn][piece_type] ^= xor_board;
    board->hash ^= Zobrist::PIECES[board->turn][piece_type][to];

    // Keep hands in ascending order.
    if (MoveBits::lower_swap(move))
        std::swap(board->cards[board->turn][0], board->cards[board->turn][1]);

    // Swap the used-card and side-card.
    bool card_index = MoveBits::card_index(move);
    board->hash ^= Zobrist::CARDS[board->cards[board->turn][card_index]][board->turn];
    board->hash ^= Zobrist::CARDS[board->cards[board->turn][card_index]][2];
    board->hash ^= Zobrist::CARDS[board->side_card][board->turn];
    board->hash ^= Zobrist::CARDS[board->side_card][2];
    std::swap(board->cards[board->turn][card_index], board->side_card);
}
