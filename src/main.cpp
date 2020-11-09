#include "make_move.h"
#include "move_tables.h"
#include "search.h"

int main() {
    init_move_tables();

    // Generate moves.
    Board board;
    board.cards[0][0] = OX;
    board.cards[0][1] = BOAR;
    board.cards[1][0] = HORSE;
    board.cards[1][1] = ELEPHANT;
    board.side_card = CRAB;

    search(&board);

    return 0;
}
