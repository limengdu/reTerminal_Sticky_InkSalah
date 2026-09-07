#!/usr/bin/env python3
"""Prepare monochrome backgrounds and native-sized icons for the PDF design."""
from pathlib import Path
from PIL import Image, ImageOps

ROOT = Path(__file__).resolve().parents[1]
ASSETS = ROOT / "assets/ui"


def scaled(image, width):
    return image.resize((width, round(image.height * width / image.width)), Image.Resampling.LANCZOS)


def packed(image, symbol):
    image = image.convert("L").point(lambda value: 255 if value >= 140 else 0, mode="1")
    stride = (image.width + 7) // 8
    data = bytes(value ^ 255 for value in image.tobytes())
    lines = [f"constexpr uint8_t {symbol}[] = {{"]
    for start in range(0, len(data), 24):
        lines.append("    " + ",".join(str(value) for value in data[start:start + 24]) + ",")
    lines.append("};")
    return "\n".join(lines), f"{{{image.width}U, {image.height}U, {stride}U, {symbol}}}"


def emit(filename, entries):
    arrays, functions = [], []
    for symbol, function, image in entries:
        array, value = packed(image, symbol)
        arrays.append(array)
        functions.append(f"IslamicBackgroundAsset {function}()\n{{\n    return {value};\n}}")
        image.save(ASSETS / f"{function}.png")
    content = '#include "islamic_background_asset.h"\n\nnamespace {\n'
    content += "\n\n".join(arrays) + "\n}  // namespace\n\n" + "\n\n".join(functions) + "\n"
    (ROOT / "src/ui" / filename).write_text(content)


def main():
    arch = Image.open(ASSETS / "arch-source.png").convert("L")
    portrait = Image.open(ASSETS / "prayer-original.png").convert("L")
    setup = Image.open(ASSETS / "setup-original.png").convert("L")
    landscape = Image.new("L", (800, 480), 0)
    landscape.paste(scaled(arch, 384), (20, 8))
    landscape.paste(scaled(arch, 384), (404, 8))
    emit("islamic_background_asset.cpp", [("kPrayer", "islamic_prayer_background_asset", portrait)])
    emit("islamic_landscape_background_asset.cpp", [("kLandscape", "islamic_landscape_background_asset", landscape)])
    lanterns = Image.open(ASSETS / "lanterns-source.png").convert("L")
    book = ImageOps.invert(Image.open(ASSETS / "book-mask.png").convert("L"))
    gear = ImageOps.invert(Image.open(ASSETS / "gear-mask.png").convert("L"))
    emit("islamic_setup_background_asset.cpp", [
        ("kSetup", "islamic_setup_background_asset", setup),
        ("kLanterns", "inksalah_lantern_asset", lanterns),
        ("kBook", "inksalah_book_asset", scaled(book, 34)),
        ("kGear", "inksalah_gear_asset", scaled(gear, 28)),
        ("kStatusGear", "inksalah_status_gear_asset", scaled(gear, 22)),
        ("kLocation", "inksalah_location_asset", scaled(Image.open(ASSETS / "location-source.png"), 18)),
        ("kClock", "inksalah_clock_asset", scaled(Image.open(ASSETS / "clock-source.png"), 24)),
    ])
    print("Generated INKSALAH artwork at native display sizes")


if __name__ == "__main__":
    main()
