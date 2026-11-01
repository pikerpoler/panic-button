# PANIC BUTTON

Firmware for the ESP8266 D1 Mini panic button (spec: [PANIC_BUTTON_V1.1.md](PANIC_BUTTON_V1.1.md)).

## Customize a unit

Edit only these strings in [src/device_config.h](src/device_config.h):

```cpp
constexpr char DEVICE_NAME[] = "SHLORP";
constexpr char DEVICE_NAME_SPEECH[] = "SHLORP";
```

## Prerequisites

- Python 3
- [PlatformIO](https://platformio.org/) (firmware build only):

```bash
pip install -r requirements.txt
```

Or with Homebrew: `brew install platformio`

## Build and upload

Plug in the D1 Mini, then print its serial port:

```bash
ls /dev/cu.usb*
```

Use the `cu.` path that prints, for example `/dev/cu.usbserial-A5069RR4`. With a single USB serial device connected, the shell can pass that path through:

```bash
python tools/build_device.py --port $(ls /dev/cu.usb*)
```

Or paste the name yourself:

```bash
python tools/build_device.py --port /dev/cu.usbserial-XXXX
```

If more than one USB serial device is plugged in, `ls` prints every match and the command above passes all of them. Copy the one path that belongs to this board.

Compile only:

```bash
python tools/build_device.py --no-upload
```

Speech clips are regenerated automatically before each PlatformIO build using the vendored SAM renderer in `tools/sam_renderer/`.

## Host tests

```bash
make -C test run
```



## PlatformIO

```bash
pio run -e d1_mini
# or: python -m platformio run -e d1_mini
```

If flash is tight, use `d1_mini_no_ota`.