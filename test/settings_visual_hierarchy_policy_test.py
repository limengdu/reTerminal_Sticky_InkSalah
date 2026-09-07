#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]
source = (root / "src/ui/prayer_settings_pages.cpp").read_text()
card = source[source.index("void card("):source.index("void footer_status(")]
assert "title, 20, GrayLevel::DarkGray" in card
assert "TextWeight::Bold" in card
assert 'if (state.first_time)' in source and '"Save & Start"' in source
assert '!first_time && kDiscard.contains(x, y)' in source
assert '"Discard & Exit"' in source and '"Back"' in source
assert "inksalah_lantern_asset()" in source
assert "state.method_page, state.first_time" in (root / "src/app/device_settings.cpp").read_text()
assert "settings-main-first-time" in (root / "tools/render_ui_preview.cpp").read_text()
