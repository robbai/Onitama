#ifndef ONITAMA_SEARCH_H
#define ONITAMA_SEARCH_H

#include <thread>
#include "board.h"

constexpr uint8_t MAX_DEPTH = 64;

const uint8_t MAX_THREADS = std::thread::hardware_concurrency();

void init_search();

Move start_search(Board *board, float search_time = 1, bool silent = false,
                  uint8_t num_threads = MAX_THREADS);

class Thread {
 private:
    static const uint8_t KILLER_NUM = 2;

    uint16_t history[PLAYERS_NUM][SQUARE_NUM][SQUARE_NUM][PIECE_TYPES_NUM];
    Move counter_move[SQUARE_NUM][SQUARE_NUM][PIECE_TYPES_NUM];
    Move killers[MAX_DEPTH][KILLER_NUM];
    int sort_values[MAX_MOVES] = {};
    Hash hash_line[MAX_DEPTH];

    void sort_moves(Board *board, Move *moves, uint8_t size, Move prev_move,
                    bool captures = false, bool tt_move_exists = false);
    int q_search(Board *board, int alpha, int beta, int ply);
    bool is_killer(uint8_t ply, Move move, uint8_t killer_num = KILLER_NUM);

 public:
    bool stop = false;
    uint8_t th = 0;
    Move move_lists[MAX_DEPTH][MAX_MOVES];

    void reset();
    int search(Board *board, int depth, int alpha, int beta, int ply, bool check_tb,
               Move prev_move);
};

#endif  // ONITAMA_SEARCH_H
