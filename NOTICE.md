# Notices

InkSalah is distributed under the MIT License in `LICENSE`.

## reTerminal Sticky board integration

Board initialization patterns and selected low-level drivers were adapted from:

- Project: `limengdu/reTerminal_Sticky_Bunny`
- Source: https://github.com/limengdu/reTerminal_Sticky_Bunny
- Audited reference commit: `cf63d3fd4978a5f18fb5b779c197a17a91c59fe1`
- License: MIT

The upstream MIT license is compatible with this project's MIT distribution.

## Espressif managed components

The build resolves these components through the ESP-IDF Component Manager:

- `espressif/button` 4.1.6, Apache License 2.0
- `espressif/cmake_utilities` 1.1.1, Apache License 2.0

Their source, license text, and exact repository commits are downloaded into `managed_components/` during dependency resolution and are not vendored in this repository.

## Artwork

The original project artwork and the PDF-derived artwork are stored in `assets/ui/`. `prayer-original.png`, `setup-original.png`, `lanterns-source.png`, `arch-source.png`, `book-source.png`, `book-mask.png`, and `gear-mask.png` are extracted from the supplied INKSALAH design. Firmware-ready one-bit derivatives are stored in `src/ui/`. PDF-derived artwork retains its original ownership and licensing; redistribution of these assets is governed by the original artwork licenses.

## Roboto

Roboto is supplied by the Roboto Project Authors under the SIL Open Font License 1.1. The font and license are vendored in `assets/fonts/`. Native-size raster derivatives are generated into `src/ui/roboto_font_data.inc`.

Source: [Google Fonts Roboto](https://github.com/google/fonts/tree/main/ofl/roboto).

Arabic labels are pre-shaped using the macOS Geeza Pro font through AppKit. The generated label bitmaps are included with the UI assets; the system font file is provided by macOS.
