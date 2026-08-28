#!/usr/bin/env python3

import argparse
import http.server
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


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", type=int, default=8765)
    args = parser.parse_args()
    project_root = pathlib.Path(__file__).resolve().parents[1]
    class SetupPreviewHandler(http.server.BaseHTTPRequestHandler):
        def do_GET(self) -> None:
            page = load_setup_page(project_root)
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.send_header("Content-Length", str(len(page)))
            self.end_headers()
            self.wfile.write(page)

        def log_message(self, message: str, *args: object) -> None:
            print("preview=" + (message % args), flush=True)

    server = http.server.ThreadingHTTPServer(
        ("127.0.0.1", args.port), SetupPreviewHandler
    )
    print(f"Setup portal preview: http://127.0.0.1:{args.port}", flush=True)
    server.serve_forever()


if __name__ == "__main__":
    main()
