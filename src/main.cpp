#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netdb.h>
#include "../include/nats/client.hpp"
#include "../include/nats/parser.hpp"

constexpr int PORT = 4222;  // Telnet-like port
constexpr int BUFFER_SIZE = 1024;

using namespace std;

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr {}, client_addr{};
    socklen_t client_len = sizeof(client_addr);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket failed");
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 5) < 0) {
        perror("listen failed");
        close(server_fd);
        return 1;
    }

    cout << "Telnet-style server listening on port " << PORT << "...\n";

    client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
    if (client_fd < 0) {
        perror("accept failed");
        close(server_fd);
        return 1;
    }

    cout << "Client connected: " << inet_ntoa(client_addr.sin_addr) << "\n";
    nats::NatsClient client(client_fd);
    string initResponse = "INFO {\"server_id\":\"randomNumberForNow\",\"server_name\":\"nats-message-broker\",\"version\":\"1.0.0\",\"client_id\":" + std::to_string(client.m_client_id) + ",\"client_ip\":\"" + std::string(inet_ntoa(client_addr.sin_addr)) + "\",\"host_ip\":\"0.0.0.0\",\"host_port\":" + std::to_string(PORT) + "}\r\n";
    send(client_fd, initResponse.c_str(), initResponse.size(), 0);

    char buffer[BUFFER_SIZE];
    while (true) {
        memset(buffer, 0, BUFFER_SIZE); //setting the whole buffer to 0x00, which is the null character
        ssize_t bytes_received = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_received <= 0) {
            cout << "Connection closed or error.\n";
            break;
        }
        nats::NatsParser::parse(&client,buffer,bytes_received);
    }

    // 6. Close connections
    close(client_fd);
    close(server_fd);
    return 0;
}
