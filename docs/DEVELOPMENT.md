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
[host-test] PASS 26/26
```

Coverage includes solar calculations, Hanafi Asr, Qiyam, cross-midnight windows, settings storage policies, touch gestures, Wi-Fi parsing and scan models, captive portal behavior, location policies, screen orientation, deep-sleep indicators, and UI layout safeguards.

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
  -o /tmp/inksalah-ui-preview
```

Render representative screens:

```bash
/tmp/inksalah-ui-preview portrait schedule /tmp/inksalah-portrait.pgm
/tmp/inksalah-ui-preview landscape schedule /tmp/inksalah-landscape.pgm
/tmp/inksalah-ui-preview portrait schedule-no-qiyam /tmp/inksalah-no-qiyam.pgm
/tmp/inksalah-ui-preview portrait setup /tmp/inksalah-setup.pgm
/tmp/inksalah-ui-preview portrait setup-return /tmp/inksalah-setup-return.pgm
/tmp/inksalah-ui-preview portrait settings-main /tmp/inksalah-settings.pgm
/tmp/inksalah-ui-preview portrait settings-location /tmp/inksalah-location.pgm
/tmp/inksalah-ui-preview portrait settings-calculation /tmp/inksalah-calculation.pgm
/tmp/inksalah-ui-preview portrait settings-methods /tmp/inksalah-methods.pgm
/tmp/inksalah-ui-preview portrait settings-adjustments /tmp/inksalah-adjustments.pgm
/tmp/inksalah-ui-preview portrait settings-editor /tmp/inksalah-editor.pgm
```

The renderer calls the production canvas and page functions. A preview therefore catches layout regressions in the actual firmware UI path.

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

The UI flow starts with `prayer_page_render_schedule`, which draws the background, date header, current window, and six or seven time rows. `prayer_settings_render` draws the selected settings page; `prayer_settings_action_at` maps a tap to an action for `device_settings.cpp` to apply. The gear shortcut and bottom-edge swipe both enter this existing native settings flow.

### UI verification

1. Run the host suite and build both profiles; expect `PASS 26/26` and `SUCCESS`.
2. Render the prayer page in both orientations, with Qiyam on and off. Every time has an AM/PM label; the seventh row remains inside the screen.
3. Render `settings-main-first-time`: the bottom row contains one Save & Start action. Render `settings-main`: Save & Exit and Discard & Exit are separate actions.
4. On the device, tap the gear, open each settings category, and test the new button positions. In Minute Adjustments, toggle Show Qiyam and save; confirm the row count changes.
5. Open Wi-Fi Setup and tap Back. Expect a return to native settings with the saved network available.
6. Test midnight, noon, long city names, and an Isha window ending the following day. Rotate the device, then leave it idle for three minutes; the small sleep crescent should remain visible against the black header.

Host previews verify rendering and hit regions. Touch responsiveness, contrast, waveform behavior, and ghosting are verified on the physical display.

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
