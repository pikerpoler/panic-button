#include "button_input.h"

#include "config.h"

#include <Arduino.h>

void ButtonInput::begin(uint8_t pin) {
    pin_ = pin;
    pinMode(pin_, INPUT_PULLUP);
    stableReleased_ = true;
    lastRaw_ = true;
    lastChangeMs_ = 0;
    pendingPress_ = false;
}

bool ButtonInput::update(uint32_t nowMs, bool rawPressed) {
    if (rawPressed != lastRaw_) {
        lastRaw_ = rawPressed;
        lastChangeMs_ = nowMs;
    }
    if ((nowMs - lastChangeMs_) >= BUTTON_DEBOUNCE_MS) {
        const bool pressed = !lastRaw_;
        if (pressed && stableReleased_) {
            pendingPress_ = true;
            stableReleased_ = false;
        }
        if (!pressed) {
            stableReleased_ = true;
        }
    }
    return pendingPress_;
}

bool ButtonInput::consumePress() {
    if (!pendingPress_) {
        return false;
    }
    pendingPress_ = false;
    return true;
}
