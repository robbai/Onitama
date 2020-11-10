#include "board.h"
#include "make_move.h"
#include "move_gen.h"

constexpr uint8_t MAX_PERFT_DEPTH = 8;

Move PERFT_MOVES[MAX_PERFT_DEPTH][MAX_MOVES];

uint64_t do_perft(Board *board, int depth, int ply = 0) {
    if (depth <= 0 || board->game_over())
        return 1;

    if (depth == 1)
        return count_moves(board);

    uint64_t nodes = 0;

    Move *moves = PERFT_MOVES[ply];
    uint8_t size = gen_moves(board, moves);
    for (int i = 0; i < size; ++i) {
        make_move(board, moves[i]);
        nodes += do_perft(board, depth - 1, ply + 1);
        undo_move(board, moves[i]);
    }

    return nodes;
}
