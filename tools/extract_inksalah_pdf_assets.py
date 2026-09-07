#!/usr/bin/env python3
"""Extract the original design's artwork with its PDF placement preserved."""
import argparse
import re
import subprocess
import tempfile
from pathlib import Path
from pypdf import PdfReader, PdfWriter
from pypdf.generic import DecodedStreamObject, NameObject

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("pdf", type=Path)
    args = parser.parse_args()
    reader = PdfReader(args.pdf)
    destination = ROOT / "assets/ui"
    with tempfile.TemporaryDirectory(prefix="inksalah-art-") as temporary:
        for page_number, marker, name in ((0, 183, "setup-original"), (1, 211, "prayer-original")):
            writer = PdfWriter()
            page = writer.add_page(reader.pages[page_number])
            data = page.get_contents().get_data()
            stop = re.search(rb"/P\s*<<\s*/MCID\s+" + str(marker).encode() + rb"\b", data)
            if stop is None:
                raise ValueError("The PDF does not match the INKSALAH design revision")
            stream = DecodedStreamObject()
            stream.set_data(data[:stop.start()] + b" Q\n")
            page[NameObject("/Contents")] = writer._add_object(stream)
            intermediate = Path(temporary) / f"{name}.pdf"
            writer.write(intermediate)
            subprocess.run(["pdftoppm", "-r", "72", "-singlefile", "-png", str(intermediate),
                            str(destination / name)], check=True)
    subprocess.run(["pdftoppm", "-f", "3", "-l", "3", "-r", "72", "-x", "0", "-y", "0",
                    "-W", "480", "-H", "120", "-singlefile", "-png", str(args.pdf),
                    str(destination / "lanterns-source")], check=True)
    print("Extracted original PDF artwork at 480 x 800 display coordinates")


if __name__ == "__main__":
    main()
