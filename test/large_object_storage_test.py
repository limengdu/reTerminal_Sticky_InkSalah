#!/usr/bin/env python3

import pathlib
import re


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def main() -> None:
    project_root = pathlib.Path(__file__).resolve().parents[1]
    app_source = (project_root / "src" / "app" / "prayer_app.cpp").read_text(
        encoding="utf-8"
    )
    cache_source = (
        project_root / "src" / "storage" / "prayer_cache.cpp"
    ).read_text(encoding="utf-8")

    require(
        "PrayerSchedule downloaded = {};" not in app_source,
        "The downloaded two-month schedule must not use the main task stack.",
    )
    require(
        re.search(r"heap_caps_calloc\(\s*1U,\s*sizeof\(PrayerSchedule\)", app_source)
        is not None,
        "The downloaded schedule must use zero-initialized heap storage.",
    )
    require(
        "CacheBlob blob = {};" not in cache_source,
        "The persistent cache transfer buffer must not use the main task stack.",
    )
    require(
        re.search(r"heap_caps_calloc\(\s*1U,\s*sizeof\(CacheBlob\)", cache_source)
        is not None,
        "Cache transfers must use zero-initialized heap storage.",
    )


if __name__ == "__main__":
    main()
