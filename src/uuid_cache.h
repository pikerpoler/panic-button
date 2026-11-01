#pragma once

#include "config.h"

#include <stddef.h>
#include <stdint.h>

class UuidCache {
public:
    UuidCache();

    bool contains(const uint8_t* uuid) const;
    bool insert(const uint8_t* uuid);
    void clear();

    void onEnterLevel0(uint32_t nowMs);
    void onLeaveLevel0();
    void onNonNormalState();
    bool update(uint32_t nowMs);

private:
    uint8_t entries_[SEEN_UUID_CAPACITY][16];
    size_t head_;
    size_t count_;
    bool idleTimerActive_;
    uint32_t level0SinceMs_;
};
