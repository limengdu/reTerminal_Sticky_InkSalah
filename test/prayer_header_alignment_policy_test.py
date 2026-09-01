#!/usr/bin/env python3

import pathlib
import re


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def main() -> None:
    project_root = pathlib.Path(__file__).resolve().parents[1]
    source = (project_root / "src/ui/prayer_pages.cpp").read_text(
        encoding="utf-8"
    )
    portrait_match = re.search(
        r"void draw_portrait_header\(.*?\n\}\n\nvoid draw_timeline_marker",
        source,
        flags=re.DOTALL,
    )
    require(portrait_match is not None, "Portrait header renderer must be present.")
    portrait_header = portrait_match.group(0)
    require(
        "kPageCenterX = 240" in portrait_header
        and portrait_header.count("kPageCenterX,") == 3,
        "Every portrait metadata row must use the physical page center.",
    )
    require(
        "format_source" not in portrait_header
        and "prayer_hijri_month_name" not in portrait_header
        and '"%s / %s"' in portrait_header,
        "Portrait metadata must show only city, Gregorian date, and the compact calculation rule.",
    )

    match = re.search(
        r"void draw_landscape_header\(.*?\n\}\n\nvoid draw_landscape_schedule",
        source,
        flags=re.DOTALL,
    )
    require(match is not None, "Landscape header renderer must be present.")
    header = match.group(0)

    require(
        "kPageCenterX = 400" in header,
        "Landscape date metadata must use the physical page center.",
    )
    require(
        header.count("kPageCenterX,") == 2
        and "kLeftColumnCenterX" not in header
        and "kRightColumnCenterX" not in header,
        "Both landscape metadata rows must use the physical page center.",
    )
    require(
        "canvas.draw_text(" not in header,
        "Landscape header text must not use left-aligned drawing.",
    )

    schedule_match = re.search(
        r"void draw_landscape_schedule\(.*?\n\}\n\nvoid draw_portrait_schedule",
        source,
        flags=re.DOTALL,
    )
    require(schedule_match is not None, "Landscape schedule renderer must be present.")
    schedule = schedule_match.group(0)
    require(
        "draw_timeline_marker" not in schedule
        and "kTimelineX" not in schedule,
        "The central lamp must be the only landscape column divider.",
    )
    require(
        "kEnglishX = 440" in schedule
        and "kArabicCenterX = 575" in schedule
        and "kTimeX = 630" in schedule,
        "Landscape prayer rows must use three stable text columns.",
    )
    require(
        "kFirstCenterY = 250" in schedule
        and "kRowStep = 34" in schedule,
        "Landscape prayer rows must use the available height without crowding.",
    )
    require(
        "draw_centered(canvas, 200, 322, range" in schedule,
        "The current prayer name and its time window must have clear separation.",
    )


if __name__ == "__main__":
    main()
