#include "move_tables.h"
#include "client.h"
#include "tt/zobrist.h"
#include "evaluate.h"
#include "runner/runner.h"
#include "search.h"
#include "nnue/generate_data.h"
#include "nnue/network.h"

int main(int argc, char *argv[]) {
    init_zobrist();
    init_move_tables();
    init_evaluation_parameters();
    init_search();
    init_network();

    if (argc > 1) {
        if (!strcmp(argv[1], "generate"))
            return generate_data();
        return use_runner();
    }

    std::string match_id;
    std::cout << "Match ID: ";
    getline(std::cin, match_id);

    Client client(match_id);
    return client.loop();
}
