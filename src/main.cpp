#include "move_tables.h"
#include "client.h"
#include "tt/zobrist.h"
#include "evaluate.h"

int main() {
    init_zobrist();
    init_move_tables();
    init_evaluation_parameters();

    std::string match_id;
    std::cout << "Match ID: ";
    getline(std::cin, match_id);

    Client client(match_id);
    return client.loop();
}
