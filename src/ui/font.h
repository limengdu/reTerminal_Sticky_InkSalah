#pragma once

#include <cstddef>
#include <cstdint>

struct FontGlyph {
    uint32_t codepoint;
    uint32_t offset;
    uint8_t width;
    uint8_t height;
    int8_t left;
    int8_t top;
    uint8_t advance;
};

struct FontFace {
    uint8_t size;
    bool bold;
    const FontGlyph *glyphs;
    size_t count;
};

const FontFace &font_face(uint8_t size, bool bold);
const FontGlyph &font_glyph(const FontFace &face, uint32_t codepoint);
const uint8_t *font_pixels(const FontGlyph &glyph);
uint32_t font_next_codepoint(const char *&text);
int font_text_width(const char *text, uint8_t size, bool bold = false);

// Returns the drawing origin that centers the visible glyph rows on center_y.
// 返回绘制起点，使字形实际可见行的中心位于 center_y。
int font_text_centered_y(const char *text, uint8_t size, int center_y,
                         bool bold = false);
