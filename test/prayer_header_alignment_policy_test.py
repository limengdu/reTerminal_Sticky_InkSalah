#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]
source = (root / "src/ui/prayer_pages.cpp").read_text()
header = source[source.index("void date_header("):source.index("void time_badge(")]
assert "const int center = canvas.width() / 2" in header
assert "ui::fitted_centered(canvas, center," in header
assert "canvas.draw_text" not in header
assert "prayer_hijri_month_name" in header
style = (root / "src/ui/ui_style.h").read_text()
assert "Canvas::text_width(text, size, weight) / 2" in style
assert '0xC0U' in style
assets = (root / "tools/generate_inksalah_assets.py").read_text()
assert 'ASSETS / "prayer-original.png"' in assets
assert 'ASSETS / "setup-original.png"' in assets
assert 'lanterns)' in assets
assert 'landscape.paste(scaled(arch, 384), (20, 8))' in assets
assert 'landscape.paste(scaled(arch, 384), (404, 8))' in assets
assert 'const int city_center = center' in header
assert 'inksalah_location_asset()' in header
assert 'inksalah_status_gear_asset()' in header
assert 'canvas.set_prayer_header(true)' in source
assert '"Next: %s' not in source
