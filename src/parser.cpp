#include "../include/nats/parser.hpp"
#include "../include/nats/parser_state.hpp"
#include "../include/nats/client.hpp"
#include "../include/nats/custom_base_exceptions.hpp"
#include "../include/nats/custom_specific_exceptions.hpp"
#include <iostream>
#include <cstring>
#include <string_view>
#include <nlohmann/json.hpp>

using namespace std; 

namespace nats{
    void NatsParser::parse (NatsClient* c, char* buf, int buffer_size){
        try{
            char b;
            for(int i=0;i<buffer_size;i++){
                b = *(buf+i);
                switch(c->m_state){
                    case NatsParserState::OP_START:
                        if(b=='C' || b=='c'){
                            c->m_state = NatsParserState::OP_C;
                        } else if(b=='P' || b=='p'){
                            c->m_state = NatsParserState::OP_P;  
                        } else if(b=='S' || b=='s'){
                            c->m_state = NatsParserState::OP_S;
                        } else if(b=='U' || b=='u'){
                            c->m_state = NatsParserState::OP_U;
                        }else{
                            throw UnknownProtocolOperationException();
                        }
                        break;
                    case NatsParserState::OP_C:
                        if(b=='O' || b=='o'){
                            c->m_state = NatsParserState::OP_CO;
                        } else{
                            throw UnknownProtocolOperationException();
                        }
                        break;
                    case NatsParserState::OP_CO:
                        if(b=='N' || b=='n'){
                            c->m_state = NatsParserState::OP_CON;
                        } else{
                            throw UnknownProtocolOperationException();
                        }
                        break;
                    case NatsParserState::OP_CON:
                        if(b=='N' || b=='n'){
                            c->m_state = NatsParserState::OP_CONN;
                        } else{
                            throw UnknownProtocolOperationException();
                        }
                        break;
                    case NatsParserState::OP_CONN:
                        if(b=='E' || b=='e'){
                            c->m_state = NatsParserState::OP_CONNE;
                        } else{
                            throw UnknownProtocolOperationException();
                        }
                        break;
                    case NatsParserState::OP_CONNE:
                        if(b=='C' || b=='c'){
                            c->m_state = NatsParserState::OP_CONNEC;
                        } else{
                            throw UnknownProtocolOperationException();
                        }
                        break;
                    case NatsParserState::OP_CONNEC:
                        if(b=='T' || b=='t'){
                            c->m_state = NatsParserState::OP_CONNECT;
                        } else{
                            throw UnknownProtocolOperationException();
                        }
                        break;
                    case NatsParserState::OP_CONNECT:
                        if(b==' '||b=='\t'){
                            c->m_state = NatsParserState::OP_CONNECT_SPC;
                        } else{
                            throw UnknownProtocolOperationException();
                        }
                        break;
                    case NatsParserState::OP_CONNECT_SPC:
                        if(b==' '||b=='\t'){
                        } else if(b=='{') {
                            c->m_state = NatsParserState::CONNECT_ARG;
                            c->m_as=i;
                        } else {
                            throw UnknownProtocolOperationException();
                        }
                        break;
                    case NatsParserState::CONNECT_ARG:
                        if(b=='\r'){
                            c->m_drop=1;
                        } else if(b=='\n'){
                            //validate json
                            //string view is memory efficient
                            //as it doesnt copy contents rather just gives a view of the memory contents of the char buffer
                            string_view json_view;
                            if(c->m_arg_len>0){
                                json_view = string_view(c->m_arg_buffer,c->m_arg_len);
                            } else{
                                json_view = string_view(buf + c->m_as, i - c->m_drop - c->m_as);
                            } 

                            try{
                                nlohmann::json::parse(json_view, nullptr, true);
                            } catch(const nlohmann::json::parse_error& e){
                                //indicates invalid json
                                throw JsonParseExcpetion();
                            }

                            //process connect
                            c->processConnect();
                            c->resetParsingVars();
                        } else {
                            if(c->m_arg_len>0){
                                if(c->maxArgSizeReached()){
                                    throw MaximumArgumentSizeReachedException();
                                }
                                c->m_arg_buffer[c->m_arg_len] = b;
                                c->m_arg_len++;
                            }
                        }
                        
                        break;
                    case NatsParserState::OP_P:
                        if(b=='I'||b=='i'){
                            c->m_state = NatsParserState::OP_PI;
                        } else if(b=='O'||b=='o'){
                            c->m_state = NatsParserState::OP_PO;
                        } else if(b=='U'||b=='u'){
                            c->m_state = NatsParserState::OP_PU;
                        } else {
                            throw UnknownProtocolOperationException();
                        }
                        break;
                    case NatsParserState::OP_PI:
                        if(b=='N'||b=='n'){
                            c->m_state = NatsParserState::OP_PIN;
                        } else{
                            throw UnknownProtocolOperationException();
                        }
                        break;
                    case NatsParserState::OP_PIN:
                        if(b=='G'||b=='g'){
                            c->m_state = NatsParserState::OP_PING;
                        } else {
                            throw UnknownProtocolOperationException();
                        }
                        break;
                    case NatsParserState::OP_PING:
                        //since ping is just a operational check command and doesnt change any configuration
                        //its not handled as seriously as connect or pub or sub
                        if(b=='\r'){
                        } else if (b=='\n'){
                            //complete ping received
                            c->processPing();
                            c->resetParsingVars();
                        } else {
                            throw UnknownProtocolOperationException();
                        }
                        break;
                    case NatsParserState::OP_PO:
                        if(b=='N'||b=='n'){
                            c->m_state = NatsParserState::OP_PON;
                        } else {
                            throw UnknownProtocolOperationException();
                        }
                        break;
                    case NatsParserState::OP_PON:
                        if(b=='G'||b=='g'){
                            c->m_state = NatsParserState::OP_PONG;
                        } else {
                            throw UnknownProtocolOperationException();
                        }
                        break;
                    case NatsParserState::OP_PONG:
                        //since pong is just a operational check command and doesnt change any configuration
                        //its not handled as seriously as connect or pub or sub
                        if(b=='\r'){
                        } else if (b=='\n'){
                            //complete pong received
                            c->processPong();
                            c->resetParsingVars();
                        } else {
                            throw UnknownProtocolOperationException();
                        }
                        break;
                    case NatsParserState::OP_PU:
                        if(b=='b'||b=='B'){
                            c->m_state = NatsParserState::OP_PUB;
                        } else {
                            throw UnknownProtocolOperationException();
                        }
                        break;
                    case NatsParserState::OP_PUB:
                        if(b==' '||b=='\t'){
                            c->m_state = NatsParserState::OP_PUB_SPC;
                        } else {
                            throw UnknownProtocolOperationException();
                        }
                    case NatsParserState::OP_PUB_SPC:
                        if(b==' '||b=='\t'){
                        } else {
                            c->m_state = NatsParserState::PUB_ARG;
                            c->m_as=i;
                        }
                        break;
                    case NatsParserState::PUB_ARG:
                        if(b=='\r'){
                            c->m_drop=1;
                        } else if(b=='\n'){
                            //string view is memory efficient
                            //as it doesnt copy contents rather just gives a view of the memory contents of the char buffer
                            string_view arg_view;
                            if(c->m_arg_len>0){
                                arg_view = string_view(c->m_arg_buffer,c->m_arg_len);
                            } else{
                                arg_view = string_view(buf + c->m_as, i - c->m_drop - c->m_as);
                            } 

                            c->processPubArgs(arg_view);
                            c->m_state = NatsParserState::MSG_PAYLOAD;
                            c->m_drop = 0;
                            c->m_as = i+1;
                        } else {
                            if(c->m_arg_len>0){
                                if(c->maxArgSizeReached()){
                                    throw MaximumArgumentSizeReachedException();
                                }
                                c->m_arg_buffer[c->m_arg_len] = b;
                                c->m_arg_len++;
                            }
                        }
                        
                        break;
                    case NatsParserState::MSG_PAYLOAD:
                        if(b=='\r'){
                            c->m_drop = 1;
                            c->m_state =  NatsParserState::MSG_END_R;
                        } else if(b=='\n'){
                            throw MessageParseException();
                        } else {
                            if(c->m_msg_len>0){
                                if(c->maxMessageSizeReached()){
                                    throw MaximumMessageSizeReachedException();
                                }
                                c->m_msg_buffer[c->m_msg_len] = b;
                                c->m_msg_len++;
                            }
                        }
                        break;
                    case NatsParserState::MSG_END_R:
                        if(b=='\n'){
                            c->m_state = NatsParserState::MSG_END_N;
                        } else{
                            throw MessageParseException();
                        }
                    case NatsParserState::MSG_END_N:
                        if(b=='\n'){
                            string_view msg_view;
                            if(c->m_msg_len>0){
                                msg_view = string_view(c->m_msg_buffer,c->m_msg_len);
                            } else{
                                msg_view = string_view(buf + c->m_as, i - c->m_drop - c->m_as);
                            } 
                            if(msg_view.size()!=c->m_payload_size){
                                throw PayloadSizeMismatchException();
                            }
                            c->processPub(msg_view);
                            c->resetParsingVars();
                        } else{
                            throw MessageParseException();
                        }
                        break;
                    case NatsParserState::OP_S:
                        if(b=='U' || b=='u'){
                            c->m_state = NatsParserState::OP_SU;
                        } else {
                            throw UnknownProtocolOperationException();
                        }
                        break;
                    case NatsParserState::OP_SU:
                        if(b=='B' || b=='b'){
                            c->m_state = NatsParserState::OP_SUB;
                        } else {
                            throw UnknownProtocolOperationException();
                        }
                        break;
                    case NatsParserState::OP_SUB:
                        if(b==' '||b=='\t'){
                            c->m_state = NatsParserState::OP_SUB_SPC;
                        } else {
                            throw UnknownProtocolOperationException();
                        }
                        break;
                    case NatsParserState::OP_SUB_SPC:
                        if(b==' '||b=='\t'){
                        } else {
                            c->m_state = NatsParserState::SUB_ARG;
                            c->m_as = i;
                        }
                        break;
                    case NatsParserState::SUB_ARG:
                        if(b=='\r'){
                            c->m_drop=1;
                        } else if(b=='\n'){
                            //string view is memory efficient
                            //as it doesnt copy contents rather just gives a view of the memory contents of the char buffer
                            string_view arg_view;
                            if(c->m_arg_len>0){
                                arg_view = string_view(c->m_arg_buffer,c->m_arg_len);
                            } else{
                                arg_view = string_view(buf + c->m_as, i - c->m_drop - c->m_as);
                            } 

                            c->processSub(arg_view);
                            c->resetParsingVars();
                        } else {
                            if(c->m_arg_len>0){
                                if(c->maxArgSizeReached()){
                                    throw MaximumArgumentSizeReachedException();
                                }
                                c->m_arg_buffer[c->m_arg_len] = b;
                                c->m_arg_len++;
                            }
                        }
                        
                        break;
                    case NatsParserState::OP_U:
                        if(b=='N' || b=='n'){
                            c->m_state = NatsParserState::OP_UN;
                        } else {
                            throw UnknownProtocolOperationException();
                        }
                        break;
                    case NatsParserState::OP_UN:
                        if(b=='S' || b=='s'){
                            c->m_state = NatsParserState::OP_UNS;
                        } else {
                            throw UnknownProtocolOperationException();
                        }
                        break;
                    case NatsParserState::OP_UNS:
                        if(b=='U' || b=='u'){
                            c->m_state = NatsParserState::OP_UNSU;
                        } else {
                            throw UnknownProtocolOperationException();
                        }
                        break;
                    case NatsParserState::OP_UNSU:
                        if(b=='B' || b=='b'){
                            c->m_state = NatsParserState::OP_UNSUB;
                        } else {
                            throw UnknownProtocolOperationException();
                        }
                        break;
                    case NatsParserState::OP_UNSUB:
                        if(b==' '||b=='\t'){
                            c->m_state = NatsParserState::OP_UNSUB_SPC;
                        } else {
                            throw UnknownProtocolOperationException();
                        }
                        break;
                    case NatsParserState::OP_UNSUB_SPC:
                        if(b==' '||b=='\t'){
                        } else {
                            c->m_state = NatsParserState::UNSUB_ARG;
                            c->m_as = i;
                        }
                        break;
                    case NatsParserState::UNSUB_ARG:
                        if(b=='\r'){
                            c->m_drop=1;
                        } else if(b=='\n'){
                            //string view is memory efficient
                            //as it doesnt copy contents rather just gives a view of the memory contents of the char buffer
                            string_view arg_view;
                            if(c->m_arg_len>0){
                                arg_view = string_view(c->m_arg_buffer,c->m_arg_len);
                            } else{
                                arg_view = string_view(buf + c->m_as, i - c->m_drop - c->m_as);
                            } 

                            c->processUnsub(arg_view);
                            c->resetParsingVars();
                        } else {
                            if(c->m_arg_len>0){
                                if(c->maxArgSizeReached()){
                                    throw MaximumArgumentSizeReachedException();
                                }
                                c->m_arg_buffer[c->m_arg_len] = b;
                                c->m_arg_len++;
                            }
                        }
                        break;
                    default:
                        throw UnknownProtocolOperationException();
                        break;
                }
            }
            //This indicates that the buffer is ending but the argument list hasnt been completely sent yet
            if(c->m_state==NatsParserState::OP_START){
                //Operation successfully parsed and we are back to OP_START
            } else if(
                c->m_state==NatsParserState::CONNECT_ARG 
                || c->m_state==NatsParserState::PUB_ARG
                || c->m_state==NatsParserState::SUB_ARG
                || c->m_state==NatsParserState::UNSUB_ARG
                ){
                if(c->m_arg_len==0){
                    c->m_arg_len = buffer_size - c->m_as;
                    memcpy(c->m_arg_buffer, buf+c->m_as, c->m_arg_len);
                }
            } else if(c->m_state==NatsParserState::MSG_PAYLOAD) {
                if(c->m_msg_len==0){
                    if(c->m_as==buffer_size){
                        c->m_as =0;
                    } else{
                        c->m_msg_len = buffer_size - c->m_as;
                        memcpy(c->m_msg_buffer, buf+c->m_as, c->m_msg_len); 
                    }
                }
            } else {
                throw UnknownProtocolOperationException();
            }
        } catch (const NatsParserException &ex) {
            c->closeConnection("A Parser Exception occured : " + string(ex.what()) + "\r\n");
		} catch(const NatsNonFatalParserException &ex){
            c->sendErrorMessage("A Parser Exception occured : " + string(ex.what()) + "\r\n");
            c->resetParsingVars();
        } catch (...) {
            c->closeConnection("An unexpected error occured!\r\n");
		}
    }
}
