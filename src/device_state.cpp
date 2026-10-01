#include "device_state.h"

#include "config.h"

#include <Arduino.h>

void DeviceStateController::generateUuid(uint8_t* out) {
    for (int i = 0; i < 16; i += 4) {
        const uint32_t r = ESP.random();
        out[i] = static_cast<uint8_t>(r & 0xFF);
        out[i + 1] = static_cast<uint8_t>((r >> 8) & 0xFF);
        out[i + 2] = static_cast<uint8_t>((r >> 16) & 0xFF);
        out[i + 3] = static_cast<uint8_t>((r >> 24) & 0xFF);
    }
    out[6] = static_cast<uint8_t>((out[6] & 0x0F) | 0x40);
    out[8] = static_cast<uint8_t>((out[8] & 0x3F) | 0x80);
}

bool DeviceStateController::rollMandarinGlitch() {
    return (ESP.random() % 10) == 0;
}

void DeviceStateController::setup() {
    Serial.begin(115200);
    Serial.println();

    button_.begin(PIN_BUTTON);
    leds_.begin(PIN_LED_1, PIN_LED_2);
    buzzer_.begin(PIN_BUZZER);

    displayOk_ = display_.begin();
    audioOk_ = audio_.begin();
    networkOk_ = network_.begin(ESPNOW_CHANNEL);

    randomSeed(ESP.getCycleCount());
    if (networkOk_) {
        randomSeed(ESP.random());
    }

    lockoutBootHandled_ = false;
    ledMode_ = LedMode::BootChase;
    buzzerMode_ = BuzzerMode::Off;
    ledActive_ = true;
    buzzerActive_ = false;

    const bool mandarin = rollMandarinGlitch();
    applyOutputs(engine_.startBootAfterLockout(millis(), mandarin), millis());
    if (!audioOk_) {
        engine_.setBootSpeechFinished(true);
    } else {
        audio_.play(SpeechClip::Boot, engine_.language(), true);
    }
}

void DeviceStateController::applyOutputs(const PanicOutputs& out, uint32_t nowMs) {
    if (out.displayDirty && displayOk_) {
        display_.markDirty();
    }
    if (displayOk_) {
        display_.show(out.state, out.panicLevel, out.language);
    }

    if (out.restartSpeech && out.speech != SpeechClip::None && audioOk_) {
        audio_.play(out.speech, out.language, true);
    }

    ledMode_ = out.ledMode;
    ledActive_ = out.ledEffectsActive;
    buzzerMode_ = out.buzzerMode;
    buzzerActive_ = out.buzzerEffectsActive;

    if (out.scheduleTx && networkOk_) {
        network_.scheduleOrigin(out.txUuid, nowMs);
    }
    if (out.scheduleRelay && networkOk_) {
        network_.scheduleRelay(out.txUuid, nowMs);
    }
}

void DeviceStateController::handleNetworkRx(uint32_t nowMs) {
    PanicPacket pkt;
    while (network_.popReceived(pkt)) {
        const bool isNew = !engine_.uuidCache().contains(pkt.uuid);
        PanicOutputs out = engine_.onRemotePanic(nowMs, pkt.uuid, isNew);
        applyOutputs(out, nowMs);
    }
}

void DeviceStateController::loop() {
    const uint32_t nowMs = millis();

    if (networkOk_) {
        network_.update(nowMs, ESP.random());
        handleNetworkRx(nowMs);
    }

    button_.update(nowMs, digitalRead(PIN_BUTTON) == LOW);
    if (button_.consumePress()) {
        uint8_t uuid[16];
        generateUuid(uuid);
        PanicOutputs out = engine_.onLocalPress(nowMs, uuid);
        applyOutputs(out, nowMs);
    }

    if (audioOk_) {
        audio_.update();
        if (engine_.inBoot()) {
            engine_.setBootSpeechFinished(audio_.clipFinished(SpeechClip::Boot));
        }
        if (engine_.state() == DeviceState::Level5Sequence) {
            engine_.setOverloadSpeechFinished(audio_.clipFinished(SpeechClip::Overload));
        }
    } else {
        engine_.setBootSpeechFinished(true);
        engine_.setOverloadSpeechFinished(true);
    }

    PanicOutputs tickOut = engine_.tick(nowMs);
    if (tickOut.requestPostLockoutBoot && !lockoutBootHandled_) {
        lockoutBootHandled_ = true;
        const bool mandarin = rollMandarinGlitch();
        PanicOutputs bootOut = engine_.startBootAfterLockout(nowMs, mandarin);
        applyOutputs(bootOut, nowMs);
        if (audioOk_) {
            audio_.play(SpeechClip::Boot, engine_.language(), true);
        }
    }
    if (engine_.state() != DeviceState::Lockout) {
        lockoutBootHandled_ = false;
    }

    applyOutputs(tickOut, nowMs);

    leds_.update(nowMs, ledMode_, ledActive_);
    buzzer_.update(nowMs, buzzerMode_, buzzerActive_);

    yield();
}
