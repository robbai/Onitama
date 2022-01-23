#include "move_tables.h"
#include "client.h"
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

    std::string limited_depth;
    std::cout << "Depth limit: ";
    getline(std::cin, limited_depth);
    LIMITED_DEPTH = std::stoi(limited_depth);

    std::string match_id;
    std::cout << "Match ID: ";
    getline(std::cin, match_id);

    Client client(match_id);
    return client.loop();
}
