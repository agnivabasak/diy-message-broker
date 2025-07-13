#ifndef NATS_PARSER_STATE_H
#define NATS_PARSER_STATE_H

namespace nats{
    enum class NatsParserState {
        // START
        OP_START,
        // PING
        OP_P,
        OP_PI,
        OP_PIN,
        OP_PING,
        //PONG
        OP_PO,
        OP_PON,
        OP_PONG,
        // CONNECT
        OP_C,
        OP_CO,
        OP_CON,
        OP_CONN,
        OP_CONNE,
        OP_CONNEC,
        OP_CONNECT,
        OP_CONNECT_SPC,
        CONNECT_ARG,
        // PUBLISH
        OP_PU,
        OP_PUB,
        OP_PUB_SPC,
        PUB_ARG,
        MSG_PAYLOAD,
        MSG_END_R,
        MSG_END_N,
        // SUBSCRIBE
        OP_S,
        OP_SU,
        OP_SUB,
        OP_SUB_SPC,
        SUB_ARG,
        // UNSUBSCRIBE
        OP_U,
        OP_UN,
        OP_UNS,
        OP_UNSU,
        OP_UNSUB,
        OP_UNSUB_SPC,
        UNSUB_ARG
    };
}

#endif