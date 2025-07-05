#ifndef NATS_PARSER_H
#define NATS_PARSER_H

#include "parser_state.hpp"
#include "client.hpp"

namespace nats{
    class NatsParser{
        public:
            NatsParser();
            static void parse(NatsClient* c, char* buf, int buffer_size);
    };
}

#endif