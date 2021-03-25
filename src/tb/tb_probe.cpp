#include "tb_probe.h"


bool GENERATED_TB = false;

TBEntry *entries;

Position to_position(Board *p_board);

TBEntry probe_tb(Board *board) {
    Position pos = to_position(board);
    Index index = get_index(&pos);
    return entries[index];
}

Position to_position(Board *board) {
    Position pos = {};
    pos.turn = board->turn;
    pos.pieces[WHITE] = board->pieces[WHITE][STUDENT] | board->pieces[WHITE][MASTER];
    pos.pieces[BLACK] = board->pieces[BLACK][STUDENT] | board->pieces[BLACK][MASTER];
    pos.masters = board->pieces[WHITE][MASTER] | board->pieces[BLACK][MASTER];
    pos.cards = (1ull << board->cards[WHITE][0]);
    pos.cards |= (1ull << board->cards[WHITE][1]);
    pos.cards |= (65536ull << board->cards[BLACK][0]);
    pos.cards |= (65536ull << board->cards[BLACK][1]);
    pos.cards |= (4294967296ull << board->side_card);
    return pos;
}

void setup_and_generate_tb(Board *board) {
    Tablebase::STUDENT_MEN = 1;
    Tablebase::CARD_LIST[0] = board->cards[WHITE][0];
    Tablebase::CARD_LIST[1] = board->cards[WHITE][1];
    Tablebase::CARD_LIST[2] = board->cards[BLACK][0];
    Tablebase::CARD_LIST[3] = board->cards[BLACK][1];
    Tablebase::CARD_LIST[4] = board->side_card;
    entries = generate_tb();
    GENERATED_TB = true;
}

void drop_tb() {
    delete entries;
}
