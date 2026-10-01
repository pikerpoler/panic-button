#pragma once

#include "panic_types.h"
#include "uuid_cache.h"

#include <stdint.h>

class PanicEngine {
public:
    PanicEngine();

    void beginBoot(uint32_t nowMs, bool mandarinGlitch);
    PanicOutputs startBootAfterLockout(uint32_t nowMs, bool mandarinGlitch);
    void setBootSpeechFinished(bool finished);
    void setOverloadSpeechFinished(bool finished);

    PanicOutputs tick(uint32_t nowMs);

    PanicOutputs onLocalPress(uint32_t nowMs, const uint8_t* newUuid);
    PanicOutputs onRemotePanic(uint32_t nowMs, const uint8_t* uuid, bool isNew);

    DeviceState state() const { return state_; }
    uint8_t panicLevel() const { return panicLevel_; }
    SessionLanguage language() const { return language_; }
    UuidCache& uuidCache() { return cache_; }
    const UuidCache& uuidCache() const { return cache_; }

    bool acceptsPanicInput() const;
    bool inBoot() const { return state_ == DeviceState::Boot; }

private:
    PanicOutputs makeOutputs();
    void applyLevelEffects(uint8_t newLevel, uint32_t nowMs, PanicOutputs& out);
    void enterLevel5(uint32_t nowMs, PanicOutputs& out);
    void enterLockout(uint32_t nowMs, PanicOutputs& out);
    void enterBoot(uint32_t nowMs, PanicOutputs& out);
    void finishBootToNormal(uint32_t nowMs, PanicOutputs& out);
    void updateDecay(uint32_t nowMs, PanicOutputs& out);
    void updateEffectWindow(uint32_t nowMs, PanicOutputs& out);
    void fillEffectOutputs(PanicOutputs& out) const;

    DeviceState state_;
    SessionLanguage language_;
    uint8_t panicLevel_;
    bool bootSpeechDone_;
    bool bootAbandoned_;

    uint32_t stateEnteredMs_;
    uint32_t bootStartedMs_;
    uint32_t lastPanicEventMs_;
    uint32_t nextDecayMs_;
    bool decayScheduled_;
    uint32_t effectWindowEndMs_;
    bool effectWindowActive_;

    uint32_t overloadStartedMs_;
    bool overloadSpeechDone_;

    UuidCache cache_;
};
