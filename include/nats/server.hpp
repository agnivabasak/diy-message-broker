#ifndef NATS_SERVER_H
#define NATS_SERVER_H

#include "client.hpp"
#include "sublist.hpp"
#include <unordered_map>
#include <memory>
#include <mutex>
#include <vector>
#include <utility>

namespace nats {

    class NatsServer {
    public:
        long long int m_server_id;
        int m_server_fd;
        std::unordered_map<long long, std::unique_ptr<NatsClient>> m_clients;
        std::mutex m_clients_mutex; //mutex to make sure multiple threads dont change m_clients at the same time
        std::unique_ptr<NatsSublist> m_sublist;

        NatsServer();

        void startServer();
        void addClient(std::unique_ptr<NatsClient> client);
        void removeClient(long long client_id);
        NatsClient* getClient(long long client_id);

        void addSubscription(int sub_id, std::vector<std::string>& subject_list, long long client_id);
        void removeSubscriptions(std::vector<std::pair<std::vector<std::string>,NatsSubscription>> unsub_params);
    };
}

#endif