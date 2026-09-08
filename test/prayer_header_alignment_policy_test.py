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
assert 'font_text_centered_y(location, 20, kPrayerStatusCenterY, true)' in header
assert 'font_text_centered_y(fitted_method, 20, kPrayerStatusCenterY, true)' in header
assert 'prayer_status_bitmap_y(pin)' in header
assert 'prayer_status_bitmap_y(gear)' in header

app = (root / "src/app/prayer_app.cpp").read_text()
settings = (root / "src/app/device_settings.cpp").read_text()
assert 'canvas.physical_to_logical(press.x, press.y, x, y)' in app
assert 'const PrayerHeaderAction action = prayer_page_header_action_at(' in app
assert 'action == PrayerHeaderAction::Location' in app
assert 'PrayerInputResult::LocationRequested' in app
assert app.count('run_settings(canvas, settings, false, initial_page)') == 2
assert 'device_settings_run(canvas, settings, first_time, initial_page)' in app
assert 'show_setup(canvas, settings);\n            initial_page = DeviceSettingsPage::Main;' in app
assert 'state.page = prayer_settings_entry_page(first_time, initial_page);' in settings
