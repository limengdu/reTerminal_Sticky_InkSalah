# Prayer UI visual verification

Date: 2026-09-07

## Visual targets and evidence

- Portrait source: [normalized reference](docs/images/ui-review/portrait-reference.png), supplied at 834 x 1388 pixels and normalized to 480 x 800.
- Landscape source: [normalized reference](docs/images/ui-review/landscape-reference.png), supplied at 1602 x 960 pixels and normalized to 800 x 480.
- Production-rendered portrait: [sleep state](docs/images/ui-review/portrait-sleep.png).
- Production-rendered landscape: [sleep state](docs/images/ui-review/landscape-sleep.png).
- Full-view comparisons: [portrait](docs/images/ui-review/portrait-comparison.png), [landscape](docs/images/ui-review/landscape-comparison.png).
- Focused status-header comparisons: [portrait](docs/images/ui-review/portrait-header-comparison.png), [landscape](docs/images/ui-review/landscape-header-comparison.png).

The implementation is the native firmware renderer at one framebuffer pixel per image pixel. CSS viewport and browser density are not applicable. Reference copies are normalized to the display dimensions for comparison; production artwork uses a uniform scale. Detail crops use uniform nearest-neighbor enlargement.

The matched state is Istanbul, DIB, 4 September 2026, Dhuhr from 01:04 PM to 04:42 PM, 2h 34m remaining, six time rows, sample battery 100 with external power, and the sleep crescent visible. Normal awake screenshots are available in [portrait](docs/images/prayer-portrait.png) and [landscape](docs/images/prayer-landscape.png).

## Fidelity review

- Typography: native Roboto regular and bold faces separate captions, prayer names, and numeric values. AM/PM labels use an independent size. Static Arabic row labels are shaped at 20 pixels, with 28-pixel portrait and 40-pixel landscape bold title labels. The 20-pixel minimum makes status text and AM/PM larger than their scaled reference equivalents.
- Spacing: city groups and dates are centered. Battery, crescent, location, and method have separate bounds. The landscape composition uses two arches; both orientations group the current prayer, time range, countdown, and list in the reference order. English and Arabic row labels share a right edge beside the time badges.
- Colors: the background and time capsules use black; the arches and reversed text use white. Secondary captions use the existing dark-gray token. Monochrome packing produces native-pixel edges rather than the reference image's antialiasing.
- Artwork: the arches and gear reuse the PDF source. Location and clock icons are direct reference crops. The existing battery and crescent painters retain dynamic charging, charge-level, and sleep-state rendering.
- Copy: Current Prayer, Next in, Method, prayer names, dates, and AM/PM labels follow the reference presentation. Dynamic values come from the page model. Between-prayer and cross-midnight states retain their own runtime values. Long institutional names receive priority over the Method prefix, with visual ellipsis where required.

## Comparison history

1. Initial full-view check identified a landscape date touching the upper arch edges and insufficient room above the seven-row table. The date uses native 22-pixel text in landscape; the seven-row table begins at y=154 with a 45-pixel row pitch and native 20-pixel Arabic labels.
2. Full-view and focused-header checks confirmed separation between the date and arches, complete seven-row content, and distinct battery/crescent areas. Countdown borders use two pixels, and the Arabic title is generated in native bold type.
3. Final header check confirmed the city is centered in both orientations and the landscape method area accommodates MOONSIGHTING while preserving the city group. The linked final comparisons contain the current production rendering.

## Functional verification

- Host suite: 27/27 passed.
- Release and Debug firmware builds: passed.
- AddressSanitizer and UndefinedBehaviorSanitizer checks: page rendering and status rendering passed.
- Portrait and landscape with Qiyam on/off: rendered and visually checked.
- Maghrib's wider title, between-prayer state, and Isha across midnight: rendered and visually checked.
- Long city and institutional names: rendered and visually checked.
- Battery values 0, 9, and 100, invalid and out-of-range readings, black/white backgrounds, and all four rotations: footprint checks passed.
- Battery updates and clearing preserve the neighboring sleep crescent.
- Gear tap bounds follow the new header group and exclude the city area.
- Wi-Fi setup and settings-home screenshots match their prior versions pixel-for-pixel.

## Device verification

Hardware touch response, rotation behavior, charge readings, actual sleep transitions, and ePaper ghosting require a physical-device run. Display waveforms, refresh policy, input monitoring, and prayer calculations retain their existing implementation.

Follow the [development guide](docs/DEVELOPMENT.md#ui-verification) to flash and verify both orientations, gear navigation, Qiyam toggling, charging, and the three-minute sleep transition.

## Findings

No actionable P0, P1, or P2 findings remain in the rendered states. Native monochrome edges and the minimum readable status-text size are expected display constraints. Physical-device observations are recorded separately from rendered-image verification.

final result: passed
