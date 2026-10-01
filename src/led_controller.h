#pragma once

#include "panic_types.h"

#include <stdint.h>

class LedController {
public:
    void begin(uint8_t pin1, uint8_t pin2);
    void update(uint32_t nowMs, LedMode mode, bool effectsActive);

private:
    uint8_t pin1_;
    uint8_t pin2_;
    LedMode mode_;
    bool active_;
    uint32_t phaseMs_;
    uint8_t chaseStep_;
};
