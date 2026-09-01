#!/usr/bin/env python3

import pathlib


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def main() -> None:
    project_root = pathlib.Path(__file__).resolve().parents[1]
    component = (project_root / "src" / "CMakeLists.txt").read_text(
        encoding="utf-8"
    )
    app = (project_root / "src" / "app" / "prayer_app.cpp").read_text(
        encoding="utf-8"
    )
    calculator = (
        project_root / "src" / "prayer" / "prayer_calculator.cpp"
    ).read_text(encoding="utf-8")
    portal = (
        project_root / "src" / "network" / "provisioning_portal.cpp"
    ).read_text(encoding="utf-8")
    device_settings = (
        project_root / "src" / "app" / "device_settings.cpp"
    ).read_text(encoding="utf-8")

    require(
        "prayer/prayer_calculator.cpp" in component,
        "The native prayer calculator must be part of the firmware build.",
    )
    require(
        "services/aladhan_client.cpp" not in component
        and "storage/prayer_cache.cpp" not in component,
        "The firmware build must not include the remote timetable pipeline.",
    )
    require(
        "prayer_calculate_day" in app and "aladhan" not in app.lower(),
        "The wake cycle must calculate prayer times locally.",
    )
    require(
        "settings.latitude" in calculator
        and "settings.longitude" in calculator
        and "settings.utc_offset_minutes" in calculator,
        "Local calculation must use coordinates and the selected UTC offset.",
    )
    require(
        "next_fajr" in calculator
        and "PrayerAdjustment::Qiyam" in calculator,
        "Qiyam must use the next Fajr and support local adjustment.",
    )
    require(
        "kPrayerSettingsPage" not in portal
        and "/save_settings" not in portal,
        "Prayer settings must not be served by the Wi-Fi portal.",
    )
    require(
        "sticky_touch_init" in device_settings
        and "prayer_settings_action_at" in device_settings
        and "settings_store_save" in device_settings,
        "Prayer settings must run as a native touch flow and persist locally.",
    )


if __name__ == "__main__":
    main()
