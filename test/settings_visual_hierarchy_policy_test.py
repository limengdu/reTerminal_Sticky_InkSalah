#!/usr/bin/env python3

import pathlib


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def main() -> None:
    project_root = pathlib.Path(__file__).resolve().parents[1]
    canvas_header = (project_root / "src/ui/canvas.h").read_text(
        encoding="utf-8"
    )
    settings_page = (
        project_root / "src/ui/prayer_settings_pages.cpp"
    ).read_text(encoding="utf-8")
    preview_source = (
        project_root / "tools/render_ui_preview.cpp"
    ).read_text(encoding="utf-8")

    require(
        "enum class TextWeight" in canvas_header,
        "Canvas must expose regular and bold text weights.",
    )
    require(
        'state.first_time ? "SAVE & START" : "SAVE & EXIT"'
        in settings_page,
        "First-time setup must use a start action without an exit label.",
    )
    require(
        'if (!state.first_time)' in settings_page
        and '"DISCARD & EXIT"' in settings_page,
        "Discard and exit must only be rendered outside first-time setup.",
    )
    require(
        '"CANCEL"' not in settings_page and '"BACK"' in settings_page,
        "Nested editors must use a back action instead of an exit-like label.",
    )
    require(
        "settings-main-first-time" in preview_source,
        "The preview tool must render first-time setup as a distinct state.",
    )


if __name__ == "__main__":
    main()
