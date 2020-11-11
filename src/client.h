#ifndef ONITAMA_CLIENT_H
#define ONITAMA_CLIENT_H

#include <string>
#include <iostream>

using std::string;

class Client {
 private:
    void handle_message(string message);

 public:
    int loop(string match_id);
};

#endif  // ONITAMA_CLIENT_H
