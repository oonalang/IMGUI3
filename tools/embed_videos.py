#!/usr/bin/env python3
"""Regenerate the C++ embedded video headers from the actual input MP4 files."""
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
ASSET_DIR = ROOT / "src/main/jni/Assets"
ITEMS = (
    ("hachiware_login.mp4", "LoginAssets.h", "ethnir_login_video"),
    ("arknight_card.mp4", "LoadingAssets.h", "ethnir_loading_video"),
)

def generate(filename: str, header: str, symbol: str):
    source = ROOT / "media-files" / filename
    data = source.read_bytes()
    if not data:
        raise ValueError(f"Empty clip: {source}")
    path = ASSET_DIR / header
    with path.open("w") as out:
        out.write(f"#pragma once\n#include <cstdint>\ninline const unsigned char {symbol}[] = {{\n")
        for n in range(0, len(data), 20):
            out.write("    " + ", ".join(f"0x{v:02x}" for v in data[n:n+20]) + ",\n")
        out.write(f"}};\ninline const unsigned int {symbol}_len = sizeof({symbol});\n")
    print(f"Embedded {filename}: {len(data):,} bytes into {header}")

if __name__ == "__main__":
    for entry in ITEMS:
        generate(*entry)
