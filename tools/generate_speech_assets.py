#!/usr/bin/env python3
"""Generate src/generated_speech_assets.h from device_config.h."""

from __future__ import annotations

import math
import re
import struct
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DEVICE_CONFIG = ROOT / "src" / "device_config.h"
OUT_HEADER = ROOT / "src" / "generated_speech_assets.h"
SAM_BIN = ROOT / "tools" / "sam_renderer" / "sam"


def parse_device_config() -> tuple[str, str]:
    text = DEVICE_CONFIG.read_text(encoding="utf-8")
    name = re.search(r'DEVICE_NAME\[\]\s*=\s*"([^"]+)"', text)
    speech = re.search(r'DEVICE_NAME_SPEECH\[\]\s*=\s*"([^"]+)"', text)
    if not name or not speech:
        raise SystemExit("Could not parse device_config.h")
    return name.group(1), speech.group(1)


def sam_render(text: str, out_wav: Path) -> bool:
    if not SAM_BIN.exists():
        return False
    try:
        subprocess.run(
            [str(SAM_BIN), text, str(out_wav)],
            check=True,
            capture_output=True,
            text=True,
        )
        return out_wav.exists()
    except subprocess.CalledProcessError:
        return False


def synth_beep_wav(out_wav: Path, duration_s: float, freq_hz: float = 440.0) -> None:
    sample_rate = 22050
    n = int(sample_rate * duration_s)
    samples = bytearray()
    for i in range(n):
        v = int(127 + 100 * math.sin(2 * math.pi * freq_hz * i / sample_rate))
        samples.append(max(0, min(255, v)))

    data_size = len(samples)
    byte_rate = sample_rate
    block_align = 1
    bits = 8
    header = struct.pack(
        "<4sI4s4sIHHIIHH4sI",
        b"RIFF",
        36 + data_size,
        b"WAVE",
        b"fmt ",
        16,
        1,
        1,
        sample_rate,
        byte_rate,
        block_align,
        bits,
        b"data",
        data_size,
    )
    out_wav.write_bytes(header + samples)


def wav_to_c_array(wav_path: Path) -> tuple[str, int]:
    data = wav_path.read_bytes()
    lines = []
    for i in range(0, len(data), 16):
        chunk = data[i : i + 16]
        lines.append("  " + ", ".join(f"0x{b:02x}" for b in chunk))
    body = ",\n".join(lines)
    return body, len(data)


def main() -> int:
    device_name, device_speech = parse_device_config()

    phrases = {
        "speech_boot_en": (
            f"BOOTING UP SYSTEMS. I AM {device_speech}. NO THREATS FOUND.",
            2.8,
            350,
        ),
        "speech_panic_en": ("PANIC! PANIC! PANIC!", 1.6, 520),
        "speech_overload_en": (
            f"PANIC OVERLOAD. {device_speech} IS SCARED. SHUTTING DOWN SYSTEM.",
            3.2,
            300,
        ),
        "speech_boot_zh": (
            f"SHEE TONG CHEE DONG. WOH SHUR {device_speech}. WAY JYEN WAY SHYE.",
            2.8,
            380,
        ),
        "speech_panic_zh": ("KONG HWANG! KONG HWANG! KONG HWANG!", 1.6, 540),
        "speech_overload_zh": (
            f"KONG HWANG GWO ZAI. {device_speech} HIGH PAH LUH. JUNG ZAI GWAN JEE.",
            3.2,
            320,
        ),
    }

    build_dir = ROOT / "build" / "speech"
    build_dir.mkdir(parents=True, exist_ok=True)

    parts: list[str] = [
        "#pragma once",
        "",
        "#include <Arduino.h>",
        "",
        f"// Device: {device_name} (speech: {device_speech})",
        "",
    ]

    for symbol, (text, duration, freq) in phrases.items():
        wav_path = build_dir / f"{symbol}.wav"
        if not sam_render(text, wav_path):
            synth_beep_wav(wav_path, duration, freq)
        body, length = wav_to_c_array(wav_path)
        parts.append(f"static const uint8_t {symbol}[] PROGMEM = {{")
        parts.append(body)
        parts.append("};")
        parts.append(f"static const uint32_t {symbol}_len = {length};")
        parts.append("")

    OUT_HEADER.write_text("\n".join(parts) + "\n", encoding="utf-8")
    print(f"Wrote {OUT_HEADER}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
