#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../include/nats/parser.hpp"
#include "../include/nats/parser_state.hpp"
#include "../include/nats/custom_specific_exceptions.hpp"
#include "./include/nats/test_mocks.hpp"
#include <string>
#include <cstring>

using namespace nats;
using ::testing::_;

class ParserTest : public ::testing::Test {
    protected:
        MockNatsServer server;
        MockNatsClient* client;

        void SetUp() override {
            client = new MockNatsClient(&server);
            //adding this as base  destructor is always called so removeSubscription will be called in every test case
            EXPECT_CALL(server, removeSubscriptions(_)).Times(::testing::AnyNumber());
        }

        void TearDown() override {
            delete client;
        }
    };

//CONNECT 

TEST_F(ParserTest, Connect_Success) {
    EXPECT_CALL(*client, processConnect()).Times(1);
    EXPECT_CALL(*client, resetParsingVars()).Times(1);

    std::string connect = "CONNECT {\"foo\":\"bar\"}\r\n";
    NatsParser::parse(client, connect.data(), connect.size());
}

TEST_F(ParserTest, Connect_Success_OnlyNewLineAfterArgs) {
    EXPECT_CALL(*client, processConnect()).Times(1);
    EXPECT_CALL(*client, resetParsingVars()).Times(1);

    std::string connect = "CONNECT {\"foo\":\"bar\"}\n";
    NatsParser::parse(client, connect.data(), connect.size());
}

TEST_F(ParserTest, Connect_Failure_MalformedJson) {
    EXPECT_CALL(*client, closeConnection(_)).Times(1);

    std::string connect = "CONNECT {foo:bar}\r\n";
    NatsParser::parse(client, connect.data(), connect.size());
}

TEST_F(ParserTest, Connect_Success_MultiBuffer) {
    ON_CALL(*client, maxArgSizeReached()).WillByDefault(::testing::Return(false));

    EXPECT_CALL(*client, processConnect()).Times(1);
    EXPECT_CALL(*client, resetParsingVars()).Times(1);
    EXPECT_CALL(*client, maxArgSizeReached()).Times(::testing::AnyNumber());

    std::string part1 = "CONN";
    std::string part2 = "ECT {\"foo\":";
    std::string part3 = "\"bar\"}\r\n";
    NatsParser::parse(client, part1.data(), part1.size());
    NatsParser::parse(client, part2.data(), part2.size());
    NatsParser::parse(client, part3.data(), part3.size());
}

TEST_F(ParserTest, Connect_Failure_MultiBuffer_MaxArgSizeReached) {
    ON_CALL(*client, maxArgSizeReached()).WillByDefault(::testing::Return(true));

    EXPECT_CALL(*client, closeConnection(_)).Times(1);
    EXPECT_CALL(*client, maxArgSizeReached()).Times(::testing::AnyNumber());

    std::string part1 = "CONN";
    std::string part2 = "ECT {\"foo\":";
    std::string part3 = "\"bar\"}\r\n";
    NatsParser::parse(client, part1.data(), part1.size());
    NatsParser::parse(client, part2.data(), part2.size());
    NatsParser::parse(client, part3.data(), part3.size());
}

//PING

TEST_F(ParserTest, Ping_Success) {
    EXPECT_CALL(*client, processPing()).Times(1);
    EXPECT_CALL(*client, resetParsingVars()).Times(1);

    std::string ping = "PING\r\n";
    NatsParser::parse(client, ping.data(), ping.size());
}

//PONG

TEST_F(ParserTest, Pong_Success) {
    EXPECT_CALL(*client, processPong()).Times(1);
    EXPECT_CALL(*client, resetParsingVars()).Times(1);

    std::string pong = "PONG\r\n";
    NatsParser::parse(client, pong.data(), pong.size());
}

//PUB

TEST_F(ParserTest, Pub_Success) {
    //Since this is a unit test for the parser, 
    //the function of changing m_payload_size is mocked (which is performed in client in the actual flow)
    client->m_payload_size = 3; 

    std::string pub = "PUB foo 3\r\nabc\r\n";

    EXPECT_CALL(*client, processPubArgs(::testing::Eq(std::string_view("foo 3")))).Times(1);
    EXPECT_CALL(*client, processPub(::testing::Eq(std::string_view("abc")))).Times(1);
    EXPECT_CALL(*client, resetParsingVars()).Times(1);

    NatsParser::parse(client, pub.data(), pub.size());
}

TEST_F(ParserTest, Pub_Success_OnlyNewLineAfterArgs) {
    //Since this is a unit test for the parser, 
    //the function of changing m_payload_size is mocked (which is performed in client in the actual flow)
    client->m_payload_size = 3; 
    //should work even if after args only \n is there
    std::string pub = "PUB foo 3\nabc\r\n";

    EXPECT_CALL(*client, processPubArgs(::testing::Eq(std::string_view("foo 3")))).Times(1);
    EXPECT_CALL(*client, processPub(::testing::Eq(std::string_view("abc")))).Times(1);
    EXPECT_CALL(*client, resetParsingVars()).Times(1);

    NatsParser::parse(client, pub.data(), pub.size());
}

TEST_F(ParserTest, Pub_Failure_PayloadSizeMismatch) {
    //Since this is a unit test for the parser, 
    //the function of changing m_payload_size is mocked (which is performed in client in the actual flow)
    client->m_payload_size = 3; 
    //should work even if after args only \n is there
    std::string pub = "PUB foo 3\nhello\r\n";

    EXPECT_CALL(*client, processPubArgs(::testing::Eq(std::string_view("foo 3")))).Times(1);
    EXPECT_CALL(*client, closeConnection(_)).Times(1);

    NatsParser::parse(client, pub.data(), pub.size());
}


TEST_F(ParserTest, Pub_Success_MultiBuffer) {
    //Since this is a unit test for the parser, 
    //the function of changing m_payload_size is mocked (which is performed in client in the actual flow)
    client->m_payload_size = 12; 

    std::string part1 = "PUB foo.bar.";
    std::string part2 = "test 12\r";
    std::string part3 = "\nHello W";
    std::string part4 = "orld!\r\n";
    
    ON_CALL(*client, maxArgSizeReached()).WillByDefault(::testing::Return(false));
    ON_CALL(*client, maxMessageSizeReached()).WillByDefault(::testing::Return(false));

    EXPECT_CALL(*client, processPubArgs(::testing::Eq(std::string_view("foo.bar.test 12")))).Times(1);
    EXPECT_CALL(*client, processPub(::testing::Eq(std::string_view("Hello World!")))).Times(1);
    EXPECT_CALL(*client, resetParsingVars()).Times(1);
    EXPECT_CALL(*client, maxArgSizeReached()).Times(::testing::AnyNumber());
    EXPECT_CALL(*client, maxMessageSizeReached()).Times(::testing::AnyNumber());

    NatsParser::parse(client, part1.data(), part1.size());
    NatsParser::parse(client, part2.data(), part2.size());
    NatsParser::parse(client, part3.data(), part3.size());
    NatsParser::parse(client, part4.data(), part4.size());
}

TEST_F(ParserTest, Pub_Failure_MultiBuffer_MaxArgSizeReached) {

    std::string part1 = "PUB foo.bar.test 1";
    std::string part2 = "2\r\nHello World!\r\n";
    
    ON_CALL(*client, maxArgSizeReached()).WillByDefault(::testing::Return(true));
    EXPECT_CALL(*client, maxArgSizeReached()).Times(1);
    EXPECT_CALL(*client, closeConnection(_)).Times(1);

    NatsParser::parse(client, part1.data(), part1.size());
    NatsParser::parse(client, part2.data(), part2.size());
}

TEST_F(ParserTest, Pub_Failure_MultiBuffer_MaxMessageSizeReached) {
    //Since this is a unit test for the parser, 
    //the function of changing m_payload_size is mocked (which is performed in client in the actual flow)
    client->m_payload_size = 12; 

    std::string part1 = "PUB foo.bar.";
    std::string part2 = "test 12\r";
    std::string part3 = "\nHello W";
    std::string part4 = "orld!\r\n";
    
    ON_CALL(*client, maxArgSizeReached()).WillByDefault(::testing::Return(false));
    ON_CALL(*client, maxMessageSizeReached()).WillByDefault(::testing::Return(true));
    EXPECT_CALL(*client, processPubArgs(::testing::Eq(std::string_view("foo.bar.test 12")))).Times(1);
    EXPECT_CALL(*client, maxArgSizeReached()).Times(::testing::AnyNumber());
    EXPECT_CALL(*client, maxMessageSizeReached()).Times(::testing::AnyNumber());
    EXPECT_CALL(*client, closeConnection(_)).Times(1);

    NatsParser::parse(client, part1.data(), part1.size());
    NatsParser::parse(client, part2.data(), part2.size());
    NatsParser::parse(client, part3.data(), part3.size());
    NatsParser::parse(client, part4.data(), part4.size());
}

//SUB

TEST_F(ParserTest, Sub_Success) {

    std::string sub = "SUB foo.bar.> 10\r\n";

    EXPECT_CALL(*client, processSub(::testing::Eq(std::string_view("foo.bar.> 10")))).Times(1);
    EXPECT_CALL(*client, resetParsingVars()).Times(1);

    NatsParser::parse(client, sub.data(), sub.size());
}

TEST_F(ParserTest, Sub_Success_OnlyNewLineAfterArgs) {

    std::string sub = "SUB foo.bar.> 10\n";

    EXPECT_CALL(*client, processSub(::testing::Eq(std::string_view("foo.bar.> 10")))).Times(1);
    EXPECT_CALL(*client, resetParsingVars()).Times(1);

    NatsParser::parse(client, sub.data(), sub.size());
}

TEST_F(ParserTest, Sub_Success_MultiBuffer) {

    std::string part1 = "SUB foo.bar.";
    std::string part2 = "test.> 10\r\n";

    ON_CALL(*client, maxArgSizeReached()).WillByDefault(::testing::Return(false));

    EXPECT_CALL(*client, processSub(::testing::Eq(std::string_view("foo.bar.test.> 10")))).Times(1);
    EXPECT_CALL(*client, resetParsingVars()).Times(1);
    EXPECT_CALL(*client, maxArgSizeReached()).Times(::testing::AnyNumber());

    NatsParser::parse(client, part1.data(), part1.size());
    NatsParser::parse(client, part2.data(), part2.size());
}

TEST_F(ParserTest, Sub_Failure_MultiBuffer_MaxArgSizeReached) {

    std::string part1 = "SUB foo.bar.";
    std::string part2 = "test.> 10\r\n";
    
    ON_CALL(*client, maxArgSizeReached()).WillByDefault(::testing::Return(true));

    EXPECT_CALL(*client, maxArgSizeReached()).Times(1);
    EXPECT_CALL(*client, closeConnection(_)).Times(1);

    NatsParser::parse(client, part1.data(), part1.size());
    NatsParser::parse(client, part2.data(), part2.size());
}

//UNSUB

TEST_F(ParserTest, Unsub_Success) {

    std::string unsub = "UNSUB 100\r\n";

    EXPECT_CALL(*client, processUnsub(::testing::Eq(std::string_view("100")))).Times(1);
    EXPECT_CALL(*client, resetParsingVars()).Times(1);

    NatsParser::parse(client, unsub.data(), unsub.size());
}

TEST_F(ParserTest, Unsub_Success_OnlyNewLineAfterArgs) {

    std::string unsub = "UNSUB 100\n";

    EXPECT_CALL(*client, processUnsub(::testing::Eq(std::string_view("100")))).Times(1);
    EXPECT_CALL(*client, resetParsingVars()).Times(1);

    NatsParser::parse(client, unsub.data(), unsub.size());
}

TEST_F(ParserTest, Unsub_Success_MultiBuffer) {

    std::string part1 = "UNSUB 100";
    std::string part2 = "0090\r\n";

    ON_CALL(*client, maxArgSizeReached()).WillByDefault(::testing::Return(false));

    EXPECT_CALL(*client, processUnsub(::testing::Eq(std::string_view("1000090")))).Times(1);
    EXPECT_CALL(*client, resetParsingVars()).Times(1);
    EXPECT_CALL(*client, maxArgSizeReached()).Times(::testing::AnyNumber());

    NatsParser::parse(client, part1.data(), part1.size());
    NatsParser::parse(client, part2.data(), part2.size());
}

TEST_F(ParserTest, Unsub_Failure_MultiBuffer_MaxArgSizeReached) {

    std::string part1 = "UNSUB 100";
    std::string part2 = "0090\r\n";
    
    ON_CALL(*client, maxArgSizeReached()).WillByDefault(::testing::Return(true));
    
    EXPECT_CALL(*client, maxArgSizeReached()).Times(1);
    EXPECT_CALL(*client, closeConnection(_)).Times(1);

    NatsParser::parse(client, part1.data(), part1.size());
    NatsParser::parse(client, part2.data(), part2.size());
}

//Generic Parser Failures

//This is just to test that the client can handle Non fatal exceptions since all non fatal exceptions are thrown by the client functions which are being mocked
TEST_F(ParserTest, Parser_Failure_NatsNonFatalException) {

    std::string sub = "SUB foo.bar.> 10\n";

    // Set up processSub to throw ExistingSubscriptionIdException by default
    ON_CALL(*client, processSub(::testing::_))
        .WillByDefault(::testing::Throw(nats::ExistingSubscriptionIdException()));

    EXPECT_CALL(*client, processSub(::testing::Eq(std::string_view("foo.bar.> 10")))).Times(1);
    // We expect sendErrorMessage and resetParsingVars to be called
    EXPECT_CALL(*client, sendErrorMessage(::testing::_)).Times(1);
    EXPECT_CALL(*client, resetParsingVars()).Times(1);

    NatsParser::parse(client, sub.data(), sub.size());
}

TEST_F(ParserTest, Parser_Failure_InvalidCommand_AllStates) {
    std::vector<std::string> invalid_commands = {
        // Invalid after C, CO, CON, etc.
        "CI\r\n", "COX\r\n", "CONX\r\n", "CONNEX\r\n", "CONNECX\r\n", "CONNECTX\r\n", "CONNECT abcd}{\r\n",
        // Invalid after P, PI, PIN, PING
        "PL\r\n", "PIG\r\n", "PINS\r\n", "PINGS\r\n",
        // Invalid after PO, PON, PONG
        "POMXML\r\n", "PONK\r\n", "PONGX\r\n",
        // Invalid after PU, PUB
        "PUX\r\n", "PUBXTEST\r\n", "PUB foo.bar 5\r\nHello\n", "PUB foo.bar 5\r\nHello\rTest"
        // Invalid after S, SU, SUB, etc.
        "SRNEW\r\n", "SUX\r\n", "SUBLEW\r\n",
        // Invalid after U, UN, UNS, etc.
        "UP\r\n","UNX\r\n", "UNSI\r\n", "UNSUY\r\n", "UNSUBH\r\n",
        // Completely unknown
        "FOO\r\n", "BAR\r\n", "XYZ\r\n"
    };

    for (const auto& cmd : invalid_commands) {
        MockNatsClient local_client(&server);

        EXPECT_CALL(local_client, closeConnection(::testing::_)).Times(1);
        EXPECT_CALL(local_client, processPubArgs(::testing::_)).Times(::testing::AnyNumber());
        NatsParser::parse(&local_client, const_cast<char*>(cmd.data()), cmd.size());
        // Clear expectations for the next iteration
        ::testing::Mock::VerifyAndClearExpectations(&local_client);
    }
}