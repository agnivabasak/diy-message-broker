#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fstream>
#include "../include/nats/server.hpp"

using namespace nats;

constexpr int TEST_PORT = 4222; // Should match your server's port

// Helper to connect to the server and return the socket fd
int connect_to_server() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    EXPECT_NE(sock, -1) << "Failed to create socket";

    sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(TEST_PORT);
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Wait for server to be ready
    for (int i = 0; i < 10; ++i) {
        if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == 0)
            return sock;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    ADD_FAILURE() << "Failed to connect to server";
    return -1;
}

// Helper to send a command and read the response
std::string send_and_recv(int sock, const std::string& cmd, int expected_bytes = 1024) {
    send(sock, cmd.c_str(), cmd.size(), 0);
    char buffer[2048] = {0};
    int n = recv(sock, buffer, expected_bytes, 0);
    if (n > 0) return std::string(buffer, n);
    return "";
}

TEST(ServerIntegration, BasicNatsFlow) {
    // Redirect cout and cerr for the server
    std::streambuf* orig_cout = std::cout.rdbuf();
    std::ofstream null_stream("/dev/null");
    std::cout.rdbuf(null_stream.rdbuf());

    NatsServer server;
    std::thread server_thread([&server]() {
        server.startServer();
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    GTEST_LOG_(INFO) << "Attempting to connect to server..." ;

    int sock = connect_to_server();
    GTEST_LOG_(INFO) << "Connected to server, sock=" << sock ;

    // 1. Read the INFO line
    char buffer[2048] = {0};
    int n = recv(sock, buffer, sizeof(buffer), 0);
    GTEST_LOG_(INFO) << "INFO line received, n=" << n ;
    ASSERT_GT(n, 0);
    std::string info_line(buffer, n);
    GTEST_LOG_(INFO) << "INFO line: " << info_line ;
    EXPECT_NE(info_line.find("INFO"), std::string::npos);

    // 2. Send CONNECT
    std::string connect_cmd = "CONNECT {\"verbose\":false}\r\n";
    GTEST_LOG_(INFO) << "Sending CONNECT..." ;
    std::string resp = send_and_recv(sock, connect_cmd);
    GTEST_LOG_(INFO) << "CONNECT response: " << resp ;
    EXPECT_NE(resp.find("+OK"), std::string::npos);
    EXPECT_NE(resp.find("PING"), std::string::npos);

    // 3. Send PONG (to reply to server's PING)
    GTEST_LOG_(INFO) << "Sending PONG..." ;
    ssize_t sent = send(sock, "PONG\r\n", 6, 0);
    GTEST_LOG_(INFO) << "Sent PONG, bytes sent: " << sent;

    // 4. Send PING
    GTEST_LOG_(INFO) << "Sending PING..." ;
    resp = send_and_recv(sock, "PING\r\n");
    GTEST_LOG_(INFO) << "PING response: " << resp ;
    EXPECT_NE(resp.find("PONG"), std::string::npos);

    // 5. Subscribe to a subject
    GTEST_LOG_(INFO) << "Sending SUB..." ;
    resp = send_and_recv(sock, "SUB foo.bar 1\r\n");
    GTEST_LOG_(INFO) << "SUB response: " << resp ;
    EXPECT_NE(resp.find("+OK"), std::string::npos);

    // 6. Publish a message
    GTEST_LOG_(INFO) << "Sending PUB..." ;
    resp = send_and_recv(sock, "PUB foo.bar 5\r\nhello\r\n");
    GTEST_LOG_(INFO) << "PUB response: " << resp ;
    EXPECT_NE(resp.find("+OK"), std::string::npos);

    // Check if the published message is also in this response
    size_t msg_pos = resp.find("MSG foo.bar 1 5");
    std::string published_msg;
    if (msg_pos != std::string::npos) {
        // The published message is in the same buffer
        published_msg = resp.substr(msg_pos);
        GTEST_LOG_(INFO) << "Published message (from PUB response): " << published_msg;
    } else {
        // 7. Receive the published message (if not already received)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        GTEST_LOG_(INFO) << "Waiting for published message..." ;
        n = recv(sock, buffer, sizeof(buffer), 0);
        GTEST_LOG_(INFO) << "Published message received, n=" << n ;
        published_msg = std::string(buffer, n);
        GTEST_LOG_(INFO) << "Published message: " << published_msg ;
    }

    // Now check the published message
    EXPECT_NE(published_msg.find("MSG foo.bar 1 5"), std::string::npos);
    EXPECT_NE(published_msg.find("hello"), std::string::npos);

    // 8. Unsubscribe
    GTEST_LOG_(INFO) << "Sending UNSUB..." ;
    resp = send_and_recv(sock, "UNSUB 1\r\n");
    GTEST_LOG_(INFO) << "UNSUB response: " << resp ;
    EXPECT_NE(resp.find("+OK"), std::string::npos);

    // Cleanup
    GTEST_LOG_(INFO) << "Closing socket and stopping server..." ;
    close(sock);
    server.stopServer();

    // Unblock accept() by connecting to the server port - sometimes in linux accept() doesn't unblock automatically after closing a listening socket from another thread
    int dummy_sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(TEST_PORT);
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    connect(dummy_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    close(dummy_sock);

    server_thread.join();

    // Restore cout and cerr
    std::cout.rdbuf(orig_cout);
    GTEST_LOG_(INFO) << "Test complete." ;
}