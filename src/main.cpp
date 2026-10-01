#include <Arduino.h>

#include "device_state.h"

static DeviceStateController gDevice;

void setup() {
    gDevice.setup();
}

void loop() {
    gDevice.loop();
}
