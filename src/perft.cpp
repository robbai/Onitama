#include "perft.h"

#include <cassert>
#include <ctime>
#include <string>

#include "board.h"
#include "make_move.h"
#include "move_gen.h"

constexpr uint8_t MAX_PERFT_DEPTH = 9;

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

void perft_routine() {
    // Setup board.
    Board board;
    board.cards[0][0] = OX;
    board.cards[0][1] = BOAR;
    board.cards[1][0] = HORSE;
    board.cards[1][1] = ELEPHANT;
    board.side_card = CRAB;

    const uint64_t ORACLE[] = {
            1,       10,        130,        1989,        28509,        487780,
            7748422, 137281607, 2353802670, 41817124521, 746335807162,
    };

    for (int depth = 0; depth <= MAX_PERFT_DEPTH; ++depth) {
        clock_t start = clock();
        uint64_t result = do_perft(&board, depth);
        double duration = (clock() - start) / static_cast<double>(CLOCKS_PER_SEC);
        double speed = static_cast<double>(result) / duration / 1000000;
        printf("Depth %i:%12llu nodes (%.5ss, %5.5s Mnps)\n", depth, result,
               std::to_string(duration).c_str(), std::to_string(speed).c_str());
        assert(ORACLE[depth] == result);
    }
}
