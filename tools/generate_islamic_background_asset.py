#!/usr/bin/env python3

import argparse
import re
import shutil
import subprocess
from pathlib import Path


THRESHOLD = 158


def render_grayscale(source: Path, width: int, height: int) -> bytes:
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
        f"scale={width}:{height}:flags=lanczos,format=gray",
        "-f",
        "rawvideo",
        "pipe:1",
    ]
    result = subprocess.run(command, check=True, capture_output=True)
    expected_size = width * height
    if len(result.stdout) != expected_size:
        raise SystemExit(
            f"expected {expected_size} grayscale pixels, got {len(result.stdout)}"
        )
    return result.stdout


def pack_bitmap(pixels: bytes, width: int, height: int) -> bytes:
    stride = (width + 7) // 8
    packed = bytearray(stride * height)
    for row in range(height):
        for column in range(width):
            if pixels[row * width + column] < THRESHOLD:
                packed[row * stride + column // 8] |= 1 << (7 - column % 8)
    return bytes(packed)


def format_bytes(data: bytes) -> str:
    lines = []
    for start in range(0, len(data), 12):
        values = ", ".join(f"0x{value:02X}" for value in data[start : start + 12])
        lines.append(f"    {values},")
    return "\n".join(lines)


def write_cpp(
    output: Path,
    packed: bytes,
    width: int,
    height: int,
    symbol: str,
    function: str,
) -> None:
    source = f'''#include "islamic_background_asset.h"

namespace {{

constexpr uint8_t {symbol}[] = {{
{format_bytes(packed)}
}};

}}  // namespace

IslamicBackgroundAsset {function}()
{{
    return {{{width}U,
            {height}U,
            {(width + 7) // 8}U,
            {symbol}}};
}}
'''
    output.write_text(source, encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Generate the 1-bit Islamic prayer background C++ asset."
    )
    parser.add_argument("source", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--width", type=int, default=480)
    parser.add_argument("--height", type=int, default=800)
    parser.add_argument("--symbol", default="kIslamicPrayerBackground")
    parser.add_argument("--function", default="islamic_prayer_background_asset")
    arguments = parser.parse_args()
    if arguments.width <= 0 or arguments.height <= 0:
        parser.error("width and height must be positive")
    identifier = re.compile(r"[A-Za-z_][A-Za-z0-9_]*")
    if identifier.fullmatch(arguments.symbol) is None:
        parser.error("symbol must be a valid C++ identifier")
    if identifier.fullmatch(arguments.function) is None:
        parser.error("function must be a valid C++ identifier")
    pixels = render_grayscale(
        arguments.source,
        arguments.width,
        arguments.height,
    )
    write_cpp(
        arguments.output,
        pack_bitmap(pixels, arguments.width, arguments.height),
        arguments.width,
        arguments.height,
        arguments.symbol,
        arguments.function,
    )


if __name__ == "__main__":
    main()
