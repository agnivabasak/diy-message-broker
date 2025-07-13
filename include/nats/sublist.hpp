#ifndef NATS_SUBLIST_H
#define NATS_SUBLIST_H

#include "sublist_node.hpp"
#include "subscription.hpp"
#include <mutex>
#include <vector>
#include <memory>

namespace nats{
    //A trie-like data structure to store subscription details
    class NatsSublist{
        std::unique_ptr<NatsSublistNode> m_head;
        std::mutex m_sublist_mutex; //to make sure the sublist is not changed by multiple threads at the same time
        void addSubscriptionsToVectorFromSublistNode(NatsSublistNode* cur_node, std::vector<NatsSubscription>& subscriptions);
        public:
        NatsSublist();
        void addSubscription(NatsSubscription subscription, std::vector<std::string>& subject_list);
        void removeSubscription(NatsSubscription& subscription, std::vector<std::string>& subject_list);
        std::vector<NatsSubscription> getSubscriptionsForTopic(std::vector<std::string>& subject_list);
    };
}

#endif