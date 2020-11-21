#include "move_tables.h"
#include "bitbase.h"
#include "board.h"
#include "search.h"

int main() {
    init_move_tables();

    //    std::string match_id;
    //    std::cout << "Match ID:";
    //    std::cin >> match_id;
    //
    //    Client client(match_id);
    //    return client.loop();

    create_bitbase();

    //    Board board;
    //    board.pieces[WHITE][STUDENT] = 1u << 12;
    //    board.pieces[WHITE][MASTER] = 1u << 5;
    //    board.pieces[BLACK][STUDENT] = 1u << 21;
    //    board.pieces[BLACK][MASTER] = 1u << 14;
    //    board.cards[WHITE][0] = BOAR;
    //    board.cards[WHITE][1] = OX;
    //    board.cards[BLACK][0] = HORSE;
    //    board.cards[BLACK][1] = CRANE;
    //    board.side_card = EEL;
    //    start_search(&board);
}
