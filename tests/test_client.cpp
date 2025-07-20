#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <utility>
#include <vector>
#include "./include/nats/test_mocks.hpp"
#include "../include/nats/client.hpp"
#include "../include/nats/server.hpp"
#include "../include/nats/parser_state.hpp"
#include "../include/nats/custom_specific_exceptions.hpp"

using namespace nats;
using ::testing::_;

//Fixture class for setting up and tearing down common objects for multiple related tests
class NatsClientTest : public ::testing::Test {
protected:
    MockNatsServer server;
    PartialMockNatsClient* client;
    int fake_fd = 1; // Use a dummy fd

    void SetUp() override {
        //adding this as base  destructor is always called so removeSubscription will be called in every test case
        EXPECT_CALL(server, removeSubscriptions(_)).Times(::testing::AnyNumber());
        client = new PartialMockNatsClient(fake_fd, &server);
        //except for initial connect and pong cases, this is what's required for all test cases
        client->m_waiting_for_initial_connect = false;
        client->m_waiting_for_initial_pong = false;
    }

    void TearDown() override {
        delete client;
    }
};

//TEST_F - is a Test Fixture which uses fixture class (in this case NatsClientTest)

//Tests initial connect, ping and pong processing along with verifyState function

TEST_F(NatsClientTest, ProcessConnect_Success) {
    client->m_waiting_for_initial_connect = true;
    client->m_state = NatsParserState::CONNECT_ARG;
    
    EXPECT_CALL(*client, startPongTimeoutThread()).Times(1);
    client->processConnect();
    EXPECT_FALSE(client->m_waiting_for_initial_connect);
    EXPECT_TRUE(client->m_waiting_for_initial_pong);
}

TEST_F(NatsClientTest, ProcessPong_Success) {
    client->m_waiting_for_initial_connect = false;
    client->m_waiting_for_initial_pong = true;
    client->m_state = NatsParserState::OP_PONG;
    client->processPong();
    EXPECT_FALSE(client->m_waiting_for_initial_pong);
}

TEST_F(NatsClientTest, verifyState_Failure_IncorrectState_1) {
    client->m_waiting_for_initial_connect = true;
    client->m_waiting_for_initial_pong = false;
    client->m_state = NatsParserState::OP_PONG;
    EXPECT_THROW(client->processPong(), ConnectionOperationExpectedException);
}

TEST_F(NatsClientTest, verifyState_Failure_IncorrectState_2) {
    client->m_waiting_for_initial_connect = false;
    client->m_waiting_for_initial_pong = true;
    client->m_state = NatsParserState::OP_CONNECT;
    EXPECT_THROW(client->processConnect(), PongOperationExpectedException);
}

TEST_F(NatsClientTest, ProcessPing_Success) {
    client->processPing();
    //Nothing much to test, enough that it doesn't throw any exceptions from verifyState
}

TEST_F(NatsClientTest, ProcessPubArgs_Success_Valid) {
    std::string pub_args = "foo.bar 10";
    std::string_view args_view(pub_args);
    client->processPubArgs(args_view);
    EXPECT_EQ(client->m_payload_size, 10);
    EXPECT_STREQ(client->m_payload_sub, "foo.bar");
}

TEST_F(NatsClientTest, ProcessPubArgs_Failure_Invalid_1) {
    std::string pub_args = "foo.bar";
    std::string_view args_view(pub_args);
    EXPECT_THROW(client->processPubArgs(args_view), ArgumentParseException);
}

TEST_F(NatsClientTest, ProcessPubArgs_Failure_Invalid_2) {
    std::string pub_args = "foo.bar   ";
    std::string_view args_view(pub_args);
    EXPECT_THROW(client->processPubArgs(args_view), ArgumentParseException);
}

TEST_F(NatsClientTest, ProcessPubArgs_Failure_Invalid_3_subIdNotInt) {
    std::string pub_args = "foo.bar testSubId";
    std::string_view args_view(pub_args);
    EXPECT_THROW(client->processPubArgs(args_view), ArgumentParseException);
}

TEST_F(NatsClientTest, ProcessPubArgs_Failure_Invalid_4_subIdNotInt) {
    std::string pub_args = "foo.bar 123test";
    std::string_view args_view(pub_args);
    EXPECT_THROW(client->processPubArgs(args_view), ArgumentParseException);
}

TEST_F(NatsClientTest, ProcessPub_Success) {
    // Set up a valid subject and payload
    strcpy(client->m_payload_sub, "foo.bar");
    std::string payload = "hello world";
    std::string_view payload_view(payload);

    // Expect publishMessage to be called with correct arguments
    EXPECT_CALL(server,
        publishMessage(
            testing::StrEq("foo.bar"),
            testing::ElementsAre("foo", "bar"),
            testing::StrEq(payload)
        )
    ).Times(1);

    // No exception should be thrown
    EXPECT_NO_THROW(client->processPub(payload_view));
}

TEST_F(NatsClientTest, ProcessPub_Falure_Invalid_1) {
    // Set up a valid subject and payload
    strcpy(client->m_payload_sub, "foo.*.bar.>");
    std::string payload = "hello world";
    std::string_view payload_view(payload);

    EXPECT_THROW(client->processPub(payload_view), InvalidPublishSubjectException);
}

TEST_F(NatsClientTest, ProcessPub_Falure_Invalid_2) {
    // Set up a valid subject and payload
    strcpy(client->m_payload_sub, "foo.>");
    std::string payload = "hello world";
    std::string_view payload_view(payload);

    EXPECT_THROW(client->processPub(payload_view), InvalidPublishSubjectException);
}

TEST_F(NatsClientTest, ProcessSub_Success) {
    // Valid subject and sub_id
    std::string sub_args = "foo.bar.*.test.> 42";
    std::string_view args_view(sub_args);

    // Expect addSubscription to be called with correct arguments
    EXPECT_CALL(server, addSubscription(42, testing::ElementsAre("foo", "bar","*","test",">"), testing::_)).Times(1);

    EXPECT_NO_THROW(client->processSub(args_view));
}

TEST_F(NatsClientTest, ProcessSub_Failure_Invalid_1_MissingSubId) {
    // Missing sub_id
    std::string sub_args = "foo.bar";
    std::string_view args_view(sub_args);

    EXPECT_THROW(client->processSub(args_view), ArgumentParseException);
}

TEST_F(NatsClientTest, ProcessSub_Failure_Invalid_2_NonIntSubId) {
    // sub_id is not an integer
    std::string sub_args = "foo.bar notAnInt";
    std::string_view args_view(sub_args);

    EXPECT_THROW(client->processSub(args_view), ArgumentParseException);
}

TEST_F(NatsClientTest, ProcessSub_Failure_Invalid_3_ExistingSubId) {
    // sub_id already exists
    std::string sub_args = "foo.baz 42";
    std::string_view args_view(sub_args);
    EXPECT_CALL(server, addSubscription(42, testing::ElementsAre("foo", "baz"), testing::_)).Times(1);

    client->processSub(args_view);

    std::string sub_args_new = "foo.bar.new 42";
    std::string_view args_view_new(sub_args_new);

    EXPECT_THROW(client->processSub(args_view_new), ExistingSubscriptionIdException);
}

TEST_F(NatsClientTest, ProcessSub_Failure_Invalid_4) {
    // sub_id is not an integer
    std::string sub_args = "foo.bar   ";
    std::string_view args_view(sub_args);

    EXPECT_THROW(client->processSub(args_view), ArgumentParseException);
}

TEST_F(NatsClientTest, ProcessSub_Failure_Invalid_5) {
    // sub_id is not an integer
    std::string sub_args = "foo..bar 5";
    std::string_view args_view(sub_args);

    EXPECT_THROW(client->processSub(args_view), InvalidSubscribeSubjectException);
}

TEST_F(NatsClientTest, ProcessUnsub_Success) {
    // Add a subscription first
    std::string sub_args = "foo.baz 42";
    std::string_view args_view(sub_args);

    //2 times because at the end we again add the same subscription
    EXPECT_CALL(server, addSubscription(42, testing::ElementsAre("foo", "baz"), testing::_)).Times(2);
    client->processSub(args_view);

    std::string unsub_args = "42";
    std::string_view unsub_args_view(unsub_args);

    // Expect removeSubscriptions to be called
    //2 times because the destructor will call it once automatically apart from the one executed because of processUnsub
    EXPECT_CALL(server, removeSubscriptions(testing::_)).Times(2);
    EXPECT_NO_THROW(client->processUnsub(unsub_args_view));
    
    //SUB again to make sure it doesn't throw exception for existing sub id
    EXPECT_NO_THROW(client->processSub(args_view));
}

TEST_F(NatsClientTest, ProcessUnsub_Failure_Invalid_1_MissingSubId) {
    // No sub_id provided
    std::string unsub_args = "";
    std::string_view args_view(unsub_args);

    EXPECT_THROW(client->processUnsub(args_view), ArgumentParseException);
}

TEST_F(NatsClientTest, ProcessUnsub_Failure_Invalid_2_NonIntSubId) {
    // sub_id is not an integer
    std::string unsub_args = "notAnInt";
    std::string_view args_view(unsub_args);

    EXPECT_THROW(client->processUnsub(args_view), ArgumentParseException);
}

TEST_F(NatsClientTest, ProcessUnsub_Failure_Invalid_3_NoSuchSubId) {
    // sub_id does not exist
    std::string unsub_args = "99";
    std::string_view args_view(unsub_args);

    EXPECT_THROW(client->processUnsub(args_view), NoSuchSubscriptionIdException);
}