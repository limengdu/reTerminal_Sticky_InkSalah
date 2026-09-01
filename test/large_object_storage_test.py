#!/usr/bin/env python3

import pathlib
def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def main() -> None:
    project_root = pathlib.Path(__file__).resolve().parents[1]
    app_source = (project_root / "src" / "app" / "prayer_app.cpp").read_text(
        encoding="utf-8"
    )
    component_source = (project_root / "src" / "CMakeLists.txt").read_text(
        encoding="utf-8"
    )

    require(
        "PrayerSchedule downloaded" not in app_source,
        "The wake cycle must not allocate a downloaded monthly schedule.",
    )
    require(
        "PrayerSchedule schedule = {};" in app_source,
        "The wake cycle must keep only its locally calculated days.",
    )
    require(
        "storage/prayer_cache.cpp" not in component_source,
        "The firmware build must not reserve a remote timetable cache.",
    )


if __name__ == "__main__":
    main()
