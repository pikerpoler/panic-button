#pragma once

#include <stddef.h>
#include <stdint.h>

constexpr uint8_t PIN_BUTTON = 14;  // D5
constexpr uint8_t PIN_BUZZER = 12;  // D6
constexpr uint8_t PIN_LED_1 = 13;   // D7
constexpr uint8_t PIN_LED_2 = 16;   // D0

constexpr uint8_t ESPNOW_CHANNEL = 6;

constexpr uint32_t PANIC_PACKET_MAGIC = 0x50414E43u;  // "PANC"
constexpr uint8_t PROTOCOL_VERSION = 1;
constexpr uint8_t MESSAGE_TYPE_PANIC_EVENT = 1;
constexpr uint16_t PANIC_GROUP_ID = 0x2026;

constexpr uint32_t BUTTON_DEBOUNCE_MS = 40;
constexpr uint32_t DECAY_GRACE_MS = 15000;
constexpr uint32_t DECAY_STEP_MS = 10000;
constexpr uint32_t BOOT_MIN_MS = 4000;
constexpr uint32_t PANIC_PROGRAM_MS = 4000;
constexpr uint32_t LEVEL5_SEQUENCE_MS = 6000;
constexpr uint32_t LOCKOUT_MS = 60000;
constexpr uint32_t UUID_CACHE_CLEAR_IDLE_MS = 600000;

constexpr uint8_t TX_REPEAT_COUNT = 2;
constexpr uint32_t TX_REPEAT_DELAY_MS = 40;
constexpr uint32_t TX_RELAY_JITTER_MIN_MS = 10;
constexpr uint32_t TX_RELAY_JITTER_MAX_MS = 40;

constexpr uint32_t SEEN_UUID_CAPACITY = 128;

constexpr uint8_t MANDARIN_GLITCH_PERCENT = 10;

constexpr uint32_t NET_RX_QUEUE_DEPTH = 8;
