#pragma once

#include <stdint.h>

class ButtonInput {
public:
    void begin(uint8_t pin);
    bool update(uint32_t nowMs, bool rawPressed);

    bool consumePress();

private:
    uint8_t pin_;
    bool stablePressed_;
    bool lastRawPressed_;
    uint32_t lastChangeMs_;
    bool pendingPress_;
    bool baselineSet_;
};
