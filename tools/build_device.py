#!/usr/bin/env python3
"""Regenerate speech assets and build/upload firmware."""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def main() -> int:
    parser = argparse.ArgumentParser(description="Build PANIC BUTTON firmware for this device name.")
    parser.add_argument("--port", help="Serial port for upload")
    parser.add_argument("--no-upload", action="store_true", help="Compile only")
    parser.add_argument("--env", default="d1_mini", help="PlatformIO environment name")
    args = parser.parse_args()

    subprocess.check_call([sys.executable, str(ROOT / "tools" / "generate_speech_assets.py")])

    cmd = ["pio", "run", "-e", args.env]
    if args.port and not args.no_upload:
        cmd.extend(["-t", "upload", "--upload-port", args.port])
    subprocess.check_call(cmd, cwd=ROOT)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
