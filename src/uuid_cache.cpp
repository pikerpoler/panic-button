#include "uuid_cache.h"

#include "network_protocol.h"

UuidCache::UuidCache() : head_(0), count_(0), idleTimerActive_(false), level0SinceMs_(0) {}

bool UuidCache::contains(const uint8_t* uuid) const {
    for (size_t i = 0; i < count_; ++i) {
        if (uuidEqual(entries_[i], uuid)) {
            return true;
        }
    }
    return false;
}

bool UuidCache::insert(const uint8_t* uuid) {
    if (contains(uuid)) {
        return false;
    }
    if (count_ < SEEN_UUID_CAPACITY) {
        uuidCopy(entries_[count_], uuid);
        ++count_;
        return true;
    }
    uuidCopy(entries_[head_], uuid);
    head_ = (head_ + 1) % SEEN_UUID_CAPACITY;
    return true;
}

void UuidCache::clear() {
    count_ = 0;
    head_ = 0;
    idleTimerActive_ = false;
}

void UuidCache::onEnterLevel0(uint32_t nowMs) {
    if (!idleTimerActive_) {
        idleTimerActive_ = true;
        level0SinceMs_ = nowMs;
    }
}

void UuidCache::onLeaveLevel0() {
    idleTimerActive_ = false;
}

void UuidCache::onNonNormalState() {
    idleTimerActive_ = false;
}

void UuidCache::update(uint32_t nowMs) {
    if (!idleTimerActive_ || count_ == 0) {
        return;
    }
    if (nowMs - level0SinceMs_ >= UUID_CACHE_CLEAR_IDLE_MS) {
        clear();
    }
}
