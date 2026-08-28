#!/usr/bin/env python3

import pathlib
import re


def load_setup_page(project_root: pathlib.Path) -> str:
    source = (
        project_root / "src" / "network" / "provisioning_portal.cpp"
    ).read_text(encoding="utf-8")
    match = re.search(
        r'constexpr char kSetupPage\[\] = R"html\((.*?)\)html";',
        source,
        re.DOTALL,
    )
    if match is None:
        raise AssertionError("Embedded setup page was not found.")
    return match.group(1)


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def main() -> None:
    project_root = pathlib.Path(__file__).resolve().parents[1]
    page = load_setup_page(project_root)

    require(
        'id="network_list"' in page,
        "Nearby Wi-Fi networks must use a visible selection list.",
    )
    require(
        re.search(r'<input id="ssid" name="ssid" type="text"', page)
        is not None,
        "The SSID field must remain visible and editable.",
    )
    require(
        "function selectNetwork(ssid)" in page
        and "q('ssid').value=ssid" in page,
        "Selecting a scanned network must fill the SSID field.",
    )
    require(
        "loadOffsets();loadLocations();scanNetworks();" in page,
        "The page must scan nearby Wi-Fi networks on startup.",
    )


if __name__ == "__main__":
    main()
