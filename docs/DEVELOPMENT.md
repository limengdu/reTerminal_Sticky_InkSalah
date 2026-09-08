# Development guide

This guide covers local builds, host-side tests, UI rendering, and physical-device verification for InkSalah.

## Toolchain

- PlatformIO Core 6.x
- Platform package `espressif32@6.11.0`
- ESP-IDF 5.4.1
- C++17
- Python 3
- A desktop C++ compiler for host-side render and logic tests

PlatformIO downloads the managed Espressif Button component declared in `src/idf_component.yml`.

## Build profiles

| Environment | Purpose | Logging |
| --- | --- | --- |
| `sticky-salah-release` | Daily use and release candidates | Warnings and essential lifecycle events |
| `sticky-salah-debug` | Hardware and interaction diagnosis | Detailed boot, RTC, display, and input events |

Build both profiles:

```bash
pio run -e sticky-salah-release
pio run -e sticky-salah-debug
```

## Host tests

The host suite compiles focused C++ tests and runs source-policy checks. It does not require an ESP32 or network connection.

```bash
bash tools/run_host_tests.sh
```

Expected result:

```text
[host-test] PASS 30/30
```

Coverage includes solar calculations, Hanafi Asr, Qiyam, cross-midnight windows, settings storage policies, touch gestures, Wi-Fi parsing and scan models, captive portal behavior, location policies, screen orientation, retained display contents across orientation wakes, deep-sleep indicators, and UI layout safeguards.

`test/prayer_sleep_display_test.py` compiles the production sleep function and unchanged-orientation branch with host-side board hooks. It checks retained panel pixels, repeated wakes in all four rotations, normal crescent rendering, wake routing, and peripheral error paths. The display driver and electrical wake behavior are verified on hardware.

`test/prayer_status_bar_test.cpp` measures the actual rendered bounds of every header element in all four rotations. It covers short and long city names, accented glyphs, multiple method labels, charge values, icon padding, and shortcut boundaries. Settings-page tests cover direct Location entry, first-time entry, and the Location-to-home back path.

`test/canvas_round_rect_test.cpp` verifies the filled and outlined contours against a pixel-center geometry model. Coverage includes the 50-, 54-, and 66-pixel capsule heights, odd and even dimensions, small controls, radius limits, empty shapes, grayscale colors, screen-edge clipping, and all four rotations. Adding a same-color outline to a filled shape preserves its pixels and bounds.

`test/prayer_pages_test.cpp` also checks the complete two-pixel countdown border in portrait and landscape, including six-row, seven-row, and between-prayer states in all four rotations.

## Device UI preview

Compile the preview tool from the project root:

```bash
g++ -std=c++17 \
  -Isrc/prayer -Isrc/storage -Isrc/ui \
  tools/render_ui_preview.cpp \
  src/prayer/prayer_time.cpp \
  src/storage/settings_model.cpp \
  src/ui/arabic_prayer_assets.cpp \
  src/ui/islamic_background_asset.cpp \
  src/ui/islamic_landscape_background_asset.cpp \
  src/ui/islamic_setup_background_asset.cpp \
  src/ui/canvas.cpp src/ui/font.cpp \
  src/ui/prayer_pages.cpp \
  src/ui/prayer_settings_pages.cpp \
  src/ui/sleep_indicator.cpp \
  src/ui/battery_status_render.cpp \
  src/ui/battery_status_overlay_theme.cpp \
  -o /tmp/inksalah-ui-preview
```

Render representative screens:

```bash
/tmp/inksalah-ui-preview portrait schedule /tmp/inksalah-portrait.pgm
/tmp/inksalah-ui-preview landscape schedule /tmp/inksalah-landscape.pgm
/tmp/inksalah-ui-preview portrait schedule-no-qiyam /tmp/inksalah-no-qiyam.pgm
/tmp/inksalah-ui-preview landscape schedule-no-qiyam-sleep /tmp/inksalah-sleep.pgm
/tmp/inksalah-ui-preview portrait schedule-long-header /tmp/inksalah-long-header.pgm
/tmp/inksalah-ui-preview portrait schedule-between /tmp/inksalah-between.pgm
/tmp/inksalah-ui-preview landscape schedule-night /tmp/inksalah-night.pgm
/tmp/inksalah-ui-preview portrait schedule-maghrib /tmp/inksalah-maghrib.pgm
/tmp/inksalah-ui-preview portrait setup /tmp/inksalah-setup.pgm
/tmp/inksalah-ui-preview portrait setup-return /tmp/inksalah-setup-return.pgm
/tmp/inksalah-ui-preview portrait settings-main /tmp/inksalah-settings.pgm
/tmp/inksalah-ui-preview portrait settings-location /tmp/inksalah-location.pgm
/tmp/inksalah-ui-preview portrait settings-calculation /tmp/inksalah-calculation.pgm
/tmp/inksalah-ui-preview portrait settings-methods /tmp/inksalah-methods.pgm
/tmp/inksalah-ui-preview portrait settings-adjustments /tmp/inksalah-adjustments.pgm
/tmp/inksalah-ui-preview portrait settings-editor /tmp/inksalah-editor.pgm
```

The renderer calls the production canvas, page functions, and battery painter with a sample battery reading of 100% and external power present. A preview therefore catches layout regressions in the actual firmware UI path.

## Typography and artwork

The main UI uses native-size Roboto glyphs in regular and bold weights. The minimum Latin text size is 20 pixels. `Canvas::draw_text_sized` draws a requested size, while `Canvas::text_width` measures the same proportional glyph advances. Settings buttons share their geometry with the touch action mapper. Static Arabic labels are shaped into connected word bitmaps before compilation.

The build consumes the committed font and artwork arrays directly. Asset regeneration uses Python with Pillow and, for PDF extraction, pypdf plus the Poppler `pdftoppm` command. Arabic-label regeneration uses Swift and the macOS Geeza Pro font.

```bash
python3 -m venv /tmp/inksalah-assets-venv
/tmp/inksalah-assets-venv/bin/python -m pip install Pillow==12.3.0 pypdf==6.10.0
/tmp/inksalah-assets-venv/bin/python tools/generate_roboto_font.py
/tmp/inksalah-assets-venv/bin/python tools/generate_inksalah_assets.py
swift tools/generate_arabic_prayer_assets.swift src/ui/arabic_prayer_assets.cpp
```

To extract artwork from the original design again, run `tools/extract_inksalah_pdf_assets.py` with the path to `INKSALAH.pdf` as its argument, then regenerate the artwork arrays. The source fingerprint and asset provenance are recorded in `assets/ui/PDF_SOURCE.md` and `assets/fonts/README.md`.

The UI flow starts with `prayer_page_render_schedule`, which draws the background, date header, current window, and six or seven time rows. `prayer_settings_render` draws the selected settings page; `prayer_settings_action_at` maps a tap to an action for `device_settings.cpp` to apply. The gear shortcut and bottom-edge swipe open the settings homepage. `prayer_page_header_action_at` maps a logical tap to `Settings`, `Location`, or `None`; the location result travels through `wait_for_prayer_input` and `run_settings` to the initial page of `device_settings_run`. `prayer_settings_entry_page` selects the homepage for first-time setup and Location for the daily-use location shortcut. Returning from Wi-Fi setup opens the settings homepage. All settings screens retain their portrait layout and existing save/back behavior.

The prayer renderer selects its status-bar layout on the canvas. `battery_status_render` receives the measured charge and external-power state, then paints the matching battery area; `sleep_indicator_render` uses the neighboring crescent area. Clearing the canvas restores the regular status layout for setup and settings pages. The status painters share their geometry through `status_bar_layout.h`.

The header axis is `kPrayerStatusCenterY = 38`. `font_text_centered_y` receives text, font size, axis, and weight, then returns the drawing origin based on occupied glyph rows. `prayer_status_bitmap_y` returns the equivalent origin for an icon using pixels inside its visible width. Whole-pixel placement keeps the center within half a pixel of the axis for both odd- and even-height shapes. The location and settings touch targets span y=8 through y=67, meet at x=308 in portrait and x=558 in landscape, and retain the existing artwork sizes. Their geometry also defines the available header text widths.

See the [native status-bar rendering](images/ui-review/status-bar-alignment.png) for both orientations with the sleep crescent visible.

The landscape background places two uniformly scaled copies of the source arch. `date_header` centers the calendar above the two content columns, `hero` composes the current prayer and countdown, and `times` right-aligns each English/Arabic label group beside its time badge. Seven-row schedules use a tighter row rhythm while keeping Latin text at least 20 pixels. Arabic row labels use 20-pixel type; hero labels use native 28-pixel portrait and 40-pixel landscape bold type.

### UI verification

1. Run the host suite and build both profiles; expect `PASS 30/30` and `SUCCESS`.
2. Render the prayer page in both orientations, with Qiyam on and off. Every time has an AM/PM label; the seventh row remains inside the screen.
3. Render `settings-main-first-time`: the bottom row contains one Save & Start action. Render `settings-main`: Save & Exit and Discard & Exit are separate actions.
4. On the device, tap the gear, open each settings category, and test the new button positions. In Minute Adjustments, toggle Show Qiyam and save; confirm the row count changes.
5. Open Wi-Fi Setup and tap Back. Expect a return to native settings with the saved network available.
6. Test midnight, noon, long city and method names, and an Isha window ending the following day. Rotate the device, then leave it idle for three minutes; the small sleep crescent should appear beside the battery against the black header.

### Header shortcuts

On a configured device, verify both portrait and landscape prayer pages:

1. Inspect the battery, charge digits, crescent, location pin, city, gear, and method. Their visible vertical centers align along the same horizontal axis; use the sleep preview or wait three minutes to include the crescent.
2. Tap the gear and the padded space immediately above, below, and beside it. Each tap opens the native settings homepage.
3. Tap the location pin, then the city name. Each opens Location directly with the current saved values.
4. Change a coordinate and swipe right from the left edge. The settings homepage retains the draft; save to apply it or discard to restore the original values. A downward swipe from Location keeps the existing validate-and-save behavior.
5. Tap near the boundary between location and method, then on the date, battery, and crescent areas. The two header targets remain distinct, and areas outside them retain their existing behavior.
6. After entering through Location, return to the homepage and open Wi-Fi setup. Its Back action returns to the settings homepage. First-time setup still starts at the homepage with Save & Start.
7. Check battery values 0 and 100, charging and unplugged states, and unavailable readings. The battery and crescent occupy separate areas, and the city and method remain readable after a battery update.

Host previews verify rendering and hit regions. Touch responsiveness, contrast, waveform behavior, and ghosting are verified on the physical display.

### Rounded controls

`Canvas::fill_round_rect` and `Canvas::draw_round_rect` take the top-left position, width, height, radius, and color and draw into the existing canvas buffer. Their shared `rounded_rect_rows` helper produces mirrored row insets from pixel-center distances. Filling paints each complete row; outlining paints the boundary pixels of the same shape. The drawing methods return no value. A radius of zero draws a rectangle, and a radius up to half the shorter side produces rounded corners or a capsule.

The prayer renderer reaches these methods through `time_range` and the countdown border; settings reach them through `button`, `ui::pill`, and cards. The two-pixel countdown border is a black outer capsule with a white inner capsule inset by two pixels, followed by its text and icon. Control positions, text, touch targets, and display refresh policy use their existing definitions.

On the device, open settings and inspect Save & Exit and Discard & Exit, then inspect the prayer time capsule and countdown border in both orientations. The straight edges meet the curves continuously, and the shapes remain within their assigned size. Also check Save & Start during first-time setup, Location controls, and the coordinate keypad. Native [rounded-control details](images/ui-review/rounded-controls.png) show the rendered edges at four-times integer magnification.

## Captive portal preview

Start the local preview server:

```bash
python3 tools/preview_setup_portal.py --port 8769
```

Open `http://127.0.0.1:8769/`. The preview provides deterministic nearby-network data and uses the HTML embedded in the firmware source.

## Source flow

The program starts in `src/main.cpp` and enters the application cycle in `src/app/prayer_app.cpp`:

1. initialize NVS and board power;
2. initialize display, RTC, battery, touch, buzzer, and IMU;
3. load and migrate saved settings;
4. run Wi-Fi setup when credentials are absent;
5. synchronize time when a network is available;
6. calculate adjacent prayer days locally;
7. resolve the current prayer window;
8. render the page for the stable device orientation;
9. monitor touch and orientation until the next refresh or idle timeout; and
10. arm timer, button, and orientation wake sources before deep sleep.

On the initial cycle of an orientation wake, an unchanged direction selects `SleepDisplayMode::PreservePanel`. `enter_deep_sleep` receives the canvas, timer interval in microseconds, and display mode; it prepares wake sources, powers down peripherals, and enters deep sleep without returning. `PreservePanel` keeps the existing ePaper image, while the default `RefreshCanvas` mode publishes the rendered page with its sleep crescent. Timer and button wakes continue through page rendering.

## Physical-device verification

Use a release build for final behavior and a debug build when logs are required.

### Main flow

1. Erase and upload only when testing first-time setup.
2. Connect a phone to the setup access point.
3. Select a scanned network, save it, and finish native device settings.
4. Compare the displayed boundaries with a trusted timetable using the same coordinates, UTC offset, calculation method, and Asr school.
5. Swipe up to settings, edit and save each settings category, and confirm the prayer page updates.
6. Rotate through portrait and landscape and confirm one clean transition per settled orientation.
7. Leave the device untouched for three minutes and confirm deep sleep, then test timer, top-button, and physical-rotation wake-up.

### Retained display after orientation wake

Start with saved Wi-Fi and prayer settings on a flashed device. Repeat these checks in portrait and landscape, on battery and external power:

1. Leave the prayer page idle for three minutes. Confirm the complete page remains visible and the crescent appears.
2. Move the sleeping device and return it to the same screen direction. When the log reports `input=orientation_wakeup state=unchanged`, expect `sleep=display action=preserve result=ok` followed by `power=deep_sleep`. The existing page and crescent remain visible throughout this return to sleep.
3. Repeat the same-direction wake several times. Each matching log sequence preserves the complete page.
4. Rotate the sleeping device to the other screen direction and hold it steady. Confirm the page renders in that direction and settings open normally with an upward swipe.
5. Wake with the top button, then separately allow a timer wake at the logged interval. Confirm the current prayer page renders and the normal three-minute idle behavior resumes.

The matching unchanged-direction branch performs zero refreshes. Normal page transitions retain their existing refresh modes; inspect the physical display for image retention after repeated transitions.

### Edge cases

- hidden or open Wi-Fi network;
- wrong Wi-Fi password followed by a valid retry;
- captive portal cancellation back to existing Wi-Fi;
- negative longitude and fractional coordinates;
- UTC offsets using 15-, 30-, and 45-minute increments;
- Standard and Hanafi Asr comparison;
- Qiyam display on and off across midnight;
- extreme northern and southern latitudes;
- upgrade from an older settings blob;
- battery-only and externally powered operation.

Record the firmware commit, configuration profile, coordinates, date, expected timetable, serial log, and photos for every release candidate.

## Coding conventions

- Follow the existing module and naming style.
- Keep runtime strings and logs in English.
- Add bilingual English/Chinese comments only where a function or complex transformation needs explanation.
- Keep the prayer engine independent from network clients.
- Add a minimal failing regression test before fixing a defect.
- Run host tests and both PlatformIO builds before opening a pull request.
