#include "network_controller.h"

#include "config.h"

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <espnow.h>
#include <string.h>
#include <user_interface.h>

static NetworkController* gNet = nullptr;

static void onEspNowRecv(uint8_t* mac, uint8_t* data, uint8_t len) {
    if (!gNet || len != sizeof(PanicPacket)) {
        return;
    }
    PanicPacket pkt;
    memcpy(&pkt, data, sizeof(pkt));
    if (!panicPacketIsValid(pkt)) {
        return;
    }
    gNet->pushReceived(pkt);
}

void NetworkController::pushReceived(const PanicPacket& pkt) {
    size_t next = (rxHead_ + 1) % NET_RX_QUEUE_DEPTH;
    if (next == rxTail_) {
        return;
    }
    rxQueue_[rxHead_] = pkt;
    rxHead_ = next;
}

bool NetworkController::popReceived(PanicPacket& out) {
    if (rxTail_ == rxHead_) {
        return false;
    }
    out = rxQueue_[rxTail_];
    rxTail_ = (rxTail_ + 1) % NET_RX_QUEUE_DEPTH;
    return true;
}

bool NetworkController::begin(uint8_t channel) {
    gNet = this;
    rxHead_ = 0;
    rxTail_ = 0;
    for (size_t i = 0; i < kTxQueue; ++i) {
        txQueue_[i].active = false;
    }

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    wifi_set_channel(channel);

    if (esp_now_init() != 0) {
        ready_ = false;
        return false;
    }
    esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
    esp_now_register_recv_cb(onEspNowRecv);

    uint8_t broadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    esp_now_add_peer(broadcast, ESP_NOW_ROLE_CONTROLLER, channel, nullptr, 0);

    ready_ = true;
    return true;
}

bool NetworkController::enqueueTx(const uint8_t* uuid, bool isRelay, uint32_t nowMs, uint32_t randomWord) {
    for (size_t i = 0; i < kTxQueue; ++i) {
        if (!txQueue_[i].active) {
            uuidCopy(txQueue_[i].uuid, uuid);
            txQueue_[i].sendsLeft = TX_REPEAT_COUNT;
            uint32_t jitter = 0;
            if (isRelay) {
                const uint32_t span = TX_RELAY_JITTER_MAX_MS - TX_RELAY_JITTER_MIN_MS + 1;
                jitter = TX_RELAY_JITTER_MIN_MS + (randomWord % span);
            }
            txQueue_[i].nextSendMs = nowMs + jitter;
            txQueue_[i].active = true;
            return true;
        }
    }
    return false;
}

void NetworkController::scheduleOrigin(const uint8_t* uuid, uint32_t nowMs) {
    enqueueTx(uuid, false, nowMs, ESP.random());
}

void NetworkController::scheduleRelay(const uint8_t* uuid, uint32_t nowMs) {
    enqueueTx(uuid, true, nowMs, ESP.random());
}

void NetworkController::pumpTransmit(uint32_t nowMs) {
    if (!ready_) {
        return;
    }
    for (size_t i = 0; i < kTxQueue; ++i) {
        TxJob& job = txQueue_[i];
        if (!job.active || nowMs < job.nextSendMs) {
            continue;
        }
        PanicPacket pkt{};
        pkt.magic = PANIC_PACKET_MAGIC;
        pkt.protocolVersion = PROTOCOL_VERSION;
        pkt.messageType = MESSAGE_TYPE_PANIC_EVENT;
        pkt.groupId = PANIC_GROUP_ID;
        uuidCopy(pkt.uuid, job.uuid);

        uint8_t broadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
        esp_now_send(broadcast, reinterpret_cast<uint8_t*>(&pkt), sizeof(pkt));

        --job.sendsLeft;
        if (job.sendsLeft == 0) {
            job.active = false;
        } else {
            job.nextSendMs = nowMs + TX_REPEAT_DELAY_MS;
        }
    }
}

void NetworkController::update(uint32_t nowMs, uint32_t randomWord) {
    pumpTransmit(nowMs);
}
