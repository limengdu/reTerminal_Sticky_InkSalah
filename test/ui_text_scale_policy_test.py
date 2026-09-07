#!/usr/bin/env python3
from pathlib import Path
import ast
import re

root = Path(__file__).resolve().parents[1]
generator = (root / "tools/generate_roboto_font.py").read_text()
sizes = ast.literal_eval(re.search(r"SIZES = (\([^\n]+\))", generator).group(1))
assert min(sizes) >= 20
assert "for weight in (400, 700)" in generator
canvas = (root / "src/ui/canvas.cpp").read_text()
assert "font_face(size, weight == TextWeight::Bold)" in canvas
assert "font_glyph(face, font_next_codepoint(text))" in canvas
assert "kFontWidth" not in canvas
assert "font_text_width(text, size, weight == TextWeight::Bold)" in canvas
assert (root / "assets/fonts/Roboto.ttf").stat().st_size > 100000
assert "SIL OPEN FONT LICENSE" in (root / "assets/fonts/Roboto-OFL.txt").read_text()
