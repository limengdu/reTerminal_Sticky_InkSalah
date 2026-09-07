#pragma once

#include <cstdio>
#include <cstring>

#include "canvas.h"
#include "islamic_background_asset.h"

namespace ui {

inline void centered(Canvas &canvas, int center, int y, const char *text,
                     uint8_t size = 24U, TextWeight weight = TextWeight::Regular,
                     GrayLevel color = GrayLevel::Black)
{
    canvas.draw_text_sized(center - Canvas::text_width(text, size, weight) / 2,
                           y, text, size, color, weight);
}

// Fits complete UTF-8 characters into a pixel-measured text box.
// 按实际像素宽度裁剪文字，并保留完整 UTF-8 字符。
inline void fit(const char *text, char *output, size_t capacity, int width,
                uint8_t size, TextWeight weight = TextWeight::Regular)
{
    if (capacity < 4U) return;
    std::snprintf(output, capacity, "%s", text == nullptr ? "" : text);
    if (Canvas::text_width(output, size, weight) <= width) return;
    size_t length = std::strlen(output);
    while (length > 0U) {
        --length;
        while (length > 0U && (static_cast<uint8_t>(output[length]) & 0xC0U) == 0x80U) --length;
        output[length] = '\0';
        if (length + 4U <= capacity &&
            Canvas::text_width(output, size, weight) + Canvas::text_width("...", size, weight) <= width) {
            std::strcat(output, "...");
            return;
        }
    }
}

inline void fitted_centered(Canvas &canvas, int center, int y, const char *text,
                            int width, uint8_t size = 24U,
                            TextWeight weight = TextWeight::Regular,
                            GrayLevel color = GrayLevel::Black)
{
    char fitted[160] = {};
    fit(text, fitted, sizeof(fitted), width, size, weight);
    centered(canvas, center, y, fitted, size, weight, color);
}

inline void bitmap(Canvas &canvas, int left, int top,
                   const IslamicBackgroundAsset &asset,
                   GrayLevel color = GrayLevel::Black)
{
    for (int y = 0; y < asset.height; ++y) {
        for (int x = 0; x < asset.width; ++x) {
            if ((asset.data[y * asset.stride + x / 8] & (1U << (7 - x % 8))) != 0U)
                canvas.draw_pixel(left + x, top + y, color);
        }
    }
}

inline void pill(Canvas &canvas, int x, int y, int width, int height,
                 const char *text, bool selected = false, uint8_t size = 24U,
                 TextWeight weight = TextWeight::Bold)
{
    canvas.fill_round_rect(x, y, width, height, height / 2,
                           selected ? GrayLevel::Black : GrayLevel::White);
    canvas.draw_round_rect(x, y, width, height, height / 2);
    fitted_centered(canvas, x + width / 2, y + (height - size) / 2,
                     text, width - 20, size, weight,
                     selected ? GrayLevel::White : GrayLevel::Black);
}

}  // namespace ui
