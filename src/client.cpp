#include "client.h"

#include <algorithm>
#include <string>
#include <memory>
#include <sstream>

#include "easywsclient.hpp"
#include "util.h"
#include "search.h"

using easywsclient::WebSocket;

const string SERVER_URL = "ws://litama.herokuapp.com";

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

void Client::send(std::unique_ptr<WebSocket> const &ws, string message) {
    std::cout << "< " << message << std::endl;
    ws->send(message);
}

void Client::handle_json(std::unique_ptr<WebSocket> const &ws, string json) {
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
        receive_state(ws, doc);
    else if (type == "move")
        receive_move(doc);
    else if (type == "spectate")
        receive_spectate(doc);
    else
        std::cerr << "Unknown response: " << json << std::endl;
}

void Client::receive_create(rapidjson::Document &doc) {
    our_turn = parse_colour(doc["color"].GetString());
    token = doc["token"].GetString();
}

void Client::receive_join(rapidjson::Document &doc) {
    return receive_create(doc);
}

void Client::receive_state(std::unique_ptr<WebSocket> const &ws,
                           rapidjson::Document &doc) {
    // Parse turn.
    board.turn = parse_colour(doc["currentTurn"].GetString());

    // Parse cards.
    for (int i = 0; i < PLAYERS_NUM; ++i) {
        const auto &card_array = doc["cards"][!i ? "blue" : "red"].GetArray();
        for (int j = 0; j < CARDS_EACH_NUM; ++j)
            board.cards[i][j] = parse_card(card_array[j].GetString());
    }
    board.side_card = parse_card(doc["cards"]["side"].GetString());

    // Parse squares.
    auto squares = doc["board"].GetString();
    board.pieces[WHITE][STUDENT] = 0;
    board.pieces[WHITE][MASTER] = 0;
    board.pieces[BLACK][STUDENT] = 0;
    board.pieces[BLACK][MASTER] = 0;
    for (int square = 0; square < SQUARE_NUM; ++square) {
        char character = squares[square];
        if (character == '0')
            continue;
        else if (character == '1')
            board.pieces[WHITE][STUDENT] |= 1u << square;
        else if (character == '2')
            board.pieces[WHITE][MASTER] |= 1u << square;
        else if (character == '3')
            board.pieces[BLACK][STUDENT] |= 1u << square;
        else if (character == '4')
            board.pieces[BLACK][MASTER] |= 1u << square;
    }

    std::cout << std::endl << pretty_board(&board) << std::endl;

    // Calculate and send a move back.
    if (!std::strcmp(doc["gameState"].GetString(), "in progress") &&
        board.turn == our_turn) {
        Move move = search(&board);

        // Translate move and send.
        string move_message = move_string(&board, move);
        move_message = to_lower(move_message);
        for (int i = 0; i < move_message.length(); ++i) {
            if (move_message[i] == ':') {
                move_message[i] = ' ';
                break;
            }
        }
        send(ws, "move " + match_id + " " + token + " " + move_message);
    }

    // Simply abort if the game is over.
    if (std::strcmp(doc["winner"].GetString(), "none"))
        abort();

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
    std::unique_ptr<WebSocket> ws(WebSocket::from_url(SERVER_URL));

    // Main loop.
    send(ws, "join " + match_id);
    send(ws, "spectate " + match_id);
    while (ws->getReadyState() != WebSocket::CLOSED) {
        ws->poll(-1);
        ws->dispatch([&](const std::string &json) {
            handle_json(ws, json);
        });
    }

    // Close connection.
    ws->close();
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
