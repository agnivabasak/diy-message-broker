#ifndef NATS_SUBSCRIPTION_H
#define NATS_SUBSCRIPTION_H

#include <cstddef>
#include <functional>

namespace nats{
    struct NatsSubscription {
        int m_sub_id;
        long long m_client_id;

        bool operator==(const NatsSubscription& other) const {
            return m_sub_id == other.m_sub_id && m_client_id == other.m_client_id;
        }
    };
    
    struct NatsSubscriptionHash {
        std::size_t operator()(const NatsSubscription& sub) const {
            std::size_t h1 = std::hash<int>{}(sub.m_sub_id);
            std::size_t h2 = std::hash<long long>{}(sub.m_client_id);
            return h1 ^ (h2 << 1);
        }
    };
}

#endif