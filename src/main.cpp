#include "move_tables.h"
#include "tt/zobrist.h"
#include "evaluate.h"
#include "tune.h"
#include "search.h"

int main() {
    init_zobrist();
    init_move_tables();
    init_evaluation_parameters();

    return run_tune(2, 0.1, false);
}
