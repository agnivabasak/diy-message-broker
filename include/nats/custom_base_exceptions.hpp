#ifndef NATS_CUSTOM_BASE_EXCEPTIONS_H
#define NATS_CUSTOM_BASE_EXCEPTIONS_H

#include <stdexcept>
#include <string>

namespace nats
{
    class NatsParserException : public std::runtime_error {
    public:
        explicit NatsParserException(const std::string& message)
            : std::runtime_error(message) {}
    };

    class NatsNonFatalParserException : public std::runtime_error {
        public:
            explicit NatsNonFatalParserException(const std::string& message)
                : std::runtime_error(message) {}
        };
}

#endif