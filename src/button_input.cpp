#include "button_input.h"

#include "config.h"

#include <Arduino.h>

void ButtonInput::begin(uint8_t pin) {
    pin_ = pin;
    pinMode(pin_, INPUT_PULLUP);
    stablePressed_ = false;
    lastRawPressed_ = false;
    lastChangeMs_ = 0;
    pendingPress_ = false;
    baselineSet_ = false;
}

bool ButtonInput::update(uint32_t nowMs, bool rawPressed) {
    if (!baselineSet_) {
        lastRawPressed_ = rawPressed;
        stablePressed_ = rawPressed;
        lastChangeMs_ = nowMs;
        baselineSet_ = true;
        return pendingPress_;
    }

    if (rawPressed != lastRawPressed_) {
        lastRawPressed_ = rawPressed;
        lastChangeMs_ = nowMs;
    }

    if ((nowMs - lastChangeMs_) >= BUTTON_DEBOUNCE_MS) {
        if (rawPressed && !stablePressed_) {
            pendingPress_ = true;
        }
        stablePressed_ = rawPressed;
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
