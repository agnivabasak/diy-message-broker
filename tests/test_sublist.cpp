#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../include/nats/sublist.hpp"
#include "../include/nats/subscription.hpp"

using namespace nats;

TEST(NatsSublistTest, AddAndGetSubscription) {
    NatsSublist sublist;
    NatsSubscription sub{1, 100};
    std::vector<std::string> subject = {"foo", "bar"};

    sublist.addSubscription(sub, subject);

    auto result = sublist.getSubscriptionsForTopic(subject);
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], sub);
}

TEST(NatsSublistTest, RemoveSubscription) {
    NatsSublist sublist;
    NatsSubscription sub{1, 100};
    std::vector<std::string> subject = {"foo", "bar"};

    sublist.addSubscription(sub, subject);
    sublist.removeSubscription(sub, subject);

    auto result = sublist.getSubscriptionsForTopic(subject);
    EXPECT_TRUE(result.empty());
}

TEST(NatsSublistTest, WildcardSingleLevel_1) {
    NatsSublist sublist;
    NatsSubscription sub{1, 100};
    std::vector<std::string> wildcard = {"foo", "*"};
    std::vector<std::string> match = {"foo", "bar"};

    sublist.addSubscription(sub, wildcard);

    auto result = sublist.getSubscriptionsForTopic(match);
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], sub);
}

TEST(NatsSublistTest, WildcardSingleLevel_2) {
    NatsSublist sublist;
    NatsSubscription sub_1{1, 100};
    NatsSubscription sub_2{2, 200};
    std::vector<std::string> wildcard_1 = {"foo", "*"};
    std::vector<std::string> wildcard_2 = {"foo", "bar"};
    std::vector<std::string> wildcard_3 = {"foo", "random"};
    std::vector<std::string> match = {"foo", "bar"};

    sublist.addSubscription(sub_1, wildcard_1);
    sublist.addSubscription(sub_2, wildcard_2);
    sublist.addSubscription(sub_2, wildcard_3);

    auto result = sublist.getSubscriptionsForTopic(match);
    ASSERT_EQ(result.size(), 2);
    // Check that both sub_1 and sub_2 are present, regardless of order
    EXPECT_THAT(result, ::testing::UnorderedElementsAre(sub_1, sub_2));
}

TEST(NatsSublistTest, WildcardMultiLevel_1) {
    NatsSublist sublist;
    NatsSubscription sub{1, 100};
    std::vector<std::string> wildcard = {"foo", ">"};
    std::vector<std::string> match = {"foo", "bar", "baz"};

    sublist.addSubscription(sub, wildcard);

    auto result = sublist.getSubscriptionsForTopic(match);
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], sub);
}

TEST(NatsSublistTest, WildcardMultiLevel_2) {
    NatsSublist sublist;
    NatsSubscription sub{1, 100};
    std::vector<std::string> wildcard = {"foo", "*", "bar", ">"};
    std::vector<std::string> match = {"foo", "test", "bar", "baz", "somethingElse"};

    sublist.addSubscription(sub, wildcard);

    auto result = sublist.getSubscriptionsForTopic(match);
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], sub);
}

TEST(NatsSublistTest, WildcardMultiLevel_3) {
    NatsSublist sublist;
    NatsSubscription sub_1{1, 100};
    NatsSubscription sub_2{2, 200}; 
    NatsSubscription sub_3{3, 300}; 

    std::vector<std::string> wildcard_1 = {"foo", "*", "bar", ">"};
    std::vector<std::string> wildcard_2 = {"foo", ">"};
    std::vector<std::string> wildcard_3 = {"foo", "*", "baz", ">"};
    std::vector<std::string> match = {"foo", "test", "bar", "baz", "somethingElse"};

    sublist.addSubscription(sub_1, wildcard_1);
    sublist.addSubscription(sub_2, wildcard_2);
    sublist.addSubscription(sub_3, wildcard_3);

    auto result = sublist.getSubscriptionsForTopic(match);
    ASSERT_EQ(result.size(), 2);
    // Check that both sub_1 and sub_2 are present, regardless of order
    EXPECT_THAT(result, ::testing::UnorderedElementsAre(sub_1, sub_2));
}

TEST(NatsSublistTest, NoMatch) {
    NatsSublist sublist;
    NatsSubscription sub{1, 100};
    std::vector<std::string> subject = {"foo", "bar"};

    sublist.addSubscription(sub, subject);

    std::vector<std::string> nonmatch = {"foo", "baz"};
    auto result = sublist.getSubscriptionsForTopic(nonmatch);
    EXPECT_TRUE(result.empty());
}

TEST(NatsSublistTest, DuplicateAdd) {
    NatsSublist sublist;
    NatsSubscription sub{1, 100};
    std::vector<std::string> subject = {"foo", "bar"};

    sublist.addSubscription(sub, subject);
    sublist.addSubscription(sub, subject);

    auto result = sublist.getSubscriptionsForTopic(subject);
    ASSERT_EQ(result.size(), 1); // Should not duplicate
}

TEST(NatsSublistTest, RemoveNonExistent) {
    NatsSublist sublist;
    NatsSubscription sub{1, 100};
    std::vector<std::string> subject = {"foo", "bar"};

    // Remove before add should not crash
    sublist.removeSubscription(sub, subject);

    // Add and remove different subscription
    sublist.addSubscription(sub, subject);
    NatsSubscription sub2{2, 200};
    sublist.removeSubscription(sub2, subject);

    auto result = sublist.getSubscriptionsForTopic(subject);
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], sub);
}