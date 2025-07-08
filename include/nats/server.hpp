#ifndef NATS_SERVER_H
#define NATS_SERVER_H

#include <unordered_map>
#include <memory>
#include <mutex>

namespace nats {

    class NatsClient;// forward declaration because of circular dependency between server.hpp and client.hpp

    class NatsServer {
    public:
        long long int m_server_id;
        int m_server_fd;
        std::unordered_map<long long, NatsClient*> m_clients;
        std::mutex m_clients_mutex; //mutex to make sure multiple threads dont change m_clients at the same time

        NatsServer();
        ~NatsServer();

        void startServer();
        void addClient(NatsClient* client);
        void removeClient(long long client_id);
        NatsClient* getClient(long long client_id);
    };
}

#endif