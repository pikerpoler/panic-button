#pragma once

#include "panic_types.h"

#include <stdint.h>

class BuzzerController {
public:
    void begin(uint8_t pin);
    void update(uint32_t nowMs, BuzzerMode mode, bool effectsActive);

private:
    uint8_t pin_;
    BuzzerMode mode_;
    bool active_;
    uint32_t phaseMs_;
};
