# PANIC BUTTON — Project Specification V1.1

## 0. Purpose of this document

This is the implementation specification for V1.1 of the PANIC BUTTON.

V1.1 combines:

- The panic-level behavior from v0.2.
- The multi-device ESP-NOW flooding/UUID mesh from v1.0.
- ESP8266 D1 Mini hardware.
- MAX98357A I2S audio.
- Device-specific names.
- A 10% per-boot "Mandarin localization glitch" that persists for the entire boot session.
- Build-time SAM-style robotic speech generation stored directly in ESP8266 flash.

This document is the complete behavior specification. v0.2 and v1.0 are historical context and are not required to implement V1.1. Follow this file where an older description would disagree with it.

The intended user experience remains extremely simple:

```text
PRESS THE RED BUTTON.
```

All complexity belongs in the hardware and firmware.

---



# 1. Final hardware platform



## 1.1 Microcontroller

Use:

```text
ESP8266 D1 Mini
ESP-12F / ESP8266EX
4 MB flash
80/160 MHz
3.3 V GPIO
USB programming/power connector
```

Preferred shopping search:

```text
D1 Mini ESP8266 ESP-12F 4MB CH340 Micro USB
```

or, if USB-C is preferred:

```text
D1 Mini ESP8266 ESP-12F 4MB CH340 USB-C
```

All four units should preferably use the same board/connector version.

### Do NOT buy

Do not buy these as substitutes without revising the spec:

```text
ESP-01
D1 Mini Lite 1 MB
ESP32-C3
ESP32-S3
NodeMCU boards with a different physical layout unless pin assignments are rechecked
```

A genuine/compatible D1 Mini with 4 MB flash is the reference target.

---



# 2. Audio architecture



## 2.1 Audio amplifier

Use one:

```text
MAX98357A I2S mono Class-D amplifier breakout
```

Required electrical characteristics:

```text
Supply: 2.5 V to 5.5 V
Recommended project supply: 5 V
Output: approximately 3 W into 4 ohm speaker at 5 V
Input: I2S digital audio
No MCLK required
Mono output
```

Preferred shopping search:

```text
MAX98357A I2S 3W Class D amplifier module
```

The generic low-cost breakout is acceptable.

## 2.2 Speaker

Use:

```text
4 ohm
3 watt
40 mm round speaker
```

Preferred shopping search:

```text
40mm 4ohm 3W speaker
```

An 8-ohm speaker will work but will be quieter. Prefer 4 ohm / 3 W.

### Important speaker wiring rule

The MAX98357A speaker output is bridge-tied.

Connect:

```text
MAX98357A SPK+ -> speaker +
MAX98357A SPK- -> speaker -
```

Do NOT connect either speaker terminal to GND.

---



# 3. Speech strategy



## 3.1 Do not run SAM synchronously during normal runtime

ESP8266SAM's `Say()` interface is synchronous.

The PANIC BUTTON must keep:

- LEDs animating,
- buzzer patterns running,
- ESP-NOW receiving and relaying,
- display updates,
- state timers,

while speech is playing.

Therefore V1.1 uses:

```text
SAM-style speech generated at BUILD TIME
-> WAV/PCM bytes
-> stored in PROGMEM
-> played non-blockingly through ESP8266Audio
-> MAX98357A
-> speaker
```

No SD card is used.

No DFPlayer is used.

No Internet is required by the finished device.

## 3.2 Audio format

Recommended generated asset format:

```text
WAV
mono
8-bit PCM
22050 Hz
```

This matches the natural output rate of ESP8266SAM/SAM-style synthesis and keeps assets small.

All audio files are converted to byte arrays and compiled into firmware.

Example generated assets:

```text
speech_boot_en[]
speech_panic_en[]
speech_overload_en[]

speech_boot_zh[]
speech_panic_zh[]
speech_overload_zh[]
```



## 3.3 Playback implementation

Use ESP8266Audio:

```text
AudioFileSourcePROGMEM
AudioGeneratorWAV
AudioOutputI2S
```

Runtime playback must be serviced cooperatively.

Conceptually:

```cpp
void updateAudio() {
    if (wav && wav->isRunning()) {
        wav->loop();
    }
}
```

No audio playback routine may block the main loop for the full duration of a phrase.

---



# 4. Per-device naming

Every device has a unique name.

Examples:

```text
SHLORP
BUZZ
GAZORP
BLORBO
```

The user must only need to edit two strings for each device.

File:

```text
device_config.h
```

Required configuration:

```cpp
constexpr char DEVICE_NAME[] = "SHLORP";
constexpr char DEVICE_NAME_SPEECH[] = "SHLORP";
```

For a name SAM pronounces poorly:

```cpp
constexpr char DEVICE_NAME[] = "GAZORP";
constexpr char DEVICE_NAME_SPEECH[] = "GAH ZORP";
```

`DEVICE_NAME` is used on the OLED.

`DEVICE_NAME_SPEECH` is used when generating speech assets.

No runtime menu, EEPROM configuration, Wi-Fi configuration page, or additional setup button is required.

---



# 5. Build workflow for names and speech

The repository must include a helper tool that regenerates speech assets from `device_config.h`.

Preferred workflow:

```text
1. Edit:
   DEVICE_NAME
   DEVICE_NAME_SPEECH

2. Run one build/upload command.

3. Build tool:
   - reads device_config.h
   - generates the six speech clips
   - converts them into PROGMEM byte arrays
   - compiles firmware
   - uploads firmware
```

Suggested command:

```text
python tools/build_device.py --port <serial-port>
```

The implementation agent may use PlatformIO or Arduino CLI underneath.

The user's intended workflow should remain:

```text
change two strings
build/upload
done
```

Generated audio files must not need manual editing.

---



# 6. Normal English session speech



## Boot

Displayed:

```text
HELLO WORLD
SHLORP
```

Spoken:

```text
BOOTING UP SYSTEMS.
I AM SHLORP.
NO THREATS FOUND.
```

`SHLORP` is replaced with the configured device speech name.

## Levels 1–4

Spoken:

```text
PANIC!
PANIC!
PANIC!
```



## Level 5

Displayed:

```text
PANIC
OVERLOAD
```

Spoken:

```text
PANIC OVERLOAD.
SHLORP IS SCARED.
SHUTTING DOWN SYSTEM.
```

---



# 7. Mandarin localization-glitch session



## 7.1 Selection probability

Every time the device enters the BOOT state:

```text
10% chance -> MANDARIN_GLITCH
90% chance -> ENGLISH
```

This applies to:

- initial power-on boot,
- automatic boot after level-5 lockout.

The selected mode persists for the ENTIRE session until the next BOOT.

Do not reroll when panic level changes.

Do not reroll when panic decays.

## 7.2 Random source

Initialize Wi-Fi/ESP-NOW before making the mode roll.

Use the ESP8266 random source, for example:

```cpp
ESP.random()
```

Conceptually:

```cpp
sessionLanguage =
    (ESP.random() % 10 == 0)
        ? SessionLanguage::MANDARIN_GLITCH
        : SessionLanguage::ENGLISH;
```

This probability is exactly:

```text
1 in 10 per BOOT
```

---



# 8. Mandarin-glitch display and speech table

The alternate mode must feel internally consistent.

It must NOT use random Unicode characters.

It uses a curated fixed set of real Chinese text and a matching SAM-friendly pronunciation string.

The device name remains in Latin characters.

## 8.1 Display mapping


| State   | English mode                | Mandarin-glitch mode |
| ------- | --------------------------- | -------------------- |
| Boot    | `HELLO WORLD` + device name | `系统启动` + device name |
| Level 0 | `ALL CLEAR`                 | `一切正常`               |
| Level 1 | `PANIC`                     | `恐慌`                 |
| Level 2 | `PANIC!`                    | `恐慌!`                |
| Level 3 | `PANIC!!`                   | `恐慌!!`               |
| Level 4 | `PANIC!!!`                  | `恐慌!!!`              |
| Level 5 | `PANIC` / `OVERLOAD`        | `恐慌过载`               |
| Lockout | `SYSTEM` / `OFFLINE`        | `系统离线`               |

Level 5 and lockout do not show the device name. The name is spoken in the boot phrase and in the level-5 phrase only. English level 5 and English lockout are two lines each. Mandarin level 5 is the single line `恐慌过载`. Mandarin lockout is the single line `系统离线`.




## 8.2 Mandarin-glitch boot speech

Logical Chinese phrase:

```text
系统启动。
我是 SHLORP。
未见威胁。
```

The build-time SAM renderer should receive an ASCII phonetic approximation.

Default cue:

```text
SHEE TONG CHEE DONG.
WOH SHUR SHLORP.
WAY JYEN WAY SHYE.
```

Replace `SHLORP` with `DEVICE_NAME_SPEECH`.

This is intentionally robotic and approximate.

## 8.3 Mandarin-glitch panic speech

OLED:

```text
恐慌
```

or the appropriate exclamation-mark variant.

Speech:

```text
恐慌! 恐慌! 恐慌!
```

Default SAM cue:

```text
KONG HWANG!
KONG HWANG!
KONG HWANG!
```

The phrase is predetermined.

It is not randomly generated.

## 8.4 Mandarin-glitch overload speech

Logical Chinese phrase:

```text
恐慌过载。
SHLORP 害怕了。
正在关机。
```

Default SAM cue:

```text
KONG HWANG GWO ZAI.
SHLORP HIGH PAH LUH.
JUNG ZAI GWAN JEE.
```

Replace `SHLORP` with `DEVICE_NAME_SPEECH`.

---



# 9. OLED display hardware

Use:

```text
0.96 inch OLED
128 x 64 pixels
SSD1306 controller
I2C interface
4 pins: VCC, GND, SCL, SDA
3.3 V logic
typical I2C address: 0x3C
```

Preferred shopping search:

```text
0.96 OLED SSD1306 128x64 I2C 4 pin 3.3V
```

Prefer:

```text
white-on-black
```

over dual-color displays.

Typical PCB size is approximately:

```text
27 mm x 27 mm
```



## OLED supply

Power the OLED from:

```text
D1 Mini 3V3
```

not 5 V.

This prevents an OLED module with onboard I2C pull-ups from pulling SDA/SCL above the ESP8266's 3.3 V logic level.

---



# 10. Chinese font support

Use the U8g2 library.

U8g2 includes WQY Chinese/GB2312 fonts.

Use a GB2312-capable font that contains every character in the fixed localization table.

Suggested:

```text
u8g2_font_wqy16_t_gb2312b
```

If flash use becomes excessive, the implementation agent may generate a reduced font containing only the characters required by V1.1.

The selected Chinese strings are fixed, so a subset font is allowed.

The OLED must correctly display UTF-8 Chinese strings from the table above.

---



# 11. Physical panic button

Use:

```text
24 mm arcade-style push button
red
momentary
normally open (NO)
non-latching
2 terminals
panel mount
threaded/screw-in body preferred
```

Preferred shopping search:

```text
24mm red arcade button momentary normally open screw in
```

Required enclosure cutout:

```text
24 mm circular hole
```

Prefer a screw-in button with a locking nut rather than a snap-in type.

Do NOT buy:

```text
latching/self-locking button
```

The button carries only a logic signal, so its high current rating is irrelevant.

---



# 12. LEDs

Use two external LEDs per machine.

Each LED:

```text
5 mm
red
diffused preferred
standard indicator LED
forward voltage approximately 1.8–2.2 V
```

Preferred shopping search:

```text
5mm red diffused LED
```

Each LED needs one resistor:

```text
220 ohm
1/4 watt
```

Preferred shopping search:

```text
220R 1/4W resistor
```

Wiring:

```text
GPIO -> 220 ohm resistor -> LED anode
LED cathode -> GND
```

---



# 13. Active buzzer

Use:

```text
ACTIVE buzzer module
3-pin: VCC / GND / SIGNAL
3.3–5 V capable
high-level trigger
self-oscillating
onboard transistor/driver
```

Preferred shopping search:

```text
active buzzer module 3.3V 5V high level trigger 3 pin
```

Do not buy a passive piezo-only module unless the firmware/hardware design is changed.

The active buzzer allows simple digital ON/OFF control while I2S speech runs.

---



# 14. Power supply



## 14.1 Per-device supply

Each finished machine uses:

```text
5 V regulated USB power supply
2 A minimum recommended
single USB output
100–240 VAC input
```

Preferred shopping search:

```text
5V 2A USB wall charger EU plug certified
```

For use in Israel, use a plug style physically compatible with the intended outlet/adapter.

Prefer a reputable/safety-certified wall charger.

Do not optimize mains-powered adapters purely for lowest price.

## 14.2 USB cable

Use one cable per device matching the selected D1 Mini:

```text
Micro USB data cable
```

or:

```text
USB-C data cable
```

Preferred length:

```text
0.5 to 1.0 meter
```

Use a data-capable cable so the same cable can be used for firmware flashing.

---



# 15. Power wiring inside the device

USB powers the D1 Mini.

The D1 Mini 5V rail feeds the audio amplifier.

Recommended:

```text
USB 5V
  |
  +--> D1 Mini USB
          |
          +--> 5V pin --> MAX98357A VIN
          |
          +--> 5V pin --> active buzzer VCC
          |
          +--> 3V3 pin --> OLED VCC
```

All modules share common GND.

Add one electrolytic capacitor near the MAX98357A:

```text
470 uF
10 V minimum
electrolytic
```

Wiring:

```text
capacitor + -> 5V
capacitor - -> GND
```

Preferred shopping search:

```text
470uF 10V electrolytic capacitor
```

A 16 V part is also acceptable.

---



# 16. Final ESP8266 pin map

Reference board:

```text
D1 Mini ESP8266
```



## I2S audio

These are the ESP8266Audio hardware-I2S pins:


| Function           | D1 Mini label | ESP8266 GPIO |
| ------------------ | ------------- | ------------ |
| MAX98357A LRC / WS | D4            | GPIO2        |
| MAX98357A BCLK     | D8            | GPIO15       |
| MAX98357A DIN      | RX            | GPIO3        |




## OLED


| Function | D1 Mini label | ESP8266 GPIO |
| -------- | ------------- | ------------ |
| OLED SCL | D1            | GPIO5        |
| OLED SDA | D2            | GPIO4        |




## Controls


| Function             | D1 Mini label | ESP8266 GPIO |
| -------------------- | ------------- | ------------ |
| Panic button         | D5            | GPIO14       |
| Active buzzer SIGNAL | D6            | GPIO12       |
| Red LED 1            | D7            | GPIO13       |
| Red LED 2            | D0            | GPIO16       |




## Button wiring

```text
D5 / GPIO14 ---- button ---- GND
```

Configure:

```cpp
pinMode(PIN_BUTTON, INPUT_PULLUP);
```

Therefore:

```text
HIGH = released
LOW  = pressed
```

---



# 17. Important ESP8266 pin restrictions

Do not casually move signals onto boot-strapping pins.

The reference design deliberately leaves:

```text
D3 / GPIO0
```

unused.

D4/GPIO2 and D8/GPIO15 are required by hardware I2S and are also ESP8266 boot-related pins.

Do not add external pull resistors that fight the D1 Mini's normal boot configuration.

D0/GPIO16 is used only as a simple digital LED output.

It does not need PWM or interrupts.

---



# 18. Enclosure

Use one ABS project enclosure per device.

Recommended minimum external size:

```text
100 mm x 60 mm x 35 mm
```

Preferred shopping search:

```text
ABS project box 100x60x35mm screw enclosure
```

Requirements:

- removable screw-on lid,
- flat plastic top,
- approximately 1–3 mm wall thickness,
- enough internal height for perfboard and speaker,
- no metal enclosure.

A slightly larger box is acceptable.

Do not choose a shallower box unless component clearances have been physically checked.

---



# 19. Enclosure cutouts

The enclosure is intentionally modified.

## Top panel

Make:

### Button

```text
24 mm round hole
```



### OLED window

Approximate visible opening:

```text
24 mm x 13 mm rectangle
```

The OLED PCB sits behind the opening.

Measure the actual OLED before final cutting.

### LEDs

Two holes:

```text
5 mm diameter each
```



### Speaker grille

Drill approximately:

```text
8–12 holes
3 mm diameter
```

in a circular/grid pattern over the speaker.

Do not cut a full 40 mm open hole; a grille protects the speaker cone.

## Side panel

Cut an opening aligned to the D1 Mini USB connector.

Typical starting size:

```text
approximately 12 mm x 8 mm
```

Measure the actual USB plug/board before cutting.

Mount the D1 Mini so a normal USB plug can be inserted without opening the enclosure.

---



# 20. Internal mechanical construction

Do not use a solderless breadboard inside the final product.

Use one small perfboard per finished device.

Recommended:

```text
40 mm x 60 mm
double-sided plated-through-hole prototyping PCB
2.54 mm pitch
```

Preferred shopping search:

```text
4x6cm double sided prototype PCB perfboard 2.54mm
```

Mount on perfboard:

- D1 Mini,
- MAX98357A,
- power capacitor,
- LED resistors,
- buzzer wiring,
- wiring headers if desired.

The OLED, button, speaker, and LEDs are panel-mounted and wired back to the perfboard.

---



# 21. Development breadboard

Only ONE solderless breadboard is required for developing the first unit.

Recommended:

```text
MB102-compatible 830-point breadboard
male-to-male jumper wire set
male-to-female jumper wire set
```

Preferred shopping search:

```text
MB102 830 point breadboard jumper wire kit
```

Once the design works:

```text
prototype breadboard -> removed
finished unit -> soldered perfboard
```

---



# 22. Additional wiring/assembly consumables

Required/shared across the four builds:

## Hookup wire

```text
26 AWG stranded hookup wire
multiple colors
approximately 5 meters total minimum
```

Search:

```text
26AWG silicone hookup wire kit
```



## Pin headers

```text
2.54 mm male breakaway header strips
```

Useful if modules arrive without headers.

## Heat-shrink tubing

```text
2 mm / 3 mm / 4 mm assortment
```

Recommended for button/speaker connections.

## Mounting hardware

Recommended:

```text
M2 nylon standoff/screw/nut assortment
```

Useful for OLED/perfboard mounting.

Hot glue may be used for strain relief, but should not be the only method holding the panic button.

---



# 23. Complete hardware BOM — per machine

Each finished PANIC BUTTON requires:


| Qty       | Part              | Exact requirement                                 |
| --------- | ----------------- | ------------------------------------------------- |
| 1         | ESP8266 board     | D1 Mini, ESP-12F/ESP8266EX, 4 MB flash            |
| 1         | OLED              | 0.96", SSD1306, 128x64, I2C, 4-pin, 3.3 V logic   |
| 1         | Audio amp         | MAX98357A I2S mono Class-D breakout               |
| 1         | Speaker           | 40 mm, 4 ohm, 3 W                                 |
| 1         | Panic button      | 24 mm red arcade button, momentary NO, screw-in   |
| 1         | Buzzer            | Active buzzer module, 3.3–5 V, high-level trigger |
| 2         | LEDs              | 5 mm red diffused                                 |
| 2         | LED resistors     | 220 ohm, 1/4 W                                    |
| 1         | Power capacitor   | 470 uF, >=10 V electrolytic                       |
| 1         | Perfboard         | 40x60 mm, 2.54 mm pitch                           |
| 1         | Enclosure         | ABS, approx. 100x60x35 mm                         |
| 1         | USB cable         | matching board connector, data-capable            |
| 1         | Wall power supply | regulated 5 V / >=2 A USB                         |
| as needed | Wire              | 26 AWG stranded hookup wire                       |
| as needed | Heat shrink       | 2–4 mm                                            |
| as needed | M2 hardware       | standoffs/screws/nuts                             |


---



# 24. Shopping list — four finished machines

Required quantities:

```text
4 x D1 Mini ESP8266 ESP-12F 4MB
4 x 0.96" SSD1306 128x64 I2C OLED
4 x MAX98357A I2S amplifier
4 x 40mm 4ohm 3W speaker
4 x 24mm red momentary NO arcade button
4 x active buzzer module 3.3-5V high-level trigger
8 x 5mm red diffused LED
8 x 220R 1/4W resistor
4 x 470uF >=10V electrolytic capacitor
4 x 40x60mm double-sided 2.54mm perfboard
4 x ABS enclosure approx. 100x60x35mm
4 x USB data cable matching the selected D1 Mini connector
4 x regulated 5V 2A USB wall power supply

Shared:
1 x MB102 830-point breadboard
1 x jumper-wire kit
1 x 26AWG hookup-wire assortment
1 x 2.54mm breakaway header pack
1 x heat-shrink assortment
1 x M2 nylon standoff/screw/nut assortment
```

Recommended purchase practice:

- Buy LEDs/resistors/capacitors in inexpensive multipacks.
- If cheap enough, buy 5 D1 Minis, 5 OLEDs, and 5 MAX98357A modules so one spare is available.
- Wall chargers should be reputable/safety-certified rather than the absolute cheapest mains-powered item.

---



# 25. Basic assembly tools

Not part of each machine, but needed to build the project:

```text
soldering iron
electronics solder
side cutters
wire stripper
small screwdriver
drill
24 mm step drill or hole saw
3 mm drill bit
5 mm drill bit
small file / rotary tool for OLED window
multimeter
```

Strongly recommended:

```text
hot glue gun
```

for strain relief and securing lightweight internal wiring.

---



# 26. Device state machine

Top-level states:

```cpp
enum class DeviceState {
    BOOT,
    NORMAL,
    LEVEL5_SEQUENCE,
    LOCKOUT
};
```

Session language:

```cpp
enum class SessionLanguage {
    ENGLISH,
    MANDARIN_GLITCH
};
```

Normal panic level:

```text
0..4
```

Level 5 is a terminal overload sequence that leads to lockout.

---



# 27. Boot behavior

On every BOOT entry:

1. Initialize/confirm wireless radio.
2. Choose session language with 10% Mandarin-glitch probability.
3. Display boot text for the chosen language.
4. Display the configured device name.
5. Buzzer OFF.
6. Run happy LED pattern.
7. Play corresponding generated boot speech.
8. While that greeting is playing, accept local presses and new remote panic UUIDs, as specified below.
9. If no panic was accepted, when the minimum boot duration and the boot speech are both complete:
  - `panicLevel = 0`
  - LEDs OFF
  - enter NORMAL
  - display level-0 text for the current session language.

Suggested minimum boot duration:

```text
4000 ms
```

That minimum applies only when the greeting finishes. An accepted panic does not wait for it.

### Panic during BOOT

The greeting is interruptible. The session language has already been rolled and stays fixed.

A local press, or a received UUID that is not already in the cache, is handled immediately with the same rules as a panic in NORMAL:

- remember the UUID,
- broadcast it (a new UUID for a local press, the same UUID for a relay),
- increment the panic level by one,
- reset the decay timer,
- stop the boot speech and the happy LED pattern,
- leave BOOT, and do not return to the greeting afterward.

If the resulting level is 1–4, enter NORMAL, show that level's screen, play that level's panic phrase, and run the buzzer and panic LEDs for `PANIC_PROGRAM_MS`.

If the resulting level is 5, enter `LEVEL5_SEQUENCE` immediately. From that moment the overload show ignores further input, as specified in section 42.

A UUID that is already in the cache is dropped. The greeting continues.

A button hold that began during `LEVEL5_SEQUENCE` or `LOCKOUT` is not a press. It counts only after the button is released and pressed again.

---



# 28. Happy boot LED pattern

Use both LEDs.

Example:

```text
LED1 ON, LED2 OFF
LED1 OFF, LED2 ON
BOTH ON
BOTH OFF
repeat
```

Suggested step time:

```text
200 ms
```

Buzzer remains OFF.

---



# 29. Panic level table



## English session


| Level | Screen           | Buzzer      | LEDs         | Speech                 |
| ----- | ---------------- | ----------- | ------------ | ---------------------- |
| 0     | `ALL CLEAR`      | Off         | Off          | Off                    |
| 1     | `PANIC`          | Alarm       | Normal flash | `PANIC! PANIC! PANIC!` |
| 2     | `PANIC!`         | Alarm       | Normal flash | `PANIC! PANIC! PANIC!` |
| 3     | `PANIC!!`        | Alarm       | Fast flash   | `PANIC! PANIC! PANIC!` |
| 4     | `PANIC!!!`       | Alarm       | Fast flash   | `PANIC! PANIC! PANIC!` |
| 5     | `PANIC OVERLOAD` | Sad pattern | Slow flash   | overload phrase        |




## Mandarin-glitch session


| Level | Screen  | Buzzer      | LEDs         | Speech cue                |
| ----- | ------- | ----------- | ------------ | ------------------------- |
| 0     | `一切正常`  | Off         | Off          | Off                       |
| 1     | `恐慌`    | Alarm       | Normal flash | `KONG HWANG!` x3          |
| 2     | `恐慌!`   | Alarm       | Normal flash | `KONG HWANG!` x3          |
| 3     | `恐慌!!`  | Alarm       | Fast flash   | `KONG HWANG!` x3          |
| 4     | `恐慌!!!` | Alarm       | Fast flash   | `KONG HWANG!` x3          |
| 5     | `恐慌过载`  | Sad pattern | Slow flash   | alternate overload phrase |


The Screen column is what the OLED shows for the whole time the device remains at that level. The screen stays up after the buzzer and LEDs have turned off.

For levels 1–4, Alarm and the LED flash start when an upward event is accepted and run for `PANIC_PROGRAM_MS` (4000 ms), then turn off. They do not stay on for as long as that level is displayed. A later upward event starts them again. Flash speed follows the level just reached: normal flash at levels 1–2, fast flash at levels 3–4.

`PANIC_PROGRAM_MS` does not apply to level 5. The sad buzzer and slow flash run for the whole overload sequence in section 42. Level 0 and lockout keep the buzzer and LEDs off.

Speech for levels 1–4 plays that level's phrase once per upward event. The clip plays through to the end even when that is longer than 4 seconds. The buzzer and LEDs still stop at 4 seconds. A new upward event during either the phrase or the 4-second window restarts the phrase from the beginning and starts a fresh 4-second buzzer and LED run.

---



# 30. Panic decay

Default:

```text
DECAY_GRACE_MS = 15000
DECAY_STEP_MS  = 10000
```

After an accepted panic event:

- wait 15 seconds,
- decrease panic level by 1,
- then decrease by 1 every 10 seconds,
- stop at level 0.

Critical rule:

```text
DOWNWARD TRANSITIONS ARE SILENT.
```

Decay changes the internal level and the OLED text for that lower level, in the session language chosen at BOOT. A step from level 2 to level 1 removes one exclamation mark. The last step, from level 1 to level 0, changes the screen from `PANIC` / `恐慌` to `ALL CLEAR` / `一切正常`.

Decay does not start speech, the buzzer, or the LEDs. It does not restart them, and it does not cut off a panic phrase that is already playing. The buzzer and LEDs stop when `PANIC_PROGRAM_MS` elapses, not when the level decays.

Level 5 does not decay. It runs the overload sequence, then lockout, then BOOT.

---



# 31. Local physical button flow

A press is the transition into the pressed state after the button has been seen released. Holding the button does not repeat. Debounce with `BUTTON_DEBOUNCE_MS`.

A hold that began during `LEVEL5_SEQUENCE` or `LOCKOUT` is not a press when BOOT begins. It counts only after a release and a new press.

Presses are accepted in:

```text
BOOT
NORMAL
```

Presses are ignored in `LEVEL5_SEQUENCE` and `LOCKOUT`.

When a press is accepted:

1. debounce,
2. generate a new 128-bit UUID,
3. insert that UUID into the local seen cache,
4. schedule an ESP-NOW broadcast of that same UUID,
5. apply one local panic increment immediately,
6. reset the decay timer,
7. start this level's speech, buzzer, and LEDs, as specified in section 29.

If the device was in BOOT, leave BOOT as specified in section 27. Do not return to the greeting.

The local device does not wait for network transmission.

A new accepted press while a levels 1–4 phrase or 4-second window is already running restarts the phrase from the beginning and starts a fresh `PANIC_PROGRAM_MS` buzzer and LED run. The flash speed follows the level just reached.

The press that moves the device from level 4 to level 5 is accepted, stored, and broadcast. The overload show then ignores further presses.

---



# 32. ESP-NOW network

Use ESP-NOW on the ESP8266 Wi-Fi radio.

Do not connect to office infrastructure Wi-Fi.

No:

```text
SSID
password
router
Internet
server
phone app
pairing UI
```

Use:

```text
WiFi station mode
fixed ESP-NOW channel
broadcast packets
```

Suggested:

```cpp
constexpr uint8_t ESPNOW_CHANNEL = 6;
```

All devices intended to interact must use the same channel.

---



# 33. Network packet

Suggested packed payload:

```cpp
struct PanicPacket {
    uint32_t magic;
    uint8_t protocolVersion;
    uint8_t messageType;
    uint16_t groupId;
    uint8_t uuid[16];
};
```

Suggested:

```text
messageType = PANIC_EVENT
```

Use a compile-time group identifier:

```cpp
constexpr uint16_t PANIC_GROUP_ID = 0x2026;
```

Devices with different group IDs ignore each other.

---



# 34. UUID generation

Each physical button press creates one random 128-bit UUID.

Store as:

```cpp
uint8_t uuid[16];
```

Do not transmit it as a printable string.

Use the ESP8266 random source.

Set UUIDv4 version/variant bits if convenient.

No central server is required.

---



# 35. Seen-UUID cache

Every device maintains a fixed-capacity cache of recently seen UUIDs.

Recommended:

```text
128 entries
16 bytes per UUID
oldest-entry eviction
```

Logical behavior:

```text
new UUID:
    remember
    relay
    panic +1

already-seen UUID:
    do nothing
```

Avoid `std::set` and dynamic allocation.

Use a fixed-size ring buffer or equivalent.

---



# 36. UUID cache clearing

The UUID cache clears only after the device has remained continuously at:

```text
NORMAL
panicLevel == 0
```

for:

```text
10 minutes
```

Constant:

```cpp
constexpr uint32_t UUID_CACHE_CLEAR_IDLE_MS = 600000;
```

Leaving level 0 cancels/resets this timer.

BOOT, overload, or lockout also cancel this timer. Cancelling the timer does not erase stored UUIDs.

The cache survives `LEVEL5_SEQUENCE`, `LOCKOUT`, and the BOOT that follows lockout. It lives in RAM, so a power loss clears it. The only timed clear is the 10-minute idle rule above.

After returning to level 0, start a fresh 10-minute timer.

---



# 37. Received panic event flow

For every received packet:

1. Validate packet size.
2. Validate protocol magic.
3. Validate protocol version.
4. Validate message type.
5. Validate group ID.
6. Verify the device is currently in BOOT or NORMAL. In `LEVEL5_SEQUENCE` or `LOCKOUT`, drop the packet with no increment, no relay, and no cache insert.
7. Check UUID cache.

If UUID already exists:

```text
DROP
```

No relay.

No increment.

During BOOT, a duplicate also leaves the greeting running.

If UUID is new:

```text
insert UUID into cache
schedule re-broadcast of SAME UUID
apply one local panic increment
reset decay timer
start this level's speech, buzzer, and LEDs
```

If the device was in BOOT, leave BOOT as specified in section 27.

Never generate a replacement UUID while relaying. One physical press creates one UUID. Every unit that has not seen it panics once and rebroadcasts that same UUID. A unit that already saw it does nothing, so the press raises each unit by one and then stops.

---



# 38. Flooding/relay behavior

A device forwards each unseen UUID once.

This allows:

```text
A -> B -> C
```

even if A and C cannot directly hear each other.

Because every device remembers the UUID before forwarding:

```text
A -> B -> C -> A
```

does not become an infinite loop.

A returning copy is already in A's cache and is ignored.

---



# 39. ESP-NOW retransmission

For radio reliability, each accepted/originated event may be physically transmitted twice.

Recommended:

```text
TX_REPEAT_COUNT = 2
TX_REPEAT_DELAY_MS = 40
```

Both transmissions use the same UUID.

For a relayed packet, add a small random jitter before the first transmission:

```text
10–40 ms
```

This reduces the chance that several receiving devices rebroadcast at exactly the same instant.

The jitter must be non-blocking.

---



# 40. Simultaneous button presses

Two people pressing two different machines produce two different UUIDs.

Every active device processes both UUIDs once.

Therefore:

```text
two real presses = two panic increments
```

This is intentional.

---



# 41. Events during BOOT / OVERLOAD / LOCKOUT

New panic events are accepted in `BOOT` and `NORMAL`.

During:

```text
LEVEL5_SEQUENCE
LOCKOUT
```

the device is unresponsive to the button and to wifi. Local presses and received panic events are ignored:

```text
do not increment
do not relay
do not add UUID to cache
```

The radio stays initialized. Packets that arrive during those two states are discarded.

The event that caused the transition into `LEVEL5_SEQUENCE` has already been stored and transmitted. Only events that arrive after that transition are ignored.

`LOCKOUT` lasts 60 seconds and is unresponsive for that entire time. The following BOOT is responsive again, as specified in section 27. The UUID cache is still intact.

---



# 42. Level 5 overload sequence

A panic event taking the device from level 4 to level 5 immediately begins overload.

## English

Display:

```text
PANIC
OVERLOAD
```

Speech:

```text
PANIC OVERLOAD.
SHLORP IS SCARED.
SHUTTING DOWN SYSTEM.
```



## Mandarin-glitch

Display:

```text
恐慌过载
```

The device name is not shown on the OLED. It is spoken in the phrase below.

Speech cue:

```text
KONG HWANG GWO ZAI.
SHLORP HIGH PAH LUH.
JUNG ZAI GWAN JEE.
```



## Common behavior

Buzzer:

```text
slow/sad rhythm
```

LEDs:

```text
slow flashing
```

Ignore all new button presses and wifi panics for this entire sequence. Do not increment, relay, or cache them.

`PANIC_PROGRAM_MS` does not apply here. The sad buzzer and slow LED flash run for the whole sequence, then both turn off when lockout begins.

Suggested minimum duration:

```text
6000 ms
```

Remain in overload until both of these are true:

- the minimum duration has elapsed,
- the overload speech has finished.

Then enter LOCKOUT.

---



# 43. Buzzer patterns

Use an active buzzer.

## Normal alarm, levels 1–4

Run this only during the active `PANIC_PROGRAM_MS` window after an upward event. When that window ends, turn the buzzer off even if the pattern is mid-cycle. A new upward event starts the pattern over for a fresh 4000 ms.

Example:

```text
150 ms ON
150 ms OFF
150 ms ON
500 ms OFF
repeat
```



## Level 5

Run this for the whole overload sequence, not for `PANIC_PROGRAM_MS`. Turn the buzzer off for the entire lockout.

Example:

```text
800 ms ON
600 ms OFF
250 ms ON
1000 ms OFF
repeat
```

All patterns are timestamp/state-machine driven.

No blocking delay.

---



# 44. LED patterns



## Boot

```text
alternating / happy chase
200 ms steps
```



## Levels 1–2

Run this only during the active `PANIC_PROGRAM_MS` window. When that window ends, turn both LEDs off. A new upward event starts the pattern for the level just reached.

```text
300 ms ON
300 ms OFF
```



## Levels 3–4

Same 4-second window as levels 1–2, at the faster rate. A step from level 2 to level 3 during an active window switches to this rate and restarts the 4000 ms window.

```text
100 ms ON
100 ms OFF
```



## Level 5

Run this for the whole overload sequence, then turn both LEDs off for lockout.

```text
700 ms ON
700 ms OFF
```



## Level 0 / after the 4-second window / lockout

```text
OFF
```

No blocking loops.

---



# 45. One-minute lockout

After overload:

```text
LOCKOUT_MS = 60000
```

During lockout the device is unresponsive to every input:

- button ignored,
- wireless panic events ignored,
- no relay,
- do not add the UUID to the cache,
- no speech,
- buzzer off,
- LEDs off.

The UUID cache is kept. Lockout does not clear it.

Display:

English:

```text
SYSTEM
OFFLINE
```

Mandarin-glitch:

```text
系统离线
```

After 60 seconds:

```text
state = BOOT
```

The next BOOT performs a new independent 10% language roll. That BOOT is responsive: a press or a new UUID interrupts the greeting. The cache from before lockout is still there, so a UUID seen before overload is still a duplicate.

No physical ESP8266 reset is required. A power loss is what clears the UUID cache.

---



# 46. Display implementation

Use U8g2.

ESP8266 has enough RAM for a full 128x64 monochrome framebuffer.

Preferred constructor:

```cpp
U8G2_SSD1306_128X64_NONAME_F_HW_I2C
```

Only redraw when:

- state changes,
- level changes,
- boot page changes,
- display content otherwise changes.

Do not continuously redraw every loop iteration.

English strings may use a larger bold font.

Chinese strings use a WQY/GB2312 font.

---



# 47. Audio concurrency requirement

While a WAV speech clip is playing, all of these must remain active:

```text
ESP-NOW receive
ESP-NOW relay transmit
button polling
panic decay timers
LED animation
buzzer pattern
display state
overload timer
lockout timer
```

The main loop must service audio frequently.

No `delay()` in normal runtime logic.

---



# 48. Main loop architecture

Conceptually:

```cpp
void loop() {
    updateNetworkReceiveQueue();
    updateNetworkTransmitQueue();

    updateDeviceState();
    updateButton();
    updatePanicDecay();
    updateUuidCacheTimer();

    updateAudio();
    updateBuzzer();
    updateLeds();
    updateDisplay();

    yield();
}
```

All update functions return quickly.

ESP-NOW callbacks must do minimal work.

Callbacks should copy validated packet data into a small queue and return.

---



# 49. Recommended source structure

```text
PanicButton/
|
|-- src/
|   |-- main.cpp
|   |-- config.h
|   |-- device_config.h
|   |
|   |-- device_state.h
|   |-- device_state.cpp
|   |
|   |-- display_controller.h
|   |-- display_controller.cpp
|   |
|   |-- audio_controller.h
|   |-- audio_controller.cpp
|   |
|   |-- led_controller.h
|   |-- led_controller.cpp
|   |
|   |-- buzzer_controller.h
|   |-- buzzer_controller.cpp
|   |
|   |-- network_protocol.h
|   |-- network_controller.h
|   |-- network_controller.cpp
|   |
|   |-- uuid_cache.h
|   |-- uuid_cache.cpp
|   |
|   |-- generated_speech_assets.h
|
|-- tools/
|   |-- build_device.py
|   |-- generate_speech_assets.py
|   |-- sam_renderer/
|
|-- platformio.ini
|
|-- README.md
```

PlatformIO is preferred for V1.1 because the build-time speech-generation step can be automated cleanly.

Arduino CLI is acceptable if the same one-command workflow is preserved.

---



# 50. Central configuration

`config.h` should contain:

```cpp
// Pin assignments
PIN_BUTTON
PIN_BUZZER
PIN_LED_1
PIN_LED_2

// Networking
ESPNOW_CHANNEL
PANIC_GROUP_ID
PROTOCOL_VERSION

// Timing
BUTTON_DEBOUNCE_MS
DECAY_GRACE_MS
DECAY_STEP_MS
PANIC_PROGRAM_MS
LEVEL5_SEQUENCE_MS
LOCKOUT_MS
UUID_CACHE_CLEAR_IDLE_MS

// Radio reliability
TX_REPEAT_COUNT
TX_REPEAT_DELAY_MS
TX_RELAY_JITTER_MIN_MS
TX_RELAY_JITTER_MAX_MS

// Cache
SEEN_UUID_CAPACITY
```

`device_config.h` contains only device-specific identity:

```cpp
constexpr char DEVICE_NAME[] = "SHLORP";
constexpr char DEVICE_NAME_SPEECH[] = "SHLORP";
```

---



# 51. Suggested defaults

```text
BUTTON_DEBOUNCE_MS       = 40
DECAY_GRACE_MS           = 15000
DECAY_STEP_MS            = 10000

PANIC_PROGRAM_MS         = 4000
LEVEL5_SEQUENCE_MS       = 6000
LOCKOUT_MS               = 60000

UUID_CACHE_CLEAR_IDLE_MS = 600000

SEEN_UUID_CAPACITY       = 128

TX_REPEAT_COUNT          = 2
TX_REPEAT_DELAY_MS       = 40
TX_RELAY_JITTER_MIN_MS   = 10
TX_RELAY_JITTER_MAX_MS   = 40

MANDARIN_GLITCH_PERCENT  = 10
```

`PANIC_PROGRAM_MS` is the buzzer and LED window for levels 1–4 after each upward panic event. It is not a boot duration, not a speech duration, and not used by level 5. Boot uses its own 4000 ms minimum. Level 5 uses `LEVEL5_SEQUENCE_MS`.

---



# 52. Serial/debug behavior

Because RX/GPIO3 is used for hardware I2S, ordinary full UART debugging is constrained during audio operation.

Development logging may use TX-only Serial output where practical.

Provide:

```cpp
#define PANIC_DEBUG 1
```

Example messages:

```text
[BOOT] device=SHLORP
[BOOT] language=ENGLISH

[BOOT] language=MANDARIN_GLITCH

[BUTTON] local panic
[UUID] new event ...
[NET] TX
[NET] RX
[NET] unseen -> relay
[NET] duplicate -> drop

[PANIC] level 2 -> 3

[CACHE] cleared after 10m at level 0
```

Debug logging must not materially disrupt audio timing.

---



# 53. Hardware failure behavior

If ESP-NOW initialization fails:

```text
local panic button must still work
display must still work
speech must still work
LEDs must still work
buzzer must still work
```

The device operates as a standalone panic button.

If OLED initialization fails, other subsystems continue.

If audio initialization fails, visual/buzzer/network behavior continues.

---



# 54. Firmware safety/resource rules

Do not use:

```text
dynamic Arduino String objects in core state/network paths
unbounded containers
blocking delay() sequencing
blocking speech during runtime
std::set for UUIDs
heap allocation for every packet
```

Prefer:

```text
fixed-size structs
fixed buffers
PROGMEM
ring buffers
millis()-based state machines
```

---



# 55. V1.1 acceptance criteria

V1.1 is complete only when:

## Hardware

1. Firmware runs on D1 Mini ESP8266 with 4 MB flash.
2. OLED is powered at 3.3 V and communicates over I2C.
3. MAX98357A runs from 5 V and receives I2S.
4. 4-ohm speaker is driven only from SPK+/SPK-.
5. Button works with INPUT_PULLUP.
6. Both LEDs operate through series resistors.
7. Active buzzer works from digital control.
8. Device runs reliably from a 5 V / 2 A USB supply.
9. Audio playback at intended volume does not reset the ESP8266.
10. 470 uF supply capacitor is installed near audio amplifier.



## Naming

1. Device name is controlled by exactly two user-facing strings:
  - DEVICE_NAME
    - DEVICE_NAME_SPEECH
2. Changing those strings and running the build tool regenerates appropriate speech.
3. Boot speech says the device name.
4. Level-5 speech says the device name.



## Language glitch

1. Every BOOT independently selects Mandarin-glitch mode with 10% probability.
2. The selected language mode remains fixed for the whole session.
3. Post-lockout BOOT performs a fresh roll.
4. Mandarin-glitch mode displays valid predefined Chinese strings.
5. Chinese characters render correctly on the SSD1306.
6. Mandarin-glitch panic speech uses its predetermined alternate phrase.
7. Mandarin-glitch overload speech includes the device name. The OLED shows only `恐慌过载`.
8. No random Chinese Unicode characters are generated at runtime.



## Panic behavior

1. Level 0 shows ALL CLEAR / 一切正常.
2. Upward press 0->1 triggers panic effects.
3. Level 1 displays PANIC / 恐慌.
4. Level 2 adds one exclamation mark.
5. Level 3 adds another.
6. Level 4 adds another.
7. Levels 1–4 all play the appropriate three-repeat panic phrase.
8. Levels 3–4 flash faster than levels 1–2.
9. Decay is silent.
10. Decay updates display to the lower level.
11. Level 5 runs overload.
12. Level 5 includes configured device name in speech.
13. Level 5 enters approximately one-minute lockout.
14. Lockout ignores local and network panic events.
15. After lockout, device enters BOOT rather than hardware reset.
16. Levels 1–4 run the buzzer and panic LEDs for `PANIC_PROGRAM_MS` after each upward event, then turn them off while leaving the level text on screen.
17. A new upward event during that window restarts the panic phrase and the 4-second window.
18. A new panic during BOOT interrupts the greeting and is applied immediately. A duplicate UUID during BOOT does not.
19. The overload show ignores the button and wifi, and does not cache events that arrive during the show.



## Audio/concurrency

1. Speech is stored in firmware; no SD card.
2. Speech playback is non-blocking/cooperative at runtime.
3. LEDs continue animating while speech plays.
4. Buzzer patterns continue while speech plays.
5. ESP-NOW can receive while speech plays.
6. ESP-NOW can relay while speech plays.
7. Display remains responsive while speech plays.
8. No runtime effect sequencing depends on long delay() calls.



## Network

1. Physical button press generates a new 128-bit UUID.
2. Origin stores UUID before broadcasting.
3. New remote UUID is stored before relaying.
4. Relay retains exact same UUID.
5. Each UUID increments each device at most once.
6. Duplicate UUIDs are dropped.
7. Every unseen accepted UUID is relayed.
8. Event can travel A -> B -> C when A cannot directly reach C.
9. Two independent physical presses produce two increments.
10. Received event resets decay timer.
11. Events during overload and lockout are not stored or relayed. A new event during BOOT is stored, relayed, and applied immediately.
12. UUID cache clears only after ten continuous minutes at NORMAL level 0. Overload, lockout, and the following BOOT do not clear it.
13. Cache has bounded fixed capacity.
14. Cache overflow evicts oldest entry.
15. Different group IDs ignore each other.
16. No router or Internet is required.



## Build/user workflow

1. Repository includes one-command device build/upload helper.
2. User can prepare another named unit by changing only the two name strings.
3. Generated speech assets are recreated automatically.
4. The final firmware and assets fit comfortably in 4 MB flash.

---



# 56. Instructions to the implementation agent

Implement V1.1 from this document alone. v0.2 and v1.0 are not required.

Behavior that must be implemented as written here:

- `PANIC_PROGRAM_MS` is only the levels 1–4 buzzer and LED window.
- The OLED keeps showing the current level after that window ends. Decay changes the screen and nothing else.
- BOOT accepts a new press or a new UUID and abandons the greeting. Duplicates during BOOT do nothing.
- `LEVEL5_SEQUENCE` and `LOCKOUT` ignore the button and wifi, and do not cache those events.
- The UUID cache survives lockout and the soft boot after it.
- A received panic rebroadcasts the same UUID. It does not mint a new one.
- Mandarin overload shows only `恐慌过载` on the OLED. The device name is spoken.

Required architectural decisions:

- Target D1 Mini ESP8266 / ESP-12F / 4 MB.
- Use ESP-NOW on ESP8266, not ESP32-specific APIs.
- Use hardware I2S with:
  - LRC GPIO2 / D4
  - BCLK GPIO15 / D8
  - DIN GPIO3 / RX
- Use MAX98357A.
- Use ESP8266Audio for non-blocking WAV playback.
- Do not use ESP8266SAM `Say()` synchronously in runtime.
- Generate SAM-style WAV assets at build time.
- Store all speech in PROGMEM.
- Use U8g2 for SSD1306 and Chinese text.
- Keep device customization to two strings.
- Roll 10% language mode only on BOOT.
- Keep selected language for the full boot session.
- Preserve silent decay.
- Preserve overload and lockout.
- Preserve UUID relay/flooding behavior.
- Keep the receive callback minimal.
- Use a fixed UUID ring cache.
- Re-broadcast unseen UUIDs.
- Never re-process duplicate UUIDs.
- Use non-blocking retransmission and relay jitter.
- Keep local panic response immediate.
- Keep hardware subsystems independent.
- Do not introduce additional hardware unless absolutely necessary.

---



# 57. Final shopping-search checklist

For easiest copy/paste into AliExpress or another marketplace:

```text
D1 Mini ESP8266 ESP-12F 4MB CH340 Micro USB
0.96 OLED SSD1306 128x64 I2C 4 pin 3.3V
MAX98357A I2S 3W Class D amplifier module
40mm 4ohm 3W speaker
24mm red arcade button momentary normally open screw in
active buzzer module 3.3V 5V high level trigger 3 pin
5mm red diffused LED
220R 1/4W resistor
470uF 10V electrolytic capacitor
4x6cm double sided prototype PCB perfboard 2.54mm
ABS project box 100x60x35mm screw enclosure
Micro USB data cable 1m
5V 2A USB wall charger certified
26AWG silicone hookup wire kit
2.54mm breakaway male header
heat shrink tubing assortment
M2 nylon standoff screw nut kit
MB102 830 point breadboard jumper wire kit
```

If USB-C D1 Minis are selected, replace the Micro-USB cable line with:

```text
USB-C data cable
```

All four boards should preferably use the same USB connector.

---



# 58. Guiding experience

The normal machine should feel like:

```text
HELLO WORLD
SHLORP

"BOOTING UP SYSTEMS.
 I AM SHLORP.
 NO THREATS FOUND."
```

Then one day, approximately one boot in ten:

```text
系统启动
SHLORP
```

and the same machine spends that entire session consistently speaking its strange robotic Mandarin-localization mode.

A panic spreads through the office:

```text
ONE REAL PRESS
      |
      v
NEW UUID
      |
      v
REMEMBER -> RELAY -> PANIC ONCE
      |
      v
OTHER DEVICES DO THE SAME
```

The finished object should still feel like a very simple toy:

```text
ONE BIG RED BUTTON.
LOTS OF CONSEQUENCES.
```

