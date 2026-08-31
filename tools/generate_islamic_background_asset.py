#!/usr/bin/env python3

import argparse
import shutil
import subprocess
from pathlib import Path


WIDTH = 480
HEIGHT = 800
THRESHOLD = 158


def render_grayscale(source: Path) -> bytes:
    ffmpeg = shutil.which("ffmpeg")
    if ffmpeg is None:
        raise SystemExit("ffmpeg is required to regenerate the UI background")
    command = [
        ffmpeg,
        "-v",
        "error",
        "-i",
        str(source),
        "-vf",
        f"scale={WIDTH}:{HEIGHT}:flags=lanczos,format=gray",
        "-f",
        "rawvideo",
        "pipe:1",
    ]
    result = subprocess.run(command, check=True, capture_output=True)
    expected_size = WIDTH * HEIGHT
    if len(result.stdout) != expected_size:
        raise SystemExit(
            f"expected {expected_size} grayscale pixels, got {len(result.stdout)}"
        )
    return result.stdout


def pack_bitmap(pixels: bytes) -> bytes:
    stride = (WIDTH + 7) // 8
    packed = bytearray(stride * HEIGHT)
    for row in range(HEIGHT):
        for column in range(WIDTH):
            if pixels[row * WIDTH + column] < THRESHOLD:
                packed[row * stride + column // 8] |= 1 << (7 - column % 8)
    return bytes(packed)


def format_bytes(data: bytes) -> str:
    lines = []
    for start in range(0, len(data), 12):
        values = ", ".join(f"0x{value:02X}" for value in data[start : start + 12])
        lines.append(f"    {values},")
    return "\n".join(lines)


def write_cpp(output: Path, packed: bytes) -> None:
    source = f'''#include "islamic_background_asset.h"

namespace {{

constexpr uint8_t kIslamicPrayerBackground[] = {{
{format_bytes(packed)}
}};

}}  // namespace

IslamicBackgroundAsset islamic_prayer_background_asset()
{{
    return {{{WIDTH}U,
            {HEIGHT}U,
            {(WIDTH + 7) // 8}U,
            kIslamicPrayerBackground}};
}}
'''
    output.write_text(source, encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Generate the 1-bit Islamic prayer background C++ asset."
    )
    parser.add_argument("source", type=Path)
    parser.add_argument("output", type=Path)
    arguments = parser.parse_args()
    write_cpp(arguments.output, pack_bitmap(render_grayscale(arguments.source)))


if __name__ == "__main__":
    main()
