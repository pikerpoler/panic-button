#!/usr/bin/env python3
"""Generate src/generated_speech_assets.h from device_config.h."""

from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DEVICE_CONFIG = ROOT / "src" / "device_config.h"
OUT_HEADER = ROOT / "src" / "generated_speech_assets.h"
SAM_DIR = ROOT / "tools" / "sam_renderer"
SAM_BIN = SAM_DIR / "sam"


def parse_device_config() -> tuple[str, str]:
    text = DEVICE_CONFIG.read_text(encoding="utf-8")
    name = re.search(r'DEVICE_NAME\[\]\s*=\s*"([^"]+)"', text)
    speech = re.search(r'DEVICE_NAME_SPEECH\[\]\s*=\s*"([^"]+)"', text)
    if not name or not speech:
        raise SystemExit("Could not parse device_config.h")
    return name.group(1), speech.group(1)


def ensure_sam_binary() -> None:
    if SAM_BIN.exists():
        return
    subprocess.check_call(["make", "-C", str(SAM_DIR)])


def sam_render(text: str, out_wav: Path) -> None:
    ensure_sam_binary()
    try:
        subprocess.run(
            [str(SAM_BIN), text, str(out_wav)],
            check=True,
            capture_output=True,
            text=True,
        )
    except subprocess.CalledProcessError as exc:
        stderr = exc.stderr.strip() if exc.stderr else ""
        stdout = exc.stdout.strip() if exc.stdout else ""
        detail = stderr or stdout or f"exit code {exc.returncode}"
        raise SystemExit(f"SAM render failed for '{text}': {detail}") from exc
    if not out_wav.exists() or out_wav.stat().st_size == 0:
        raise SystemExit(f"SAM render produced no output for '{text}'")


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
        "speech_boot_en": f"BOOTING UP SYSTEMS. I AM {device_speech}. NO THREATS FOUND.",
        "speech_panic_en": "PANIC! PANIC! PANIC!",
        "speech_overload_en": (
            f"PANIC OVERLOAD. {device_speech} IS SCARED. SHUTTING DOWN SYSTEM."
        ),
        "speech_boot_zh": (
            f"SHEE TONG CHEE DONG. WOH SHUR {device_speech}. WAY JYEN WAY SHYE."
        ),
        "speech_panic_zh": "KONG HWANG! KONG HWANG! KONG HWANG!",
        "speech_overload_zh": (
            f"KONG HWANG GWO ZAI. {device_speech} HIGH PAH LUH. JUNG ZAI GWAN JEE."
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

    for symbol, text in phrases.items():
        wav_path = build_dir / f"{symbol}.wav"
        sam_render(text, wav_path)
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
