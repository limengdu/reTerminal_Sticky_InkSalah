# Sticky screen compatibility

## Controller selection

The application requests the SSD family. `seeed_epaper_new_panel()` resets the panel with the SSD2677-compatible 20/10 ms timings, sends `0x70`, and reads one byte. `0x07` selects SSD2677. Every other successful read, including `0x00` and `0xFF`, selects SSD1677 to preserve the vendor fallback. Those fallback values are **not** proof of a positively identified old panel: broken read wiring can also return them. SPI errors abort initialization.

The serial log includes `panel probe: chip_id=0x07 controller=SSD2677` or the legacy controller name, followed by `display=panel_ready controller=...`. `seeed_epaper_panel_get_model()` returns the actual model. SSD2677 uses active-low BUSY, a 30-second timeout and 20/10 ms reset. SSD1677 keeps active-high BUSY and the application's existing timeout/reset settings. Detection runs once per panel creation, not on every wake.

SPI now uses `SPI_DEVICE_HALFDUPLEX` for receive-only ID and internal-temperature transactions. Existing board pins are unchanged: MOSI 14, MISO 12, CLK 13, CS 15, DC 16, RST 17, BUSY 18, EN 47. Hardware CS remains in use. The supplied component does not include a complete board SPI configuration, so physical readback must be verified on the target panel. No unsupported three-wire wiring assumption is made.

## Framebuffer and power behavior

SSD1677 retains its existing controller implementation, plane polarity, software mirroring, rotation and sleep commands.

SSD2677 uses the supplied Seeed reference's OTP register sequence and temperature buckets. Full monochrome pixels expand to packed two-bit values. A partial refresh interleaves the previous/current monochrome bits. Four-level grayscale streams packed two-bit pixels. All three send **96,000 bytes** through `0x10`, including a partial refresh of a small rectangle. The existing 1,024-byte internal DMA staging buffer bounds each SPI transfer.

SSD2677 allocates two 48,000-byte monochrome shadows and, when grayscale is first used, an additional 96,000-byte grayscale shadow. Shadows use identical physical coordinates including mirroring, and committed history advances only after successful refresh. A failed refresh invalidates the partial baseline, requiring a full refresh on recovery. New-controller unaligned monochrome writes preserve pixels outside the requested rectangle. The public grayscale bitmap API accepts the application's packed two-bit pixels, with `0=black` and `3=white`.

InkSalah refreshes infrequently. Unlike the continuous-handwriting reference, it powers down SSD2677's high-voltage supply after each successful refresh; a subsequent refresh powers it back on. Deep sleep uses `0x07/0xA5` and wake reinitializes the selected controller. The vendor resolution register bytes `03 20 02 A8` are retained verbatim; the logical framebuffer is 800×480. Do not replace controller scan-register values based only on logical dimensions.

## Verification

Run `bash tools/run_host_tests.sh` for the application regression suite and driver tests. Run `bash tools/run_epaper_tests.sh` for the focused production-driver tests alone. Build both `sticky-salah-release` and `sticky-salah-debug` with PlatformIO.

Host tests emulate SPI/GPIO/memory. They verify software byte streams and state transitions, **not** electrical wiring, real waveforms, display quality, or physical power consumption.

On **both** an original and revised Sticky screen:

1. Cold boot with the debug build. Capture the raw ID and selected controller. Confirm a revised screen reads `0x07`; investigate unexpected fallback rather than assuming old hardware.
2. Check setup, prayer and settings screens. Verify all four gray levels, black/white polarity, all corners and asymmetric text with no mirror or rotation errors.
3. Exercise portrait/landscape and all four rotations. Confirm touch and visible controls still align.
4. Trigger repeated fast transitions, countdown/battery updates, then grayscale cleanup. Check changed and unchanged areas for corruption or ghosting.
5. Enter deep sleep, wake repeatedly without changing orientation, then wake with orientation changes. Check retained image, crescent indicator and full refresh recovery.
6. Verify temperature reads and BUSY completion. Check supply current after both grayscale and partial refreshes and while asleep.
7. Where safe, test an interrupted refresh or power cycle and verify the next boot/full refresh recovers cleanly.

发布到 Sticky registry 前，需先在新旧两种实机上完成以上检查。本次软件测试不等于实机验证，尤其要确认新屏的 `0x07` 读回、灰度与局刷切换、四方向显示、休眠唤醒及刷新后的电流。
