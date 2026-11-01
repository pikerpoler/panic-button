#include "panic_engine.h"

#include "config.h"
#include "network_protocol.h"

PanicEngine::PanicEngine()
    : state_(DeviceState::Boot),
      language_(SessionLanguage::English),
      panicLevel_(0),
      bootSpeechDone_(false),
      bootAbandoned_(false),
      stateEnteredMs_(0),
      bootStartedMs_(0),
      lastPanicEventMs_(0),
      nextDecayMs_(0),
      decayScheduled_(false),
      effectWindowEndMs_(0),
      effectWindowActive_(false),
      overloadStartedMs_(0),
      overloadSpeechDone_(false) {}

PanicOutputs PanicEngine::startBootAfterLockout(uint32_t nowMs, bool mandarinGlitch) {
    beginBoot(nowMs, mandarinGlitch);
    PanicOutputs out = makeOutputs();
    out.displayDirty = true;
    out.speech = SpeechClip::Boot;
    out.restartSpeech = true;
    out.ledMode = LedMode::BootChase;
    out.ledEffectsActive = true;
    fillEffectOutputs(out);
    return out;
}

void PanicEngine::beginBoot(uint32_t nowMs, bool mandarinGlitch) {
    state_ = DeviceState::Boot;
    language_ = mandarinGlitch ? SessionLanguage::MandarinGlitch : SessionLanguage::English;
    panicLevel_ = 0;
    bootSpeechDone_ = false;
    bootAbandoned_ = false;
    bootStartedMs_ = nowMs;
    stateEnteredMs_ = nowMs;
    decayScheduled_ = false;
    effectWindowActive_ = false;
    overloadSpeechDone_ = false;
    cache_.onNonNormalState();
}

void PanicEngine::setBootSpeechFinished(bool finished) {
    bootSpeechDone_ = finished;
}

void PanicEngine::setOverloadSpeechFinished(bool finished) {
    overloadSpeechDone_ = finished;
}

bool PanicEngine::acceptsPanicInput() const {
    return state_ == DeviceState::Boot || state_ == DeviceState::Normal;
}

PanicOutputs PanicEngine::makeOutputs() {
    PanicOutputs o{};
    o.state = state_;
    o.panicLevel = panicLevel_;
    o.language = language_;
    o.displayDirty = false;
    o.speech = SpeechClip::None;
    o.restartSpeech = false;
    o.ledMode = LedMode::Off;
    o.ledEffectsActive = false;
    o.buzzerMode = BuzzerMode::Off;
    o.buzzerEffectsActive = false;
    o.scheduleTx = false;
    o.scheduleRelay = false;
    o.bootAbandoned = false;
    o.requestPostLockoutBoot = false;
    return o;
}

void PanicEngine::applyLevelEffects(uint8_t newLevel, uint32_t nowMs, PanicOutputs& out) {
    panicLevel_ = newLevel;
    out.panicLevel = panicLevel_;
    out.displayDirty = true;
    lastPanicEventMs_ = nowMs;
    decayScheduled_ = true;
    nextDecayMs_ = nowMs + DECAY_GRACE_MS;
    cache_.onLeaveLevel0();

    if (newLevel >= 1 && newLevel <= 4) {
        state_ = DeviceState::Normal;
        out.state = state_;
        out.speech = SpeechClip::Panic;
        out.restartSpeech = true;
        out.ledMode = (newLevel <= 2) ? LedMode::PanicNormal : LedMode::PanicFast;
        out.ledEffectsActive = true;
        out.buzzerMode = BuzzerMode::PanicAlarm;
        out.buzzerEffectsActive = true;
        effectWindowActive_ = true;
        effectWindowEndMs_ = nowMs + PANIC_PROGRAM_MS;
    } else if (newLevel >= 5) {
        enterLevel5(nowMs, out);
    }
}

void PanicEngine::enterLevel5(uint32_t nowMs, PanicOutputs& out) {
    panicLevel_ = 5;
    state_ = DeviceState::Level5Sequence;
    stateEnteredMs_ = nowMs;
    overloadStartedMs_ = nowMs;
    overloadSpeechDone_ = false;
    effectWindowActive_ = false;
    decayScheduled_ = false;
    cache_.onNonNormalState();

    out.state = state_;
    out.panicLevel = panicLevel_;
    out.displayDirty = true;
    out.speech = SpeechClip::Overload;
    out.restartSpeech = true;
    out.ledMode = LedMode::OverloadSlow;
    out.ledEffectsActive = true;
    out.buzzerMode = BuzzerMode::OverloadSad;
    out.buzzerEffectsActive = true;
    out.bootAbandoned = bootAbandoned_;
}

void PanicEngine::enterLockout(uint32_t nowMs, PanicOutputs& out) {
    state_ = DeviceState::Lockout;
    stateEnteredMs_ = nowMs;
    effectWindowActive_ = false;
    cache_.onNonNormalState();

    out.state = state_;
    out.displayDirty = true;
    out.speech = SpeechClip::None;
    out.ledMode = LedMode::Off;
    out.ledEffectsActive = false;
    out.buzzerMode = BuzzerMode::Off;
    out.buzzerEffectsActive = false;
}

void PanicEngine::enterBoot(uint32_t nowMs, PanicOutputs& out) {
    beginBoot(nowMs, false);
    out = makeOutputs();
    out.displayDirty = true;
    out.speech = SpeechClip::Boot;
    out.restartSpeech = true;
    out.ledMode = LedMode::BootChase;
    out.ledEffectsActive = true;
}

void PanicEngine::finishBootToNormal(uint32_t nowMs, PanicOutputs& out) {
    state_ = DeviceState::Normal;
    panicLevel_ = 0;
    stateEnteredMs_ = nowMs;
    bootAbandoned_ = false;
    effectWindowActive_ = false;
    cache_.onEnterLevel0(nowMs);

    out.state = state_;
    out.panicLevel = 0;
    out.displayDirty = true;
    out.ledMode = LedMode::Off;
    out.ledEffectsActive = false;
}

void PanicEngine::updateDecay(uint32_t nowMs, PanicOutputs& out) {
    if (state_ != DeviceState::Normal || panicLevel_ == 0 || !decayScheduled_) {
        return;
    }
    if (nowMs < nextDecayMs_) {
        return;
    }
    if (panicLevel_ > 0) {
        --panicLevel_;
        out.panicLevel = panicLevel_;
        out.displayDirty = true;
        if (panicLevel_ == 0) {
            decayScheduled_ = false;
            cache_.onEnterLevel0(nowMs);
        } else {
            nextDecayMs_ = nowMs + DECAY_STEP_MS;
        }
    }
}

void PanicEngine::updateEffectWindow(uint32_t nowMs, PanicOutputs&) {
    if (effectWindowActive_ && nowMs >= effectWindowEndMs_) {
        effectWindowActive_ = false;
    }
}

void PanicEngine::fillEffectOutputs(PanicOutputs& out) const {
    if (state_ == DeviceState::Boot && !bootAbandoned_) {
        out.ledMode = LedMode::BootChase;
        out.ledEffectsActive = true;
        out.buzzerMode = BuzzerMode::Off;
        out.buzzerEffectsActive = false;
        return;
    }
    if (state_ == DeviceState::Level5Sequence) {
        out.ledMode = LedMode::OverloadSlow;
        out.ledEffectsActive = true;
        out.buzzerMode = BuzzerMode::OverloadSad;
        out.buzzerEffectsActive = true;
        return;
    }
    if (state_ == DeviceState::Normal && effectWindowActive_ && panicLevel_ >= 1 && panicLevel_ <= 4) {
        out.ledMode = (panicLevel_ <= 2) ? LedMode::PanicNormal : LedMode::PanicFast;
        out.ledEffectsActive = true;
        out.buzzerMode = BuzzerMode::PanicAlarm;
        out.buzzerEffectsActive = true;
        return;
    }
    out.ledMode = LedMode::Off;
    out.ledEffectsActive = false;
    out.buzzerMode = BuzzerMode::Off;
    out.buzzerEffectsActive = false;
}

PanicOutputs PanicEngine::tick(uint32_t nowMs) {
    PanicOutputs out = makeOutputs();

    if (state_ == DeviceState::Boot && !bootAbandoned_) {
        out.speech = SpeechClip::Boot;
        const bool minBoot = (nowMs - bootStartedMs_) >= BOOT_MIN_MS;
        if (minBoot && bootSpeechDone_) {
            finishBootToNormal(nowMs, out);
        }
    } else if (state_ == DeviceState::Level5Sequence) {
        out.speech = SpeechClip::Overload;
        const bool minSeq = (nowMs - overloadStartedMs_) >= LEVEL5_SEQUENCE_MS;
        if (minSeq && overloadSpeechDone_) {
            enterLockout(nowMs, out);
        }
    } else if (state_ == DeviceState::Lockout) {
        if (nowMs - stateEnteredMs_ >= LOCKOUT_MS) {
            out.requestPostLockoutBoot = true;
        }
    }

    updateDecay(nowMs, out);
    updateEffectWindow(nowMs, out);

    out.state = state_;
    out.panicLevel = panicLevel_;
    out.language = language_;
    fillEffectOutputs(out);
    return out;
}

PanicOutputs PanicEngine::onLocalPress(uint32_t nowMs, const uint8_t* newUuid) {
    PanicOutputs out = makeOutputs();
    if (!acceptsPanicInput()) {
        return out;
    }

    cache_.insert(newUuid);
    uuidCopy(out.txUuid, newUuid);
    out.scheduleTx = true;

    if (state_ == DeviceState::Boot) {
        bootAbandoned_ = true;
        out.bootAbandoned = true;
    }

    const uint8_t newLevel = (panicLevel_ < 5) ? static_cast<uint8_t>(panicLevel_ + 1) : 5;
    applyLevelEffects(newLevel, nowMs, out);
    fillEffectOutputs(out);
    return out;
}

PanicOutputs PanicEngine::onRemotePanic(uint32_t nowMs, const uint8_t* uuid, bool isNew) {
    PanicOutputs out = makeOutputs();
    if (!acceptsPanicInput()) {
        return out;
    }
    if (!isNew) {
        return out;
    }

    cache_.insert(uuid);
    uuidCopy(out.txUuid, uuid);
    out.scheduleRelay = true;

    if (state_ == DeviceState::Boot) {
        bootAbandoned_ = true;
        out.bootAbandoned = true;
    }

    const uint8_t newLevel = (panicLevel_ < 5) ? static_cast<uint8_t>(panicLevel_ + 1) : 5;
    applyLevelEffects(newLevel, nowMs, out);
    fillEffectOutputs(out);
    return out;
}
