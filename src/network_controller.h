#pragma once

#include "network_protocol.h"

#include <stdint.h>

class NetworkController {
public:
    bool begin(uint8_t channel);
    void update(uint32_t nowMs, uint32_t randomWord);

    void pushReceived(const PanicPacket& pkt);
    bool popReceived(PanicPacket& out);
    void scheduleOrigin(const uint8_t* uuid, uint32_t nowMs);
    void scheduleRelay(const uint8_t* uuid, uint32_t nowMs);

private:
    void pumpTransmit(uint32_t nowMs);
    bool enqueueTx(const uint8_t* uuid, bool isRelay, uint32_t nowMs, uint32_t randomWord);

    struct TxJob {
        uint8_t uuid[16];
        uint8_t sendsLeft;
        uint32_t nextSendMs;
        bool active;
    };

    static const size_t kTxQueue = 4;
    TxJob txQueue_[kTxQueue];
    PanicPacket rxQueue_[NET_RX_QUEUE_DEPTH];
    size_t rxHead_;
    size_t rxTail_;
    bool ready_;
};
