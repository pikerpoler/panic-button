#pragma once

#include "panic_types.h"

#include <stdint.h>

class DisplayController {
public:
    bool begin();
    void show(DeviceState state, uint8_t panicLevel, SessionLanguage language);
    void markDirty() { dirty_ = true; }

private:
    void drawEnglish(DeviceState state, uint8_t panicLevel);
    void drawMandarin(DeviceState state, uint8_t panicLevel);
    bool dirty_;
    DeviceState lastState_;
    uint8_t lastLevel_;
    SessionLanguage lastLang_;
};
