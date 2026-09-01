#!/usr/bin/env python3

import pathlib


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def main() -> None:
    project_root = pathlib.Path(__file__).resolve().parents[1]
    settings_source = (project_root / "src/app/device_settings.cpp").read_text(
        encoding="utf-8"
    )
    app_source = (project_root / "src/app/prayer_app.cpp").read_text(
        encoding="utf-8"
    )

    require(
        "kIdleTimeout" not in settings_source,
        "Native settings must not expire while the user is reading or editing.",
    )
    require(
        "while (true)" in settings_source,
        "Native settings must remain open until an explicit user action.",
    )
    require(
        '"SETTINGS UNAVAILABLE"' not in app_source,
        "An idle settings page must never be presented as a hardware failure.",
    )
    require(
        "StickyButtonEvent::DoubleClick" not in app_source,
        "The AI button must not open native settings.",
    )
    require(
        "StickyVerticalSwipe::UpFromBottom" in app_source,
        "The schedule page must open settings with an upward edge swipe.",
    )
    require(
        "StickyVerticalSwipe::DownFromTop" in settings_source,
        "Native settings must close with a downward edge swipe.",
    )
    require(
        "StickyHorizontalSwipe::RightFromLeft" in settings_source,
        "Native settings must close with a swipe from the left edge toward center.",
    )
    require(
        "restart_after_settings" not in app_source,
        "Closing native settings must return to the schedule without restarting.",
    )
    require(
        '"settings=closed result=%s mode=in_place timezone_changed=%d"'
        in app_source,
        "The in-place settings exit must remain visible in runtime logs.",
    )
    require(
        "sticky_touch_stop();\n                return DeviceSettingsResult::Saved;"
        not in settings_source,
        "Saving settings must keep the active touch task available to the schedule.",
    )


if __name__ == "__main__":
    main()
