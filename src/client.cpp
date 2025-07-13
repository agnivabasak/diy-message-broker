#include "../include/nats/client.hpp"
#include "../include/nats/server.hpp"
#include "../include/nats/parser_state.hpp"
#include "../include/nats/subscription.hpp"
#include "../include/nats/custom_specific_exceptions.hpp"
#include <random>
#include <utility>
#include <climits>
#include <sys/socket.h>
#include <unistd.h>
#include <string>
#include <cstring>
#include <vector>
#include <string_view>
#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>

using namespace std;

namespace nats{
    NatsClient::NatsClient(int client_fd, NatsServer* server): 
        m_waiting_for_initial_connect(true), 
        m_waiting_for_initial_pong(false),
        m_arg_len(0),
        m_msg_len(0),
        m_as(-1),
        m_drop(0),
        m_state(NatsParserState::OP_START),
        m_payload_size(0),
        m_client_fd(client_fd),
        m_server(server)
    {
        random_device rd;
        mt19937 gen(rd());
        uniform_int_distribution<long long> dis(1, LLONG_MAX);
        
        m_client_id = dis(gen);
    }

    NatsClient::~NatsClient() {
        stopTimeoutThread();
        // we need to remove the subscriptions from the common sublist of this client
        m_server->removeSubscriptions(getUnsubParams());
    }

    void NatsClient::closeConnection(){
        stopTimeoutThread();
        close(m_client_fd);
    }

    void NatsClient::sendErrorMessage(string msg){
        send(m_client_fd, msg.c_str(), msg.size(), 0);
    }

    void NatsClient::closeConnection(string msg){
        stopTimeoutThread();
        send(m_client_fd, msg.c_str(), msg.size(), 0);
        close(m_client_fd);
    }

    void NatsClient::resetParsingVars(){
        //reset everything
        m_state = NatsParserState::OP_START;
        m_as = -1;
        m_arg_len = 0;
        m_msg_len = 0;
        m_drop = 0;
        m_payload_size = 0;
        memset(m_arg_buffer, 0, INTERNAL_BUFFER_SIZE);
        memset(m_msg_buffer, 0, INTERNAL_BUFFER_SIZE);
        memset(m_payload_sub, 0, INTERNAL_BUFFER_SIZE);
    }

    bool NatsClient::maxArgSizeReached(){
        return m_arg_len>=INTERNAL_BUFFER_SIZE;
    }

    bool NatsClient::maxMessageSizeReached(){
        return m_msg_len>=INTERNAL_BUFFER_SIZE-1;
    }

    void NatsClient::verifyState(){
        if(m_waiting_for_initial_connect && m_state != NatsParserState::CONNECT_ARG){
            throw ConnectionOperationExpectedException();
        } else if(m_waiting_for_initial_pong && m_state != NatsParserState::OP_PONG){
            throw PongOperationExpectedException();
        }
        //else good to go
    }

    void NatsClient::startPongTimeoutThread() {
        if (m_timeout_thread_running) {
            return; // Already running
        }
        
        m_timeout_thread_running = true;
        m_timeout_thread = std::thread([this]() {
            auto start_time = std::chrono::steady_clock::now();
            
            while (m_timeout_thread_running) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Check every 100ms
                
                if (!m_waiting_for_initial_pong) {
                    // Pong received, stop monitoring
                    break;
                }
                
                auto now = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::minutes>(now - start_time);
                
                if (elapsed.count() >= 1) {
                    // Send timeout message and close socket directly
                    string timeout_msg = "Pong timeout occured. Connection closed!\r\n";
                    send(m_client_fd, timeout_msg.c_str(), timeout_msg.size(), 0);
                    close(m_client_fd);
                    
                    // Signal that we should stop the thread
                    m_timeout_thread_running = false;
                    break;
                }
            }
        });
    }

    void NatsClient::stopTimeoutThread() {
        if (m_timeout_thread_running) {
            m_timeout_thread_running = false;
            if (m_timeout_thread.joinable()) {
                m_timeout_thread.join();
            }
        }
    }

    void NatsClient::processConnect(){
        verifyState();
        if(m_waiting_for_initial_connect){
            m_waiting_for_initial_connect = false;
            send(m_client_fd, "+OK\r\nPING\r\n", 11, 0);
            m_waiting_for_initial_pong = true;
            startPongTimeoutThread();
        } else{  
            send(m_client_fd, "+OK\r\n", 5, 0);
        }
    }

    void NatsClient::processPing(){
        verifyState();
        send(m_client_fd, "PONG\r\n", 6, 0);
    }

    void NatsClient::processPong(){
        verifyState();
        if(m_waiting_for_initial_pong){
            m_waiting_for_initial_pong=false;
        }
    }

    void NatsClient::processPubArgs(std::string_view& pub_args){
        // Find the first space
        size_t space_pos = pub_args.find(' ');
        if (space_pos == std::string_view::npos) {
            throw ArgumentParseException();
        }

        // Extract subject and payload size as string_views
        std::string_view subject = pub_args.substr(0, space_pos);
        std::string_view payload_size_str = pub_args.substr(space_pos + 1);

        // Remove any leading/trailing spaces from payload_size_str
        payload_size_str.remove_prefix(std::min(payload_size_str.find_first_not_of(' '), payload_size_str.size()));
        payload_size_str.remove_suffix(payload_size_str.size() - payload_size_str.find_last_not_of(' ') - 1);

        // Check for empty subject or payload_size_str
        if (subject.empty() || payload_size_str.empty() || payload_size_str.find(' ') != std::string_view::npos) {
            throw ArgumentParseException();
        }

        // Parse payload_size_str as integer
        size_t idx = 0;
        int payload_size = 0;
        try {
            payload_size = std::stoi(std::string(payload_size_str), &idx);
        } catch (...) {
            throw ArgumentParseException();
        }
        // Ensure the whole string was parsed
        if (idx != payload_size_str.size() || payload_size < 0) {
            throw ArgumentParseException();
        }
        if(payload_size>INTERNAL_BUFFER_SIZE){
            throw MaximumMessageSizeReachedException();
        }

        m_payload_size = payload_size;

        std::memset(m_payload_sub, 0, INTERNAL_BUFFER_SIZE);
        size_t copy_len = std::min(subject.size(), static_cast<size_t>(INTERNAL_BUFFER_SIZE - 1));
        std::memcpy(m_payload_sub, subject.data(), copy_len);
        m_payload_sub[copy_len] = '\0';
    }

    void NatsClient::processPub(string_view& payload){
        //parse subject and convert to subject list
        std::string_view subject_view(m_payload_sub);
        std::vector<std::string> subject_list = convertSubjectToList(subject_view, true);
        send(m_client_fd, "+OK\r\n", 5, 0);
    }

    void NatsClient::processSub(string_view& sub_args){
        // Find the first space
        size_t space_pos = sub_args.find(' ');
        if (space_pos == std::string_view::npos) {
            throw ArgumentParseException();
        }

        // Extract subject and subscription id as string_views
        std::string_view subject = sub_args.substr(0, space_pos);
        std::string_view sub_id_str = sub_args.substr(space_pos + 1);

        // Remove any leading/trailing spaces from sub_id
        sub_id_str.remove_prefix(std::min(sub_id_str.find_first_not_of(' '), sub_id_str.size()));
        sub_id_str.remove_suffix(sub_id_str.size() - sub_id_str.find_last_not_of(' ') - 1);

        // Check for empty subject or sub_id
        if (subject.empty() || sub_id_str.empty() || sub_id_str.find(' ') != std::string_view::npos) {
            throw ArgumentParseException();
        }

        // Parse sub_id as integer
        size_t idx = 0;
        int sub_id = -1;
        try {
            sub_id = std::stoi(std::string(sub_id_str), &idx);
        } catch (...) {
            throw ArgumentParseException();
        }
        // Ensure the whole string was parsed
        if (idx != sub_id_str.size() || sub_id < 0) {
            throw ArgumentParseException();
        }

        //parse subject and convert to subject list
        std::vector<std::string> subject_list = convertSubjectToList(subject, false);

        //make server process the subscription and add to sublist
        m_server->addSubscription(sub_id,subject_list,m_client_id);
        //if adding to sublist succeeds then add subcription metadata to client
        addSubscriptionMetadata(sub_id,std::string(subject));

        send(m_client_fd, "+OK\r\n", 5, 0);
    }

    vector<pair<vector<std::string>,NatsSubscription>> NatsClient::getUnsubParams(bool filter_sub_id, int sub_id){
        vector<pair<vector<std::string>,NatsSubscription>> unsub_params;

        if(filter_sub_id){
            if(m_subscriptions.find(sub_id)!=m_subscriptions.end()){
                std::string_view cur_subscription_subject = m_subscriptions[sub_id];
                unsub_params.emplace_back(convertSubjectToList(cur_subscription_subject,false) , NatsSubscription{sub_id,m_client_id} );
            }
        } else {
            for(const auto& pair: m_subscriptions){
                std::string_view cur_subscription_subject = pair.second;
                unsub_params.emplace_back(convertSubjectToList(cur_subscription_subject,false) , NatsSubscription{pair.first,m_client_id});
            }
        }

        return unsub_params;
    }

    void NatsClient::addSubscriptionMetadata(int sub_id, std::string subject){
        if(m_subscriptions.find(sub_id)!=m_subscriptions.end()){
            //this means there is already a subscription with the current sub_id provided
            throw ExistingSubscriptionIdException();
        } else{
            m_subscriptions[sub_id]=subject;
        }
    }

    std::vector<std::string> NatsClient::convertSubjectToList(std::string_view& subject, bool is_publish) {
        std::vector<std::string> subject_list; // thread_local to avoid issues in multithreaded context
        subject_list.clear();
    
        size_t start = 0;
        while (start < subject.size()) {
            size_t end = subject.find('.', start);
            if (end == std::string_view::npos) end = subject.size();
            std::string token = std::string(subject.substr(start, end - start));
    
            // Check for empty token
            if (token.empty()) {
                if (is_publish)
                    throw InvalidPublishSubjectException();
                else
                    throw InvalidSubscribeSubjectException();
            }
    
            // For publish, "*" and ">" are not allowed as tokens
            if (is_publish && (token == "*" || token == ">")) {
                throw InvalidPublishSubjectException();
            }
    
            // If token is ">", it must be the last one
            if (token == ">" && end != subject.size()) {
                throw InvalidSubscribeSubjectException();
            }

            subject_list.push_back(token);
            start = end + 1;
        }
    
        return subject_list;
    }
}

