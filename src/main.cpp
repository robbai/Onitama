#include "move_tables.h"
#include "tt/zobrist.h"
#include "evaluate.h"
#include "search.h"
#include "nnue/network.h"

int main() {
    init_zobrist();
    init_move_tables();
    init_evaluation_parameters();
    init_search();
    init_network();

    return run_tune(1, 0.3, 500, false);
}
