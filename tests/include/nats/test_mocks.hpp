#ifndef NATS_TEST_MOCKS_H
#define NATS_TEST_MOCKS_H

#include "../../../include/nats/client.hpp"
#include "../../../include/nats/server.hpp"

namespace nats{
    //Mock NatsServer for testing
    class MockNatsServer : public NatsServer {
    public:
        MOCK_METHOD(void, publishMessage, (std::string&, std::vector<std::string>& , std::string), (override));
        MOCK_METHOD(void, addSubscription, (int, std::vector<std::string>&, long long), (override));
        MOCK_METHOD(void, removeSubscriptions, ((std::vector<std::pair<std::vector<std::string>, NatsSubscription>>)), (override));
    };

    // Mock NatsClient for testing
    class MockNatsClient : public NatsClient {
    public:
        MockNatsClient(MockNatsServer* server = nullptr) : NatsClient(0, server) {}
        MockNatsClient(int client_fd, NatsServer* server): 
                    NatsClient(client_fd,server){}
        ~MockNatsClient() override = default; 

        MOCK_METHOD(void, processConnect, (), (override));
        MOCK_METHOD(void, processPing, (), (override));
        MOCK_METHOD(void, processPong, (), (override));
        MOCK_METHOD(void, processPubArgs, (std::string_view&), (override));
        MOCK_METHOD(void, processPub, (std::string_view&), (override));
        MOCK_METHOD(void, processSub, (std::string_view&), (override));
        MOCK_METHOD(void, processUnsub, (std::string_view&), (override));
        MOCK_METHOD(void, closeConnection, (const std::string), (override));
        MOCK_METHOD(void, sendErrorMessage, (const std::string), (override));
        MOCK_METHOD(void, resetParsingVars, (), (override));
        MOCK_METHOD(bool, maxArgSizeReached, (), (override));
        MOCK_METHOD(bool, maxMessageSizeReached, (), (override));
        MOCK_METHOD(void, startPongTimeoutThread, (), (override)); 
    };

    // Partial Mock of NatsClient

    class PartialMockNatsClient : public NatsClient {
        public:
            PartialMockNatsClient(int client_fd, NatsServer* server): 
                NatsClient(client_fd,server){}
            ~PartialMockNatsClient() override = default; 
            MOCK_METHOD(void, startPongTimeoutThread, (), (override)); 
    };
}

#endif