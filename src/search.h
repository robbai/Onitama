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

    uint16_t counter_hist[PIECE_TYPES_NUM][SQUARE_NUM][PIECE_TYPES_NUM][SQUARE_NUM];
    uint16_t capture_hist[PIECE_TYPES_NUM][SQUARE_NUM][SQUARE_NUM];
    int counter_card[CARD_NUM][CARD_NUM][CARD_NUM][2];
    Move killers[MAX_DEPTH][KILLER_NUM];
    int sort_values[MAX_MOVES] = {};
    Hash hash_line[MAX_DEPTH];
    bool pv_played[MAX_DEPTH];

    void sort_moves(Board *board, Move *moves, uint8_t size, bool captures = false,
                    Move last_move = 0);
    int q_search(Board *board, int alpha, int beta, int ply, Move last_move = 0);
    bool is_killer(uint8_t ply, Move move, uint8_t killer_num = KILLER_NUM);

 public:
    bool stop = false;
    uint8_t th = 0;
    Move move_lists[MAX_DEPTH][MAX_MOVES];
    int root_depth;

    void reset();
    int search(Board *board, int depth, int alpha, int beta, int ply = 0,
               bool cut_node = false, Move last_move = 0, bool check_tb = false,
               Move excluded_move = 0);
};

#endif  // ONITAMA_SEARCH_H
