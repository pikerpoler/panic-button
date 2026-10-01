#pragma once

#include <stdint.h>

class ButtonInput {
public:
    void begin(uint8_t pin);
    bool update(uint32_t nowMs, bool rawPressed);

    bool consumePress();

private:
    uint8_t pin_;
    bool stableReleased_;
    bool lastRaw_;
    uint32_t lastChangeMs_;
    bool pendingPress_;
};
