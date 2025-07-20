#include "../include/nats/client.hpp"
#include "../include/nats/server.hpp"
#include "../include/nats/parser.hpp"
#include "../include/nats/subscription.hpp"
#include "../include/nats/sublist.hpp"

#include <random>
#include <cerrno>
#include <climits>
#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <vector>
#include <utility>
#include <unordered_map>
#include <mutex>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netdb.h>

using namespace std;

namespace nats {

    constexpr int PORT = 4222;  // Telnet-like port
    constexpr int BUFFER_SIZE = 1024;

    NatsServer::NatsServer() {
        m_running = false;
        random_device rd;
        mt19937 gen(rd());
        uniform_int_distribution<long long> dis(1, LLONG_MAX);
        
        m_server_id = dis(gen);
        m_sublist = std::make_unique<NatsSublist>();
    }

    NatsServer::~NatsServer(){
        if(m_running) stopServer();
    }

    void NatsServer::startServer(){
        int server_fd, client_fd;
        struct sockaddr_in server_addr {}, client_addr{};
        socklen_t client_len = sizeof(client_addr);

        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd == -1) {
            perror("socket failed");
            return;
        }

        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(PORT);
        server_addr.sin_addr.s_addr = INADDR_ANY;

        if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            perror("bind failed");
            close(server_fd);
            return;
        }

        if (listen(server_fd, 5) < 0) {
            perror("listen failed");
            close(server_fd);
            return;
        }

        m_server_fd = server_fd;

        cout << "Telnet-style server listening on port " << PORT << "...\n";
        m_running = true;

        // Vector to keep track of client threads
        std::vector<std::thread> client_threads;

        while (m_running) {
            struct sockaddr_in client_addr {};
            int client_fd = accept(m_server_fd, (struct sockaddr*)&client_addr, &client_len);
            if (client_fd < 0) {
                if (!m_running || errno == EBADF || errno == EINVAL) break;
                perror("accept failed");
                continue; // Don't exit the server, just skip this iteration
            }

            // Lambda to handle each client in a separate thread
            client_threads.emplace_back([this, client_fd, client_addr]() mutable {
                cout << "Client connected: " << inet_ntoa(client_addr.sin_addr) << "\n";
                std::unique_ptr<NatsClient> client_unique_ptr = std::make_unique<NatsClient>(client_fd, this);
                NatsClient* client = client_unique_ptr.get();
                addClient(std::move(client_unique_ptr));

                string initResponse = "INFO {\"server_id\":"+ std::to_string(m_server_id) + ",\"server_name\":\"nats-message-broker\",\"version\":\"1.0.0\",\"client_id\":" + std::to_string(client->m_client_id) + ",\"client_ip\":\"" + std::string(inet_ntoa(client_addr.sin_addr)) + "\",\"host_ip\":\"0.0.0.0\",\"host_port\":" + std::to_string(PORT) + "}\r\n";
                send(client_fd, initResponse.c_str(), initResponse.size(), 0);

                char buffer[BUFFER_SIZE];
                while (true) {
                    memset(buffer, 0, BUFFER_SIZE);
                    ssize_t bytes_received = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
                    if (bytes_received <= 0) {
                        cout << "Connection closed or error.\n";
                        break;
                    }
                    nats::NatsParser::parse(client, buffer, bytes_received);
                }

                // Close connections
                removeClient(client->m_client_id);
                close(client_fd);
                cout << "Client disconnected: " << inet_ntoa(client_addr.sin_addr) << "\n";
            });
        }

        for (auto& t : client_threads) {
            if (t.joinable()) t.join();
        }
        stopServer();
    }

    void NatsServer::stopServer() { 
        m_running = false; 
        close(m_server_fd);
    }

    void NatsServer::addClient(std::unique_ptr<NatsClient>client) {
        std::lock_guard<std::mutex> lock(m_clients_mutex);
        m_clients[client->m_client_id] = std::move(client);
    }

    void NatsServer::removeClient(long long client_id) {
        std::lock_guard<std::mutex> lock(m_clients_mutex);
        auto it = m_clients.find(client_id);
        if (it != m_clients.end()) {
            m_clients.erase(it);
        }
    }

    NatsClient* NatsServer::getClient(long long client_id) {
        std::lock_guard<std::mutex> lock(m_clients_mutex);
        auto it = m_clients.find(client_id);
        return (it != m_clients.end()) ? it->second.get() : nullptr;
    }

    void NatsServer::addSubscription(int sub_id, std::vector<std::string>& subject_list, long long client_id){
        //for subscription the server just passes on the request to the Sublist Class
        m_sublist->addSubscription({sub_id,client_id},subject_list);
    }

    void NatsServer::removeSubscriptions(vector<pair<vector<std::string>,NatsSubscription>> unsub_params){
        for(auto& pair: unsub_params){
            m_sublist->removeSubscription(pair.second,pair.first);
        }
    }

    void NatsServer::publishMessage(std::string& subject, std::vector<std::string>& subject_list, std::string msg){
        //first we get list of Subscriptions to the particular topic
        std::vector<NatsSubscription>subscriptions = m_sublist->getSubscriptionsForTopic(subject_list);
        for(NatsSubscription& subscription: subscriptions){
            NatsClient* client = getClient(subscription.m_client_id);
            if(client !=nullptr){
                client->sendMessage(
                    "MSG "+subject+" "+std::to_string(subscription.m_sub_id)+" "+std::to_string(msg.length())+"\r\n"
                    + msg +"\r\n"
                );
            }
        }
    }

}