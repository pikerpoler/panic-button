# PANIC BUTTON

Firmware for the ESP8266 D1 Mini panic button (spec: [PANIC_BUTTON_V1.1.md](PANIC_BUTTON_V1.1.md)).

## Customize a unit

Edit only these strings in [src/device_config.h](src/device_config.h):

```cpp
constexpr char DEVICE_NAME[] = "SHLORP";
constexpr char DEVICE_NAME_SPEECH[] = "SHLORP";
```

## Build and upload

```bash
python tools/build_device.py --port /dev/cu.usbserial-XXXX
```

Compile only:

```bash
python tools/build_device.py --no-upload
```

Speech clips are regenerated automatically before each PlatformIO build. Optional SAM-style renderer:

```bash
make -C tools/sam_renderer
```

## Host tests

```bash
make -C test run
```

## PlatformIO

```bash
pio run -e d1_mini
```

If flash is tight, use `d1_mini_no_ota`.
