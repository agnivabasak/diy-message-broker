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

    class MaximumArgumentSizeReachedException: public NatsParserException {
    public:
        explicit MaximumArgumentSizeReachedException()
            : NatsParserException("Maximum argument size reached!"){}
    };

    class MaximumMessageSizeReachedException: public NatsParserException {
    public:
        explicit MaximumMessageSizeReachedException()
            : NatsParserException("Maximum message payload size reached!"){}
    };

    class JsonParseExcpetion: public NatsParserException {
    public:
        explicit JsonParseExcpetion()
            : NatsParserException("Invalid JSON provided!"){}
    };

    class ArgumentParseException: public NatsParserException {
    public:
        explicit ArgumentParseException()
            : NatsParserException("Incorrect argument format!") {}
    };

    class MessageParseException: public NatsParserException {
    public:
        explicit MessageParseException()
            : NatsParserException("Incorrect Message Payload format!") {}
    };

    class PayloadSizeMismatchException: public NatsParserException {
    public:
        explicit PayloadSizeMismatchException()
            : NatsParserException("Actual payload size doesn't match declared payload size!") {}
    };

    class InvalidPublishSubjectException: public NatsNonFatalParserException {
        public:
            explicit InvalidPublishSubjectException()
                : NatsNonFatalParserException("Invalid Publish Subject!") {}
    };

    class InvalidSubscribeSubjectException: public NatsNonFatalParserException {
        public:
            explicit InvalidSubscribeSubjectException()
                : NatsNonFatalParserException("Invalid Subscribe Subject!") {}
    };

    class ExistingSubscriptionIdException: public NatsNonFatalParserException {
        public:
            explicit ExistingSubscriptionIdException()
                : NatsNonFatalParserException("Subscription ID already exists!") {}
    };

    class NoSuchSubscriptionIdException: public NatsNonFatalParserException {
        public:
            explicit NoSuchSubscriptionIdException()
                : NatsNonFatalParserException("Subscription ID doesn't exist!") {}
    };
}

#endif