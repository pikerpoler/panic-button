#include "led_controller.h"

#include <Arduino.h>

void LedController::begin(uint8_t pin1, uint8_t pin2) {
    pin1_ = pin1;
    pin2_ = pin2;
    pinMode(pin1_, OUTPUT);
    pinMode(pin2_, OUTPUT);
    digitalWrite(pin1_, LOW);
    digitalWrite(pin2_, LOW);
    mode_ = LedMode::Off;
    active_ = false;
    phaseMs_ = 0;
    chaseStep_ = 0;
}

void LedController::update(uint32_t nowMs, LedMode mode, bool effectsActive) {
    if (!effectsActive || mode == LedMode::Off) {
        digitalWrite(pin1_, LOW);
        digitalWrite(pin2_, LOW);
        return;
    }

    if (mode != mode_ || !active_) {
        mode_ = mode;
        phaseMs_ = nowMs;
        chaseStep_ = 0;
    }
    active_ = effectsActive;

    const uint32_t elapsed = nowMs - phaseMs_;

    if (mode == LedMode::BootChase) {
        const uint8_t step = static_cast<uint8_t>((elapsed / 200) % 4);
        if (step == 0) {
            digitalWrite(pin1_, HIGH);
            digitalWrite(pin2_, LOW);
        } else if (step == 1) {
            digitalWrite(pin1_, LOW);
            digitalWrite(pin2_, HIGH);
        } else if (step == 2) {
            digitalWrite(pin1_, HIGH);
            digitalWrite(pin2_, HIGH);
        } else {
            digitalWrite(pin1_, LOW);
            digitalWrite(pin2_, LOW);
        }
        return;
    }

    uint32_t period = 300;
    if (mode == LedMode::PanicFast) {
        period = 100;
    } else if (mode == LedMode::OverloadSlow) {
        period = 700;
    }

    const bool on = (elapsed % (period * 2)) < period;
    digitalWrite(pin1_, on ? HIGH : LOW);
    digitalWrite(pin2_, on ? HIGH : LOW);
}
