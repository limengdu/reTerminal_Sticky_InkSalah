# E-paper host regression tests

Run `./tools/run_epaper_tests.sh` from any directory. Requires a C11 compiler, not ESP-IDF or PlatformIO. `CC` selects the compiler; `EPAPER_TEST_CFLAGS` adds compiler/linker flags.

The runner builds the **actual** shared panel implementation and SSD1677, SSD2677 and UC8179 driver sources. Only ESP-IDF GPIO, SPI, allocation, time and FreeRTOS interfaces are replaced with small host doubles. It creates a temporary executable and removes it on exit.

Coverage:

- `0x70` probe reads `0x07` as SSD2677; `0x00` and `0xFF` retain SSD1677; both probe transactions propagate SPI failure.
- Probe reset timing, controller BUSY polarity, interrupt edge, 5/30-second timeout, successful wake and immediately-ready path.
- Allocation failures for DMA staging, previous frame and current frame; tracked heap allocations return to zero after deletion/failure.
- Gray4 wire byte order and monochrome shadow with and without horizontal mirroring.
- Full 96,000-byte transfers, partial previous/current bit interleaving, waveform/power/refresh commands, and history advancement after success.
- Failure at every SPI transaction in a partial commit, including every frame chunk, power-on, refresh and power-off; history remains unchanged and panel requires reinitialization.
- Twelve temperature boundaries across all three waveform modes.
- Gray-to-partial and failed-partial-to-full transitions, deep sleep/idempotence/wakeup, and unaligned writes preserving neighboring pixels.
- Two-row right-edge clipping with default source stride for mono and gray4, both orientations.
- Gray4 reconstruction after a mono commit, checking every unchanged byte across the 96KB shadow.
- Explicit previous/current diff regions over two rows, with mirrored unaligned edges and exact wire-byte assertions.
- Legacy SSD1677 current `0x24`, previous `0x26`, and refresh `0x20` paths.

Strict warning run (unused logging expressions and stub parameters are intentionally suppressed by the runner):

```sh
EPAPER_TEST_CFLAGS='-Werror -Wpedantic' ./tools/run_epaper_tests.sh
```

Optional sanitizer run:

```sh
ASAN_OPTIONS=detect_leaks=0 EPAPER_TEST_CFLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' ./tools/run_epaper_tests.sh
```

The sandbox uses ptrace, which prevents LeakSanitizer from running. Omit `ASAN_OPTIONS=detect_leaks=0` on ordinary hosts to enable leak checking too. Heap-capability allocations are independently counted by the test harness.

These tests validate software byte sequences and state, not physical waveform quality, panel identification electrical behavior, SPI half-duplex compatibility or actual BUSY timing. Hardware validation is still required.
