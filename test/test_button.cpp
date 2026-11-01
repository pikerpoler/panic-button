#include "button_input.h"

#include "../src/config.h"

#include <cassert>
#include <cstdio>

static void test_no_phantom_press_while_released() {
    ButtonInput button;
    button.begin(14);
    for (uint32_t t = 0; t < 200; t += 10) {
        button.update(t, false);
        assert(!button.consumePress());
    }
}

static void test_press_on_falling_edge_after_debounce() {
    ButtonInput button;
    button.begin(14);
    const uint32_t pressAt = 10;
    button.update(0, false);
    button.update(pressAt, true);
    for (uint32_t t = pressAt + 10; t < pressAt + BUTTON_DEBOUNCE_MS; t += 10) {
        button.update(t, true);
        assert(!button.consumePress());
    }
    button.update(pressAt + BUTTON_DEBOUNCE_MS, true);
    assert(button.consumePress());
    assert(!button.consumePress());
}

static void test_no_repeat_while_held() {
    ButtonInput button;
    button.begin(14);
    const uint32_t pressAt = 10;
    button.update(0, false);
    button.update(pressAt, true);
    button.update(pressAt + BUTTON_DEBOUNCE_MS, true);
    assert(button.consumePress());
    for (uint32_t t = BUTTON_DEBOUNCE_MS + 10; t < 500; t += 10) {
        button.update(t, true);
        assert(!button.consumePress());
    }
}

static void test_new_press_requires_release() {
    ButtonInput button;
    button.begin(14);
    const uint32_t pressAt = 10;
    button.update(0, false);
    button.update(pressAt, true);
    button.update(pressAt + BUTTON_DEBOUNCE_MS, true);
    assert(button.consumePress());

    const uint32_t releaseAt = 200;
    button.update(releaseAt, false);
    button.update(releaseAt + BUTTON_DEBOUNCE_MS, false);
    const uint32_t press2At = 300;
    button.update(press2At, true);
    button.update(press2At + BUTTON_DEBOUNCE_MS, true);
    assert(button.consumePress());
}

static void test_held_at_boot_does_not_count_until_release() {
    ButtonInput button;
    button.begin(14);
    button.update(0, true);
    for (uint32_t t = 10; t < 500; t += 10) {
        button.update(t, true);
        assert(!button.consumePress());
    }
    button.update(500, false);
    button.update(500 + BUTTON_DEBOUNCE_MS, false);
    button.update(600, true);
    button.update(600 + BUTTON_DEBOUNCE_MS, true);
    assert(button.consumePress());
}

int main() {
    test_no_phantom_press_while_released();
    test_press_on_falling_edge_after_debounce();
    test_no_repeat_while_held();
    test_new_press_requires_release();
    test_held_at_boot_does_not_count_until_release();
    std::printf("all button tests passed\n");
    return 0;
}
