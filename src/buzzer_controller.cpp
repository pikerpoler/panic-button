#include "buzzer_controller.h"

#include <Arduino.h>

void BuzzerController::begin(uint8_t pin) {
    pin_ = pin;
    pinMode(pin_, OUTPUT);
    digitalWrite(pin_, LOW);
    mode_ = BuzzerMode::Off;
    active_ = false;
    phaseMs_ = 0;
}

void BuzzerController::update(uint32_t nowMs, BuzzerMode mode, bool effectsActive) {
    if (!effectsActive || mode == BuzzerMode::Off) {
        digitalWrite(pin_, LOW);
        return;
    }

    if (mode != mode_ || !active_) {
        mode_ = mode;
        phaseMs_ = nowMs;
    }
    active_ = effectsActive;

    const uint32_t t = nowMs - phaseMs_;
    bool on = false;

    if (mode == BuzzerMode::PanicAlarm) {
        const uint32_t cycle = 950;
        const uint32_t x = t % cycle;
        on = (x < 150) || (x >= 300 && x < 450);
    } else if (mode == BuzzerMode::OverloadSad) {
        const uint32_t cycle = 2650;
        const uint32_t x = t % cycle;
        on = (x < 800) || (x >= 1400 && x < 1650);
    }

    digitalWrite(pin_, on ? HIGH : LOW);
}
