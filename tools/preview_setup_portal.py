#!/usr/bin/env python3

import argparse
import http.server
import json
import pathlib
import re


def load_setup_page(project_root: pathlib.Path) -> bytes:
    source = (
        project_root / "src" / "network" / "provisioning_portal.cpp"
    ).read_text(encoding="utf-8")
    match = re.search(
        r'constexpr char kSetupPage\[\] = R"html\((.*?)\)html";',
        source,
        re.DOTALL,
    )
    if match is None:
        raise RuntimeError("Embedded setup page was not found.")
    return match.group(1).encode("utf-8")


def load_location_catalog(project_root: pathlib.Path) -> bytes:
    source = (
        project_root / "src" / "location" / "location_catalog.cpp"
    ).read_text(encoding="utf-8")
    countries = []
    pattern = re.compile(
        r'\b([CM])\("([A-Z]{2})", "([^"]+)"'
        r'(?:, (\d+)U, "([^"]*)")?\),'
    )
    for kind, code, name, method, cities in pattern.findall(source):
        countries.append(
            {
                "code": code,
                "name": name,
                "method": int(method) if kind == "M" else 3,
                "cities": cities.split("|") if cities else [],
            }
        )
    return json.dumps({"countries": countries}).encode("utf-8")


def mock_wifi_scan() -> bytes:
    return json.dumps(
        {
            "networks": [
                {"ssid": "Home Wi-Fi", "rssi": -42, "secure": True},
                {"ssid": "Masjid Guest", "rssi": -63, "secure": True},
                {"ssid": "Cafe Open", "rssi": -78, "secure": False},
            ]
        }
    ).encode("utf-8")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", type=int, default=8765)
    args = parser.parse_args()
    project_root = pathlib.Path(__file__).resolve().parents[1]
    class SetupPreviewHandler(http.server.BaseHTTPRequestHandler):
        def do_GET(self) -> None:
            if self.path == "/api/wifi/scan":
                body = mock_wifi_scan()
                content_type = "application/json; charset=utf-8"
            elif self.path == "/api/locations":
                body = load_location_catalog(project_root)
                content_type = "application/json; charset=utf-8"
            else:
                body = load_setup_page(project_root)
                content_type = "text/html; charset=utf-8"
            self.send_response(200)
            self.send_header("Content-Type", content_type)
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)

        def do_POST(self) -> None:
            length = int(self.headers.get("Content-Length", "0"))
            self.rfile.read(length)
            body = json.dumps(
                {
                    "ok": True,
                    "message": "Wi-Fi and prayer time data were verified.",
                }
            ).encode("utf-8")
            self.send_response(200)
            self.send_header("Content-Type", "application/json; charset=utf-8")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)

        def log_message(self, message: str, *args: object) -> None:
            print("preview=" + (message % args), flush=True)

    server = http.server.ThreadingHTTPServer(
        ("127.0.0.1", args.port), SetupPreviewHandler
    )
    print(f"Setup portal preview: http://127.0.0.1:{args.port}", flush=True)
    server.serve_forever()


if __name__ == "__main__":
    main()
