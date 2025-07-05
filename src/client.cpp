#include "../include/nats/client.hpp"
#include "../include/nats/parser_state.hpp"
#include "../include/nats/custom_specific_exceptions.hpp"
#include <random>
#include <climits>
#include <sys/socket.h>
#include <unistd.h>
#include <string>
#include <cstring>
#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>

using namespace std;

namespace nats{
    NatsClient::NatsClient(int client_fd): 
        m_waiting_for_initial_connect(true), 
        m_waiting_for_initial_pong(false),
        m_arg_len(0),
        m_msg_len(0),
        m_as(-1),
        m_drop(0),
        m_state(NatsParserState::OP_START),
        m_payload_size(0),
        m_client_fd(client_fd)
    {
        random_device rd;
        mt19937 gen(rd());
        uniform_int_distribution<long long> dis(1, LLONG_MAX);
        
        m_client_id = dis(gen);
    }

    NatsClient::~NatsClient() {
        stopTimeoutThread();
    }

    void NatsClient::closeConnection(){
        stopTimeoutThread();
        close(m_client_fd);
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
}

