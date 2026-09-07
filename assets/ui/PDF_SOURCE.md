# INKSALAH design assets

The portrait prayer and setup backgrounds preserve the image placement from the supplied four-page INKSALAH design. The settings lantern strip is rendered directly from the first 120 points of page 3. The book image and the book and gear masks are embedded resources from that document. The landscape composition reuses the same arch artwork in a two-column layout.

Source PDF SHA-256: `3e3b240fbbcba60c8f01457da97d765b28a9c7790fc65c9c3b06d788b03516f9`.

`tools/extract_inksalah_pdf_assets.py` extracts the page artwork. `tools/generate_inksalah_assets.py` packs the resulting PNGs into monochrome firmware arrays. Existing source PNGs make the build reproducible without the design PDF.

Artwork licensing is described in [Notices](../../NOTICE.md#artwork).
