# Security policy

## Supported version

InkSalah is currently in early development. Security fixes are applied to the latest code on the `main` branch.

## Reporting a vulnerability

Use GitHub's private vulnerability reporting feature for this repository. Include:

- the affected commit or version;
- the hardware and build profile;
- reproducible steps;
- the security impact;
- relevant logs or proof of concept; and
- a suggested mitigation, when available.

Please allow reasonable time for investigation before public disclosure. A maintainer will acknowledge the report, assess its impact, and coordinate a fix and disclosure timeline.

## Sensitive data

InkSalah stores Wi-Fi credentials and configuration in ESP32 NVS. Diagnostic logs and issue reports should use placeholder SSIDs and must not contain real passwords, private keys, access tokens, or personal network information.
