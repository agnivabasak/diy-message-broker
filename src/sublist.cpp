#include "../include/nats/sublist.hpp"
#include "../include/nats/subscription.hpp"
#include "../include/nats/sublist_node.hpp"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <mutex>
#include <memory>

using namespace std;

namespace nats{

    NatsSublist::NatsSublist(){
        m_head = std::make_unique<NatsSublistNode>();
    }

    void NatsSublist::addSubscription(NatsSubscription subscription, std::vector<std::string>& subject_list){
        std::lock_guard<std::mutex> lock(m_sublist_mutex);
        NatsSublistNode* cur_node = m_head.get();
        //reach the correct subject nodes and if it doesn't exist create one
        for(std::string& subject_part: subject_list){
            if(cur_node->m_next.find(subject_part) == cur_node->m_next.end()){
                cur_node->m_next[subject_part] = std::make_unique<NatsSublistNode>();
            }
            cur_node = cur_node->m_next[subject_part].get();
        }
        //now that we are at the current node, we add the subscription
        cur_node->m_subscriptions.insert(subscription);
    }

    void NatsSublist::removeSubscription(NatsSubscription& subscription, std::vector<std::string>& subject_list){
        std::lock_guard<std::mutex> lock(m_sublist_mutex);
        NatsSublistNode* cur_node = m_head.get();
        //reach the correct subject nodes and if it doesn't exist, ignore
        for(std::string& subject_part: subject_list){
            if(cur_node->m_next.find(subject_part) != cur_node->m_next.end()){
                cur_node = cur_node->m_next[subject_part].get();
            } else {
                return;
            }
        }
        //now that we are at the current node, we erase the subscription if it exists
        cur_node->m_subscriptions.erase(subscription);

    }
}