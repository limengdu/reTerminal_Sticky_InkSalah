#include "font.h"

#include <cstdlib>

namespace {
#include "roboto_font_data.inc"
}

const FontFace &font_face(uint8_t size, bool bold)
{
    const FontFace *best = &kRobotoFaces[bold ? 1 : 0];
    for (const FontFace &face : kRobotoFaces) {
        if (face.bold == bold &&
            std::abs(static_cast<int>(face.size) - size) <
                std::abs(static_cast<int>(best->size) - size)) {
            best = &face;
        }
    }
    return *best;
}

const FontGlyph &font_glyph(const FontFace &face, uint32_t codepoint)
{
    size_t first = 0U;
    size_t last = face.count;
    while (first < last) {
        const size_t middle = first + (last - first) / 2U;
        if (face.glyphs[middle].codepoint < codepoint) first = middle + 1U;
        else last = middle;
    }
    return first < face.count && face.glyphs[first].codepoint == codepoint
               ? face.glyphs[first] : face.glyphs['?' - 32];
}

const uint8_t *font_pixels(const FontGlyph &glyph)
{
    return kRobotoPixels + glyph.offset;
}

uint32_t font_next_codepoint(const char *&text)
{
    // Consumes one UTF-8 sequence, leaving a malformed suffix for the next read.
    // 读取一个 UTF-8 字符，格式不完整的后缀保留给下一次读取。
    const uint8_t lead = static_cast<uint8_t>(*text++);
    if (lead < 128U) return lead;
    const int length = lead >= 0xC2U && lead <= 0xDFU ? 2
                       : lead <= 0xEFU && lead >= 0xE0U ? 3
                       : lead <= 0xF4U && lead >= 0xF0U ? 4 : 0;
    if (length == 0) return '?';
    uint32_t point = lead & (0x7FU >> length);
    for (int index = 1; index < length; ++index) {
        const uint8_t next = static_cast<uint8_t>(*text);
        if ((next & 0xC0U) != 0x80U) return '?';
        point = (point << 6U) | (next & 0x3FU);
        ++text;
    }
    return point;
}

int font_text_width(const char *text, uint8_t size, bool bold)
{
    if (text == nullptr) return 0;
    const FontFace &face = font_face(size, bold);
    int width = 0;
    while (*text != '\0') width += font_glyph(face, font_next_codepoint(text)).advance;
    return width;
}
