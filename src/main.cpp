#include "../include/nats/server.hpp"

constexpr int PORT = 4222;  // Telnet-like port
constexpr int BUFFER_SIZE = 1024;

using namespace std;

int main() {
    nats::NatsServer server;
    server.startServer();
    return 0;
}
