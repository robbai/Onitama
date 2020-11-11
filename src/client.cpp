#include "client.h"

#include <string>
#include <memory>
#include <sstream>

#ifdef _WIN32
#pragma comment(lib, "ws2_32")
#include <WinSock2.h>
#endif

#include "easywsclient.hpp"

const string SERVER_URL = "ws://litama.herokuapp.com";


void Client::handle_message(string message) {
    std::cout << message << std::endl;
}

int Client::loop(string match_id) {
    // Open connection.
    using easywsclient::WebSocket;
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
    ws->send("join " + match_id);
    while (ws->getReadyState() != WebSocket::CLOSED) {
        ws->poll(-1);
        ws->dispatch([&](const std::string &message) {
            handle_message(message);
        });
    }

    // Close connection.
    ws->close();
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
