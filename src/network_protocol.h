#pragma once

#include "config.h"

#include <stdint.h>

struct PanicPacket {
    uint32_t magic;
    uint8_t protocolVersion;
    uint8_t messageType;
    uint16_t groupId;
    uint8_t uuid[16];
} __attribute__((packed));

static_assert(sizeof(PanicPacket) == 24, "PanicPacket must be 24 bytes");

inline bool panicPacketIsValid(const PanicPacket& pkt) {
    return pkt.magic == PANIC_PACKET_MAGIC && pkt.protocolVersion == PROTOCOL_VERSION &&
           pkt.messageType == MESSAGE_TYPE_PANIC_EVENT && pkt.groupId == PANIC_GROUP_ID;
}

inline bool uuidEqual(const uint8_t* a, const uint8_t* b) {
    for (int i = 0; i < 16; ++i) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

inline void uuidCopy(uint8_t* dst, const uint8_t* src) {
    for (int i = 0; i < 16; ++i) {
        dst[i] = src[i];
    }
}
