# INKSALAH design assets

The portrait prayer and setup backgrounds preserve the image placement from the supplied four-page INKSALAH design. The settings lantern strip is rendered directly from the first 120 points of page 3. The book image and the book and gear masks are embedded resources from that document. The landscape composition reuses the same arch artwork in a two-column layout.

Source PDF SHA-256: `3e3b240fbbcba60c8f01457da97d765b28a9c7790fc65c9c3b06d788b03516f9`.

`tools/extract_inksalah_pdf_assets.py` extracts the page artwork. `tools/generate_inksalah_assets.py` packs the resulting PNGs into monochrome firmware arrays. Existing source PNGs make the build reproducible without the design PDF.

The 2026-09-07 portrait and landscape reference images define the prayer-page composition. Both landscape arches use the PDF's original arch at a uniform scale. `location-source.png` and `clock-source.png` are direct icon crops from the supplied landscape reference; the gear continues to use the PDF mask. Their native-size firmware assets are generated alongside the backgrounds.

- Portrait reference: 834 x 1388, SHA-256 `70f915695f13f8d0724b13ed01acead0860d93ba50b56ef6293c32c45a3be387`.
- Landscape reference: 1602 x 960, SHA-256 `733c61badce1b978f363f85b022d8a0a6c67461cf14f15938ba2bd33e15c0370`.
- Display-sized reference copies and side-by-side verification images: `docs/images/ui-review/`.

Artwork licensing is described in [Notices](../../NOTICE.md#artwork).
