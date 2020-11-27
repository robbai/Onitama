#ifndef ONITAMA_CLIENT_H
#define ONITAMA_CLIENT_H

#include <memory>
#include <string>
#include <iostream>

#ifdef _WIN32
#pragma comment(lib, "ws2_32")
#include <WinSock2.h>
#include <rapidjson/pointer.h>
#endif

#include "rapidjson/document.h"
#include "easywsclient.hpp"
#include "types.h"
#include "board.h"

using easywsclient::WebSocket;
using std::string;

class Client {
 private:
    Board board;
    string match_id, token;
    Turn index;
    bool end_loop = false, generated_tb = false;
    void send(std::unique_ptr<WebSocket> const &ws, string message);
    void handle_json(std::unique_ptr<WebSocket> const &unique_ptr, string json);
    void receive_create(std::unique_ptr<WebSocket> const &unique_ptr,
                        rapidjson::Document &doc);
    void receive_join(std::unique_ptr<WebSocket> const &unique_ptr,
                      rapidjson::Document &doc);
    void receive_state(std::unique_ptr<WebSocket> const &ws, rapidjson::Document &doc);
    void receive_move(rapidjson::Document &doc);
    void receive_spectate(rapidjson::Document &doc);

 public:
    explicit Client(string match_id) {
        this->match_id = match_id;
    }
    int loop();
};

#endif  // ONITAMA_CLIENT_H
