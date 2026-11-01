#pragma once

#if PANIC_DEBUG
#include <Arduino.h>

#define PANIC_LOG(fmt, ...) Serial.printf(fmt "\n", ##__VA_ARGS__)
#else
#define PANIC_LOG(fmt, ...) ((void)0)
#endif
