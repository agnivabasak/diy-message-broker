#include "../include/nats/server.hpp"
#include <iostream>

constexpr int PORT = 4222;  // Telnet-like port
constexpr int BUFFER_SIZE = 1024;

using namespace std;

int main() {
    //disabling buffering
    std::cout.setf(std::ios::unitbuf);
    std::cerr.setf(std::ios::unitbuf);
    
    nats::NatsServer server;
    server.startServer();
    return 0;
}
