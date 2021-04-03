#include "client.h"

#include <algorithm>
#include <string>
#include <sstream>

#include "easywsclient.hpp"
#include "util.h"
#include "search.h"
#include "tb/tb_probe.h"
#include "version.h"
#include "tt/zobrist.h"

using easywsclient::WebSocket;

const string SERVER_URL = "ws://litama.herokuapp.com";

const string USERNAME =
        "robbai" + (GIT_BRANCH[0] == 0 ? "" : "-" + std::string(GIT_BRANCH));

Turn parse_colour(const string colour) {
    return (Turn)(colour == "red");
}

Card parse_card(const string card_name) {
    for (int i = 0; i < CARD_NUM; ++i) {
        if (to_lower(CARD_NAMES[i]) == card_name)
            return (Card) i;
    }
    return CARD_NONE;
}

void Client::send(string message) {
    std::cout << "< " << message << std::endl;
    ws->send(message);
}

void Client::handle_json(string json) {
    // Parse JSON.
    rapidjson::Document doc;
    doc.Parse(json.c_str());

    const string type = doc["messageType"].GetString();
    std::cout << "> " << type << std::endl;
    if (type == "create")
        receive_create(doc);
    else if (type == "join")
        receive_join(doc);
    else if (type == "state")
        receive_state(doc);
    else if (type == "move")
        receive_move(doc);
    else if (type == "spectate")
        receive_spectate(doc);
    else
        std::cerr << "Unknown response: " << json << std::endl;
}

void Client::receive_create(rapidjson::Document &doc) {
    if (match_id.empty()) {
        match_id = doc["matchId"].GetString();
        std::cout << "Match ID: " << match_id << std::endl;
        send("spectate " + match_id);
    }
    index = (Turn) doc["index"].GetInt();
    token = doc["token"].GetString();
}

void Client::receive_join(rapidjson::Document &doc) {
    return receive_create(doc);
}

void Client::receive_state(rapidjson::Document &doc) {
    // Skip parsing if not in progress.
    if (std::strcmp(doc["gameState"].GetString(), "in progress")) {
        // Simply exit if the game is over.
        if (!std::strcmp(doc["gameState"].GetString(), "ended"))
            end_loop = true;
        return;
    }

    Board new_board = board.copy();

    // Parse turn.
    new_board.turn = parse_colour(doc["currentTurn"].GetString());

    // Parse cards.
    for (int i = 0; i < PLAYERS_NUM; ++i) {
        const auto &card_array = doc["cards"][i ? "red" : "blue"].GetArray();
        for (int j = 0; j < CARDS_EACH_NUM; ++j)
            new_board.cards[i][j] = parse_card(card_array[j].GetString());
    }
    new_board.side_card = parse_card(doc["cards"]["side"].GetString());

    // Parse squares.
    auto squares = doc["board"].GetString();
    new_board.pieces[WHITE][STUDENT] = 0;
    new_board.pieces[WHITE][MASTER] = 0;
    new_board.pieces[BLACK][STUDENT] = 0;
    new_board.pieces[BLACK][MASTER] = 0;
    for (int square = 0; square < SQUARE_NUM; ++square) {
        char character = squares[square];
        if (character == '0')
            continue;
        else if (character == '1')
            new_board.pieces[WHITE][STUDENT] |= 1u << square;
        else if (character == '2')
            new_board.pieces[WHITE][MASTER] |= 1u << square;
        else if (character == '3')
            new_board.pieces[BLACK][STUDENT] |= 1u << square;
        else if (character == '4')
            new_board.pieces[BLACK][MASTER] |= 1u << square;
    }

    new_board.move_count = doc["moves"].GetArray().Size();

    set_hash(&new_board);

    // Setup and generate tablebase.
    if (!GENERATED_TB)
        setup_and_generate_tb(&new_board);

    std::cout << std::endl << pretty_board(&new_board) << std::endl;

    // Calculate and send a move back.
    if (new_board.turn == (doc["indices"]["red"].GetInt() == index ? BLACK : WHITE) &&
        !(new_board == board)) {
        Move move = start_search(&new_board, 1, false, 1);

        // Translate move and send.
        string move_message = move_string(&new_board, move);
        move_message = to_lower(move_message);
        for (int i = 0; i < move_message.length(); ++i) {
            if (move_message[i] == ':') {
                move_message[i] = ' ';
                break;
            }
        }
        send("move " + match_id + " " + token + " " + move_message);
    }

    board = new_board;

    std::cout << std::endl;
}

void Client::receive_move(rapidjson::Document &doc) {
    return;
}

void Client::receive_spectate(rapidjson::Document &doc) {
    return;
}

int Client::loop() {
    // Open connection.
#ifdef _WIN32
    INT rc;
    WSADATA wsaData;
    rc = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (rc) {
        printf("WSAStartup Failed.\n");
        return 1;
    }
#endif
    ws = WebSocket::from_url(SERVER_URL);

    // Main loop.
    if (match_id.empty()) {
        send("create " + USERNAME);
    } else {
        send("join " + match_id + " " + USERNAME);
        send("spectate " + match_id);
    }
    while (ws->getReadyState() != WebSocket::CLOSED && !end_loop) {
        ws->poll(-1);
        ws->dispatch([&](const std::string &json) {
            handle_json(json);
        });
    }

    // Close connection.
    ws->close();
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
