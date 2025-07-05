#ifndef NATS_CUSTOM_SPECIFIC_EXCEPTIONS_H
#define NATS_CUSTOM_SPECIFIC_EXCEPTIONS_H

#include "custom_base_exceptions.hpp"
#include <stdexcept>
#include <string>

namespace nats
{
    class UnknownProtocolOperationException: public NatsParserException {
    public:
        explicit UnknownProtocolOperationException()
            : NatsParserException("Unknown Protocol Operation!") {}
    };

    class ConnectionOperationExpectedException: public NatsParserException {
    public:
        explicit ConnectionOperationExpectedException()
            : NatsParserException("Expected CONNECT Operation!") {}
    };

    class PongOperationExpectedException: public NatsParserException {
    public:
        explicit PongOperationExpectedException()
            : NatsParserException("Expected PONG Operation!") {}
    };

    class MaximumArgumentSizeReached: public NatsParserException {
    public:
        explicit MaximumArgumentSizeReached()
            : NatsParserException("Maximum argument size reached!"){}
    };

    class JsonParseExcpetion: public NatsParserException {
    public:
        explicit JsonParseExcpetion()
            : NatsParserException("Invalid JSON provided!"){}
    };
}

#endif