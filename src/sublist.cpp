#include "../include/nats/sublist.hpp"
#include "../include/nats/subscription.hpp"
#include "../include/nats/sublist_node.hpp"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <mutex>
#include <memory>
#include <queue>

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

    std::vector<NatsSubscription> NatsSublist::getSubscriptionsForTopic(std::vector<std::string>& subject_list){
        std::lock_guard<std::mutex> lock(m_sublist_mutex);
        std::vector<NatsSubscription> subscriptions;
        NatsSublistNode* cur_node;
        std:queue<NatsSublistNode*> q;
        q.push(m_head.get());
        //we do a bfs to get all the subscriptions, each level is essentially one of the subsubjects in the subject_list
        for(std::string& subject_part: subject_list){
            int level_count = q.size();
            for(int i=0;i<level_count;i++){
                cur_node = q.front();
                q.pop();
                if(cur_node->m_next.find(subject_part) != cur_node->m_next.end()){
                    q.push(cur_node->m_next[subject_part].get());
                }
                if(cur_node->m_next.find("*") != cur_node->m_next.end()){
                    q.push(cur_node->m_next["*"].get());
                }
                if(cur_node->m_next.find(">") != cur_node->m_next.end()){
                    //cover the case where ">" covers everything after the previous subject_part
                    cur_node = cur_node->m_next[">"].get();
                    addSubscriptionsToVectorFromSublistNode(cur_node,subscriptions);
                }
            }
        }
        //now if there is anything in the queue, then that means we reached the last subject_part and have some NatsSublistNodes available
        while(!q.empty()){
            cur_node = q.front();
            q.pop();
            addSubscriptionsToVectorFromSublistNode(cur_node,subscriptions);
        }
        return subscriptions;
    }

    void NatsSublist::addSubscriptionsToVectorFromSublistNode(NatsSublistNode* cur_node, std::vector<NatsSubscription>& subscriptions){
        for(const auto& sub : cur_node->m_subscriptions){
            subscriptions.push_back(sub);
        }
    }
}