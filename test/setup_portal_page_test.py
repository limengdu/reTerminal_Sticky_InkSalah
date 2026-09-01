#!/usr/bin/env python3

import pathlib
import re


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def main() -> None:
    project_root = pathlib.Path(__file__).resolve().parents[1]
    source = (project_root / "src/network/provisioning_portal.cpp").read_text(
        encoding="utf-8"
    )
    match = re.search(
        r'constexpr char kSetupPage\[\] = R"html\((.*?)\)html";',
        source,
        re.DOTALL,
    )
    require(match is not None, "The embedded Wi-Fi page must exist.")
    page = match.group(1)
    require('id="network_list"' in page,
            "Nearby Wi-Fi networks must use a visible selection list.")
    require(re.search(r'<input id="ssid" name="ssid" type="text"', page) is not None,
            "The SSID field must remain visible and editable.")
    require("function selectNetwork(s)" in page and "q('ssid').value=s" in page,
            "Selecting a scanned network must fill the SSID field.")
    require("scanNetworks();" in page,
            "The page must scan nearby Wi-Fi networks on startup.")
    require('action="/save_wifi"' in page,
            "The form must submit Wi-Fi credentials only.")
    require("Continue setup on the Sticky screen" in page,
            "The page must hand prayer setup back to the device screen.")
    require("location.href='/settings'" not in page and "/save_settings" not in source,
            "The captive portal must not contain a prayer-settings route.")
    for field in ('name="country"', 'name="method"', 'name="school"',
                  'name="latitude"', 'name="adj_fajr"'):
        require(field not in page,
                f"The Wi-Fi page must not contain prayer field {field}.")
    require("kPrayerSettingsPage" not in source,
            "Prayer settings must be native device pages, not embedded HTML.")


if __name__ == "__main__":
    main()
