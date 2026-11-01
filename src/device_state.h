#pragma once

#include "audio_controller.h"
#include "button_input.h"
#include "buzzer_controller.h"
#include "display_controller.h"
#include "led_controller.h"
#include "network_controller.h"
#include "panic_engine.h"

class DeviceStateController {
public:
    void setup();
    void loop();

private:
    void applyOutputs(const PanicOutputs& out, uint32_t nowMs);
    void handleNetworkRx(uint32_t nowMs);
    void generateUuid(uint8_t* out);
    bool rollMandarinGlitch();
    void logBoot(bool mandarin) const;
    void logUuidEvent(const uint8_t* uuid) const;
    void trackPanicLevel(uint8_t newLevel);

    PanicEngine engine_;
    ButtonInput button_;
    LedController leds_;
    BuzzerController buzzer_;
    DisplayController display_;
    AudioController audio_;
    NetworkController network_;

    LedMode ledMode_;
    BuzzerMode buzzerMode_;
    bool ledActive_;
    bool buzzerActive_;

    bool displayOk_;
    bool audioOk_;
    bool networkOk_;
    bool lockoutBootHandled_;
    uint8_t lastPanicLevel_;
};
