#include "panic_engine.h"

#include "config.h"
#include "network_protocol.h"

#include <cassert>
#include <cstdio>
#include <cstring>

static void test_uuid_cache_ring() {
    UuidCache cache;
    uint8_t u1[16] = {1};
    uint8_t u2[16] = {2};
    assert(cache.insert(u1));
    assert(cache.contains(u1));
    assert(!cache.contains(u2));
    assert(cache.insert(u2));
    assert(cache.insert(u1) == false);
}

static void test_one_press_one_increment_mesh() {
    PanicEngine a;
    PanicEngine b;
    a.beginBoot(0, false);
    b.beginBoot(0, false);

    uint8_t uuid[16] = {9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0};
    PanicOutputs local = a.onLocalPress(100, uuid);
    assert(local.scheduleTx);
    assert(a.panicLevel() == 1);

    PanicOutputs remote = b.onRemotePanic(120, uuid, true);
    assert(remote.scheduleRelay);
    assert(b.panicLevel() == 1);

    PanicOutputs echo = a.onRemotePanic(140, uuid, false);
    assert(!echo.scheduleRelay);
    assert(a.panicLevel() == 1);
}

static void test_boot_interrupt() {
    PanicEngine e;
    e.beginBoot(0, false);
    uint8_t uuid[16] = {3};
    PanicOutputs out = e.onLocalPress(500, uuid);
    assert(out.bootAbandoned);
    assert(e.panicLevel() == 1);
    assert(e.state() == DeviceState::Normal);
}

static void test_decay_silent() {
    PanicEngine e;
    e.beginBoot(0, false);
    e.setBootSpeechFinished(true);
    PanicOutputs tick = e.tick(5000);
    assert(tick.panicLevel == 0);

    uint8_t uuid[16] = {4};
    e.onLocalPress(6000, uuid);
    assert(e.panicLevel() == 1);
    PanicOutputs decay = e.tick(6000 + DECAY_GRACE_MS + 1);
    assert(decay.panicLevel == 0);
    assert(!decay.restartSpeech);
}

static void test_overload_ignores_input() {
    PanicEngine e;
    e.beginBoot(0, false);
    for (int i = 0; i < 5; ++i) {
        uint8_t uuid[16] = {static_cast<uint8_t>(i + 10)};
        e.onLocalPress(1000 + i * 10, uuid);
    }
    assert(e.state() == DeviceState::Level5Sequence);
    uint8_t extra[16] = {99};
    PanicOutputs ignored = e.onLocalPress(2000, extra);
    assert(!ignored.scheduleTx);
    assert(e.panicLevel() == 5);
}

static void test_effect_window_4s() {
    PanicEngine e;
    e.beginBoot(0, false);
    e.setBootSpeechFinished(true);
    e.tick(5000);

    uint8_t uuid[16] = {7};
    e.onLocalPress(6000, uuid);
    PanicOutputs during = e.tick(7000);
    assert(during.ledEffectsActive);
    PanicOutputs after = e.tick(6000 + PANIC_PROGRAM_MS + 1);
    assert(!after.ledEffectsActive);
}

int main() {
    test_uuid_cache_ring();
    test_one_press_one_increment_mesh();
    test_boot_interrupt();
    test_decay_silent();
    test_overload_ignores_input();
    test_effect_window_4s();
    std::printf("all tests passed\n");
    return 0;
}
