#ifndef NATS_CLIENT_H
#define NATS_CLIENT_H

#include "parser_state.hpp"
#include "subscription.hpp"
#include <string>
#include <string_view>
#include <atomic>
#include <thread>
#include <vector>
#include <utility>
#include <unordered_map>

namespace nats{

    class NatsServer; // forward declaration because of circular dependency between server.hpp and client.hpp

    class NatsClient {
        NatsServer* m_server;
        static constexpr int INTERNAL_BUFFER_SIZE = 1024*5;
        int m_client_fd;
        std::thread m_timeout_thread;
        std::atomic<bool> m_timeout_thread_running;
        std::unordered_map<int, std::string> m_subscriptions; //mapping between sub_id and subject topic
        void addSubscriptionMetadata(int sub_id, std::string subject);
        std::vector<std::pair<std::vector<std::string>,NatsSubscription>> getUnsubParams(bool filter_sub_id=false, int sub_id=-1);
        std::vector<std::string> convertSubjectToList(std::string_view& subject, bool is_publish);
    public:
        NatsClient(int client_fd, NatsServer* server);
        virtual ~NatsClient();
        bool m_waiting_for_initial_connect;
        bool m_waiting_for_initial_pong;
        long long m_client_id;
        int m_as;
        int m_drop;
        int m_arg_len;
        int m_msg_len;
        NatsParserState m_state;
        char m_msg_buffer[INTERNAL_BUFFER_SIZE];
        char m_arg_buffer[INTERNAL_BUFFER_SIZE];
        int m_payload_size;
        char m_payload_sub[INTERNAL_BUFFER_SIZE];
        virtual void resetParsingVars();

        virtual bool maxArgSizeReached();
        virtual bool maxMessageSizeReached();
        virtual void verifyState();
        virtual void closeConnection();
        virtual void closeConnection(std::string msg);
        virtual void sendMessage(std::string msg);
        virtual void sendErrorMessage(std::string msg);
        virtual void processConnect();
        virtual void processPing();
        virtual void processPong();
        virtual void processPubArgs(std::string_view& pub_args);
        virtual void processPub(std::string_view& payload);
        virtual void processSub(std::string_view& sub_args);
        virtual void processUnsub(std::string_view& unsub_args);
        virtual void startPongTimeoutThread();
        virtual void stopTimeoutThread();
    };
}

#endif