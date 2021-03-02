#include "move_gen.h"

#include "move_bits.h"
#include "move_tables.h"

uint8_t gen_moves(Board *board, Move *moves, Bitboard targets) {
    uint8_t total = 0;

    // Iterate through pieces.
    Bitboard pieces =
            board->pieces[board->turn][STUDENT] | board->pieces[board->turn][MASTER];
    targets &= ~pieces;
    while (pieces) {
        uint8_t from = __builtin_ctz(pieces);
        Bitboard mask_1 = (1u << from);

        // Iterate through cards.
        for (int card_index = 0; card_index < CARDS_EACH_NUM; ++card_index) {
            bool lower_swap = (card_index == (board->side_card <
                                              board->cards[board->turn][1 - card_index]));

            // Iterate through move squares and create moves.
            Bitboard squares =
                    MOVE_TABLES[(board->cards[board->turn][card_index] * SQUARE_NUM +
                                 from) * PLAYERS_NUM +
                                board->turn] &
                    targets;
            Move move = MoveBits::half_create_move(
                    from, card_index, mask_1 & board->pieces[board->turn][MASTER],
                    lower_swap);
            while (squares) {
                uint8_t to = __builtin_ctz(squares);
                Bitboard mask_2 = (1u << to);

                moves[total] = MoveBits::finish_create_move(
                        move, to, mask_2 & board->pieces[!board->turn][STUDENT]);
                ++total;

                squares ^= mask_2;
            }
        }

        pieces ^= mask_1;
    }

    return total;
}

uint8_t count_moves(Board *board) {
    uint8_t total = 0;

    // Iterate through pieces.
    Bitboard pieces =
            board->pieces[board->turn][STUDENT] | board->pieces[board->turn][MASTER];
    Bitboard targets = ~pieces;
    while (pieces) {
        uint8_t from = __builtin_ctz(pieces);

        // Iterate through cards.
        for (int card_index = 0; card_index < CARDS_EACH_NUM; ++card_index) {
            // Add move squares.
            total += __builtin_popcount(
                    MOVE_TABLES[(board->cards[board->turn][card_index] * SQUARE_NUM +
                                 from) * PLAYERS_NUM +
                                board->turn] &
                    targets);
        }

        pieces ^= (1u << from);
    }

    return total;
}

bool move_exists(Board *board, Move move) {
    uint8_t from = MoveBits::from(move);
    bool piece_type = MoveBits::piece_type(move);

    // No piece to start with.
    if (!((1u << from) & board->pieces[board->turn][piece_type]))
        return false;

    // Already a friendly on that destination.
    uint8_t to = MoveBits::to(move);
    if ((1u << to) &
        (board->pieces[board->turn][STUDENT] | board->pieces[board->turn][MASTER]))
        return false;

    // Must be a capture.
    if (MoveBits::capture(move) && !((1u << to) & (board->pieces[!board->turn][STUDENT] |
                                                   board->pieces[!board->turn][MASTER])))
        return false;

    // Card cannot move to this destination.
    Card card = board->cards[board->turn][MoveBits::card_index(move)];
    Bitboard squares =
            MOVE_TABLES[(card * SQUARE_NUM + from) * PLAYERS_NUM + board->turn];
    return squares & (1u << to);
}
