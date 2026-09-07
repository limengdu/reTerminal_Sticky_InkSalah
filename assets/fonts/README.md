# Roboto font assets

`Roboto.ttf` is the upright variable Roboto font from [Google Fonts](https://github.com/google/fonts/tree/main/ofl/roboto), retrieved on 2026-09-07. `Roboto-OFL.txt` contains the font license.

- Font SHA-256: `d7598e12c5dbef095ff8272cfc55da0250bd07fbdecbac8a530b9b277872a134`
- License SHA-256: `061402327a96aadb0bfb694a960ed289ecd38d383e396243831ab81feb109c41`
- Weights: 400 (regular), 700 (bold), width axis 100.
- Sizes: 20, 22, 24, 26, 28, 32, 36, 40, 48 pixels.
- Coverage: printable ASCII, Latin-1 Supplement, Latin Extended-A, en dash, em dash, and right apostrophe.

Run `python3 tools/generate_roboto_font.py` from the project root with Pillow installed to regenerate the firmware glyphs. The glyphs are stored in flash and rendered by the same code on the device and in host previews.
