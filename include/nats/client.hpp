#ifndef NATS_CLIENT_H
#define NATS_CLIENT_H

#include "parser_state.hpp"
#include <string>
#include <atomic>
#include <thread>

namespace nats{
    class NatsClient {
        static constexpr int INTERNAL_BUFFER_SIZE = 1024*5;
        int m_client_fd;
        std::thread m_timeout_thread;
        std::atomic<bool> m_timeout_thread_running;
    public:
        NatsClient(int client_fd);
        ~NatsClient();
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
        void resetParsingVars();
        bool maxArgSizeReached();
        void verifyState();
        void closeConnection();
        void closeConnection(std::string msg);
        void processConnect();
        void processPing();
        void processPong();
        void startPongTimeoutThread();
        void stopTimeoutThread();
    };
}

#endif