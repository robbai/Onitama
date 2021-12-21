#include "move_tables.h"
#include "client.h"
#include "tt/zobrist.h"
#include "evaluate.h"
#include "runner/runner.h"
#include "search.h"

int main(int argc, char *argv[]) {
    init_zobrist();
    init_move_tables();
    init_search();

    if (argc > 1) {
        return use_runner();
    }

    std::string match_id;
    std::cout << "Match ID: ";
    getline(std::cin, match_id);

    Client client(match_id);
    return client.loop();
}
