# Contributing to InkSalah

Thank you for helping improve InkSalah. Focused bug reports, verified prayer-calculation corrections, hardware findings, documentation improvements, and small pull requests are welcome.

## Before opening an issue

Search existing issues and collect enough information to reproduce the behavior. For a device problem, include:

- firmware version or commit;
- release or debug build profile;
- device orientation and power source;
- exact steps leading to the result;
- expected and actual behavior;
- relevant serial log lines; and
- a screen photo when the issue is visual.

For prayer-time differences, also include the date, latitude, longitude, UTC offset, calculation method, Asr school, minute adjustments, and the trusted timetable used for comparison.

## Development workflow

1. Fork the repository and create a focused branch.
2. Make one coherent change.
3. Add or update a regression test.
4. Run the host suite:

   ```bash
   bash tools/run_host_tests.sh
   ```

5. Build both firmware profiles:

   ```bash
   pio run -e sticky-salah-release
   pio run -e sticky-salah-debug
   ```

6. Follow the relevant physical-device checks in `docs/DEVELOPMENT.md`.
7. Open a pull request describing the behavior, evidence, and test results.

## Pull-request scope

- Preserve the separation between Wi-Fi setup and native prayer settings.
- Keep prayer-time calculation available without a network connection.
- Keep credentials and user configuration out of source files and logs.
- Include both portrait and landscape checks for page-layout changes.
- Keep commits and pull requests limited to one understandable purpose.

## Prayer-calculation changes

Prayer-time rules affect religious practice and require clear evidence. A calculation pull request should provide:

- the institution or method being implemented;
- an authoritative public reference;
- exact angles, intervals, offsets, and high-latitude rules;
- at least one ordinary-latitude test case;
- relevant high-latitude or cross-midnight test cases; and
- a comparison against an independent implementation or published timetable.

## Style

- Use the existing C++17 and ESP-IDF style.
- Keep identifiers, user-visible strings, and logs in English.
- Add concise bilingual comments for key functions or complex logic.
- Prefer existing project facilities over adding dependencies.
- Update the README, changelog, examples, and screenshots when behavior changes.

## Code of conduct and security

Participation is governed by `CODE_OF_CONDUCT.md`. Send security-sensitive reports through the process in `SECURITY.md` rather than a public issue.
