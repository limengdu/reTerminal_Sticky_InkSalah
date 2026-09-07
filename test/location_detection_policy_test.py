#!/usr/bin/env python3

import pathlib


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def main() -> None:
    project_root = pathlib.Path(__file__).resolve().parents[1]
    wifi_source = (project_root / "src/network/wifi_service.cpp").read_text(
        encoding="utf-8"
    )
    settings_pages = (
        project_root / "src/ui/prayer_settings_pages.cpp"
    ).read_text(encoding="utf-8")

    require(
        "if (!settings_wifi_valid(settings))" in wifi_source,
        "Saved Wi-Fi credentials must be enough to start network location.",
    )
    require(
        "if (wifi_service_is_connected())" in wifi_source,
        "Network location must reuse an active saved Wi-Fi connection.",
    )
    require(
        '"Detect from connected Wi-Fi"' in settings_pages,
        "The Location page must identify its automatic network action.",
    )


if __name__ == "__main__":
    main()
