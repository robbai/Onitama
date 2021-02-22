#ifndef ONITAMA_SEARCH_H
#define ONITAMA_SEARCH_H


#include "board.h"

constexpr uint8_t MAX_DEPTH = 64;

Move start_search(Board *board, bool silent = false, float max_time = 1);

Board get_pv_leaf(Board board);

struct Line {
    int length = 0;         // Number of moves in the line.
    Move moves[MAX_DEPTH];  // The line.
};

class Thread {
 private:
    uint16_t history[PLAYERS_NUM][SQUARE_NUM][SQUARE_NUM][PIECE_TYPES_NUM];
    int sort_values[MAX_MOVES] = {};

    void sort_moves(Board *board, Move *moves, uint8_t size, uint8_t bump);
    int q_search(Board *board, int alpha, int beta, int ply);

 public:
    Line pv_line = {};
    bool stop = false;
    uint8_t th = 0;
    Move move_lists[MAX_DEPTH][MAX_MOVES];

    void reset_history();
    Board get_pv_leaf(Board board);
    int search(Board *board, int depth, int alpha, int beta, int ply, bool check_tb,
               bool pv_node, bool following_pv, Line *curr_line);
};

#endif  // ONITAMA_SEARCH_H
