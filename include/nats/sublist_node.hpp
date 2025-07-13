#ifndef NATS_SUBLIST_NODE_H
#define NATS_SUBLIST_NODE_H

#include "subscription.hpp"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>

namespace nats{
    class NatsSublistNode{
    public:
        std::unordered_map<std::string,std::unique_ptr<NatsSublistNode>> m_next;
        std::unordered_set<NatsSubscription, NatsSubscriptionHash> m_subscriptions;
    };
}

#endif