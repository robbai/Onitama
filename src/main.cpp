#include "move_tables.h"
#include "tb/tb_gen.h"

int main() {
    init_move_tables();

    generate_tb();
}
