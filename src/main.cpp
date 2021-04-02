#include "move_tables.h"
#include "tt/zobrist.h"
#include "evaluate.h"
#include "tune.h"

int main() {
    init_zobrist();
    init_move_tables();
    init_evaluation_parameters();

    return run_tune(4, 0.2);
}
