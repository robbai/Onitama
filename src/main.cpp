#include <ctime>
#include <string>

#include "make_move.h"
#include "move_gen.h"
#include "move_tables.h"

constexpr uint8_t MAX_DEPTH = 8;

Move ALL_MOVES[MAX_DEPTH][MAX_MOVES];

uint64_t perft(Board *board, int depth, int ply = 0) {
    if (depth == 0 || board->game_over())
        return 1;

    uint64_t nodes = 0;

    Move *moves = ALL_MOVES[ply];
    uint8_t size = gen_moves(board, moves);
    for (int i = 0; i < size; ++i) {
        make_move(board, moves[i]);
        nodes += perft(board, depth - 1, ply + 1);
        undo_move(board, moves[i]);
    }

    return nodes;
}

int main() {
    init_move_tables();

    // Generate moves.
    Board board;
    board.cards[0][0] = OX;
    board.cards[0][1] = BOAR;
    board.cards[1][0] = HORSE;
    board.cards[1][1] = ELEPHANT;
    board.side_card = CRAB;

    for (int depth = 0; depth <= MAX_DEPTH; ++depth) {
        clock_t start = clock();
        uint64_t result = perft(&board, depth);
        double duration = (clock() - start) / static_cast<double>(CLOCKS_PER_SEC);
        double speed = static_cast<double>(result) / duration / 1000000;
        printf("Depth %i: %10llu nodes (%.5ss, %5.5s Mnps)\n", depth, result,
               std::to_string(duration).c_str(), std::to_string(speed).c_str());
    }

    return 0;
}
