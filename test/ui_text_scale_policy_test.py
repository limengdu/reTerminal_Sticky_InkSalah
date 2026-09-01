#!/usr/bin/env python3

import pathlib
import re


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def main() -> None:
    project_root = pathlib.Path(__file__).resolve().parents[1]
    canvas_header = (project_root / "src/ui/canvas.h").read_text(
        encoding="utf-8"
    )
    canvas_source = (project_root / "src/ui/canvas.cpp").read_text(
        encoding="utf-8"
    )
    require(
        "kMinimumTextScale = 2U" in canvas_header,
        "Canvas must define two-times text as the project minimum.",
    )
    require(
        "std::max<uint8_t>(scale, kMinimumTextScale)" in canvas_source,
        "Canvas must enforce the minimum text scale at runtime.",
    )

    drawing_sources = "\n".join(
        path.read_text(encoding="utf-8")
        for path in (project_root / "src/ui").glob("*.cpp")
    )
    forbidden_patterns = (
        r"\b(?:draw_text|draw_centered|centered)\s*\([^;]*?,\s*1U?\s*(?:,|\))",
        r"\bbutton\s*\([^;]*?,\s*(?:true|false),\s*1U?\s*\)",
        r"\?\s*[0-9]+U?\s*:\s*1U?",
    )
    for pattern in forbidden_patterns:
        require(
            re.search(pattern, drawing_sources, re.DOTALL) is None,
            "UI drawing code must not request the retired one-times text scale.",
        )


if __name__ == "__main__":
    main()
