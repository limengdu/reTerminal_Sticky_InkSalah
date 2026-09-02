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

The monochrome Islamic UI artwork in `assets/ui/` was created for InkSalah. Firmware-ready one-bit derivatives are stored in `src/ui/`.
