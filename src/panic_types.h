#pragma once

#include <stdint.h>

enum class DeviceState : uint8_t {
    Boot = 0,
    Normal = 1,
    Level5Sequence = 2,
    Lockout = 3,
};

enum class SessionLanguage : uint8_t {
    English = 0,
    MandarinGlitch = 1,
};

enum class SpeechClip : uint8_t {
    None = 0,
    Boot,
    Panic,
    Overload,
};

enum class LedMode : uint8_t {
    Off = 0,
    BootChase,
    PanicNormal,
    PanicFast,
    OverloadSlow,
};

enum class BuzzerMode : uint8_t {
    Off = 0,
    PanicAlarm,
    OverloadSad,
};

struct PanicOutputs {
    DeviceState state;
    uint8_t panicLevel;
    SessionLanguage language;
    bool displayDirty;
    SpeechClip speech;
    bool restartSpeech;
    LedMode ledMode;
    bool ledEffectsActive;
    BuzzerMode buzzerMode;
    bool buzzerEffectsActive;
    bool scheduleTx;
    bool scheduleRelay;
    uint8_t txUuid[16];
    bool bootAbandoned;
    bool requestPostLockoutBoot;
};
