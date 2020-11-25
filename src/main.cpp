#include "move_tables.h"
#include "client.h"

int main() {
    init_move_tables();

    std::string match_id;
    std::cout << "Match ID:";
    std::cin >> match_id;

    Client client(match_id);
    return client.loop();
}
