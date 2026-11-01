#!/usr/bin/env python3
"""Regenerate speech assets and build/upload firmware."""

from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

PIO_INSTALL_HINT = """\
PlatformIO is required to compile firmware but was not found.

Install one of:
  pip install -r requirements.txt
  brew install platformio

Then verify:
  pio --version
  # or: python -m platformio --version
"""


def resolve_pio_command() -> list[str]:
    for candidate in ("pio", "platformio"):
        if shutil.which(candidate):
            return [candidate]

    try:
        subprocess.run(
            [sys.executable, "-m", "platformio", "--version"],
            check=True,
            capture_output=True,
            text=True,
        )
    except (subprocess.CalledProcessError, FileNotFoundError):
        pass
    else:
        return [sys.executable, "-m", "platformio"]

    print(PIO_INSTALL_HINT, file=sys.stderr)
    sys.exit(1)


def main() -> int:
    parser = argparse.ArgumentParser(description="Build PANIC BUTTON firmware for this device name.")
    parser.add_argument("--port", help="Serial port for upload")
    parser.add_argument("--no-upload", action="store_true", help="Compile only")
    parser.add_argument("--env", default="d1_mini", help="PlatformIO environment name")
    args = parser.parse_args()

    subprocess.check_call([sys.executable, str(ROOT / "tools" / "generate_speech_assets.py")])

    cmd = resolve_pio_command() + ["run", "-e", args.env]
    if args.port and not args.no_upload:
        cmd.extend(["-t", "upload", "--upload-port", args.port])
    subprocess.check_call(cmd, cwd=ROOT)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
