#include "canvas.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>

#include "font.h"

namespace {

// Emits mirrored row insets using pixel-center distances from each rounded corner.
// 按像素中心到圆角圆心的距离，生成上下对称的逐行缩进。
template <typename DrawRow>
void rounded_rect_rows(int width, int height, int radius, DrawRow draw_row)
{
    if (width <= 0 || height <= 0) return;
    radius = std::clamp(radius, 0, std::min(width, height) / 2);
    const int64_t diameter_squared = 4LL * radius * radius;
    int inset = radius;
    int previous_inset = radius;
    for (int row = 0; row < height / 2 + height % 2; ++row) {
        const int64_t dy = std::max<int64_t>(0, 2LL * radius - 2LL * row - 1);
        while (inset > 0) {
            const int64_t dx = 2LL * (radius - inset) + 1;
            if (dx * dx + dy * dy > diameter_squared) break;
            --inset;
        }
        draw_row(row, inset, previous_inset);
        const int opposite_row = height - 1 - row;
        if (opposite_row != row) draw_row(opposite_row, inset, previous_inset);
        previous_inset = inset;
    }
}

}  // namespace

Canvas::Canvas(uint16_t width, uint16_t height, uint8_t *buffer, size_t buffer_size)
    : physical_width_(width),
      physical_height_(height),
      stride_((width + 3U) / 4U),
      buffer_(buffer),
      buffer_size_(buffer_size)
{
}

uint16_t Canvas::width() const
{
    return rotation_ == CanvasRotation::Deg90Clockwise ||
                   rotation_ == CanvasRotation::Deg90CounterClockwise
               ? physical_height_
               : physical_width_;
}

uint16_t Canvas::height() const
{
    return rotation_ == CanvasRotation::Deg90Clockwise ||
                   rotation_ == CanvasRotation::Deg90CounterClockwise
               ? physical_width_
               : physical_height_;
}

void Canvas::clear(GrayLevel color)
{
    prayer_header_ = false;
    // Repeat one 2-bit value four times so memset can clear four pixels at once.
    // 将同一个2位灰度值复制四次，让memset一次写入四个像素。
    const uint8_t value = static_cast<uint8_t>(color) & 0x03U;
    const uint8_t packed = static_cast<uint8_t>(value * 0x55U);
    if (buffer_ != nullptr) {
        std::memset(buffer_, packed, buffer_size_);
    }
}

GrayLevel Canvas::pixel_at(int x, int y) const
{
    if (buffer_ == nullptr || x < 0 || y < 0 ||
        x >= width() || y >= height()) {
        return GrayLevel::White;
    }

    // Reads through the same logical rotation used by the drawing functions.
    // 使用与绘图函数相同的逻辑旋转读取画布像素。
    int physical_x = x;
    int physical_y = y;
    switch (rotation_) {
    case CanvasRotation::Deg90Clockwise:
        physical_x = physical_width_ - 1 - y;
        physical_y = x;
        break;
    case CanvasRotation::Deg180:
        physical_x = physical_width_ - 1 - x;
        physical_y = physical_height_ - 1 - y;
        break;
    case CanvasRotation::Deg90CounterClockwise:
        physical_x = y;
        physical_y = physical_height_ - 1 - x;
        break;
    case CanvasRotation::Deg0:
    default:
        break;
    }

    const size_t index = static_cast<size_t>(physical_y) * stride_ +
                         static_cast<size_t>(physical_x) / 4U;
    const uint8_t shift =
        static_cast<uint8_t>((3 - (physical_x & 0x03)) * 2);
    return static_cast<GrayLevel>((buffer_[index] >> shift) & 0x03U);
}

void Canvas::draw_pixel(int x, int y, GrayLevel color)
{
    if (buffer_ == nullptr || x < 0 || y < 0 || x >= width() || y >= height()) {
        return;
    }

    int physical_x = x;
    int physical_y = y;
    switch (rotation_) {
    case CanvasRotation::Deg90Clockwise:
        physical_x = physical_width_ - 1 - y;
        physical_y = x;
        break;
    case CanvasRotation::Deg180:
        physical_x = physical_width_ - 1 - x;
        physical_y = physical_height_ - 1 - y;
        break;
    case CanvasRotation::Deg90CounterClockwise:
        physical_x = y;
        physical_y = physical_height_ - 1 - x;
        break;
    case CanvasRotation::Deg0:
    default:
        break;
    }

    // Pixels are packed from the most-significant pair to the least-significant pair.
    // 每个字节从高位到低位依次保存四个像素，每个像素占两位。
    const size_t index = static_cast<size_t>(physical_y) * stride_ +
                         static_cast<size_t>(physical_x) / 4U;
    const uint8_t shift =
        static_cast<uint8_t>((3 - (physical_x & 0x03)) * 2);
    const uint8_t mask = static_cast<uint8_t>(0x03U << shift);
    const uint8_t value = static_cast<uint8_t>(color) << shift;
    buffer_[index] = static_cast<uint8_t>((buffer_[index] & ~mask) | value);
}

void Canvas::draw_line(int x0, int y0, int x1, int y1, GrayLevel color)
{
    // Bresenham's integer algorithm draws every slope without floating-point math.
    // Bresenham整数算法无需浮点运算，就能绘制任意斜率的连续直线。
    const int dx = std::abs(x1 - x0);
    const int step_x = x0 < x1 ? 1 : -1;
    const int dy = -std::abs(y1 - y0);
    const int step_y = y0 < y1 ? 1 : -1;
    int error = dx + dy;

    while (true) {
        draw_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) {
            break;
        }
        const int twice_error = 2 * error;
        if (twice_error >= dy) {
            error += dy;
            x0 += step_x;
        }
        if (twice_error <= dx) {
            error += dx;
            y0 += step_y;
        }
    }
}

void Canvas::draw_rect(int x, int y, int width, int height, GrayLevel color)
{
    if (width <= 0 || height <= 0) {
        return;
    }
    draw_line(x, y, x + width - 1, y, color);
    draw_line(x, y + height - 1, x + width - 1, y + height - 1, color);
    draw_line(x, y, x, y + height - 1, color);
    draw_line(x + width - 1, y, x + width - 1, y + height - 1, color);
}

void Canvas::fill_rect(int x, int y, int width, int height, GrayLevel color)
{
    if (width <= 0 || height <= 0) {
        return;
    }

    // Clip once before looping so out-of-bounds rectangles remain safe and predictable.
    // 进入循环前统一裁剪边界，使超出屏幕的矩形也能安全绘制。
    const int start_x = std::max(0, x);
    const int start_y = std::max(0, y);
    const int end_x = std::min<int>(this->width(), x + width);
    const int end_y = std::min<int>(this->height(), y + height);
    for (int row = start_y; row < end_y; ++row) {
        for (int column = start_x; column < end_x; ++column) {
            draw_pixel(column, row, color);
        }
    }
}

void Canvas::draw_circle(int center_x,
                         int center_y,
                         int radius,
                         GrayLevel color)
{
    if (radius < 0) {
        return;
    }

    // Draws the circle perimeter with the integer midpoint algorithm.
    // 使用整数中点圆算法绘制圆形边缘。
    int x = radius;
    int y = 0;
    int error = 1 - radius;
    while (x >= y) {
        draw_pixel(center_x + x, center_y + y, color);
        draw_pixel(center_x + y, center_y + x, color);
        draw_pixel(center_x - y, center_y + x, color);
        draw_pixel(center_x - x, center_y + y, color);
        draw_pixel(center_x - x, center_y - y, color);
        draw_pixel(center_x - y, center_y - x, color);
        draw_pixel(center_x + y, center_y - x, color);
        draw_pixel(center_x + x, center_y - y, color);

        ++y;
        if (error < 0) {
            error += 2 * y + 1;
        } else {
            --x;
            error += 2 * (y - x + 1);
        }
    }
}

void Canvas::fill_circle(int center_x,
                         int center_y,
                         int radius,
                         GrayLevel color)
{
    if (radius < 0) {
        return;
    }

    // Fills the circle interior with horizontal spans.
    // 使用水平线段填充圆形内部。
    int x = radius;
    int y = 0;
    int error = 1 - radius;
    while (x >= y) {
        draw_line(center_x - x, center_y + y, center_x + x, center_y + y, color);
        draw_line(center_x - x, center_y - y, center_x + x, center_y - y, color);
        draw_line(center_x - y, center_y + x, center_x + y, center_y + x, color);
        draw_line(center_x - y, center_y - x, center_x + y, center_y - x, color);

        ++y;
        if (error < 0) {
            error += 2 * y + 1;
        } else {
            --x;
            error += 2 * (y - x + 1);
        }
    }
}

uint8_t Canvas::text_size(uint8_t scale)
{
    scale = std::max<uint8_t>(scale, kMinimumTextScale);
    return scale == 2U ? 20U : scale == 3U ? 28U : scale == 4U ? 36U
           : scale == 5U ? 40U : 48U;
}

int Canvas::text_width(const char *text, uint8_t size, TextWeight weight)
{
    return font_text_width(text, size, weight == TextWeight::Bold);
}

void Canvas::draw_text(int x, int y, const char *text, uint8_t scale,
                       GrayLevel color, TextWeight weight)
{
    draw_text_sized(x, y, text, text_size(scale), color, weight);
}

void Canvas::draw_text_sized(int x, int y, const char *text, uint8_t size,
                             GrayLevel color, TextWeight weight)
{
    if (text == nullptr) return;
    const FontFace &face = font_face(size, weight == TextWeight::Bold);
    while (*text != '\0') {
        const FontGlyph &glyph = font_glyph(face, font_next_codepoint(text));
        const uint8_t *pixels = font_pixels(glyph);
        const int stride = (glyph.width + 7) / 8;
        for (int row = 0; row < glyph.height; ++row) {
            for (int column = 0; column < glyph.width; ++column) {
                if ((pixels[row * stride + column / 8] &
                     (1U << (7 - column % 8))) != 0U) {
                    draw_pixel(x + glyph.left + column, y + glyph.top + row, color);
                }
            }
        }
        x += glyph.advance;
    }
}

void Canvas::fill_round_rect(int x, int y, int width, int height, int radius,
                             GrayLevel color)
{
    rounded_rect_rows(width, height, radius, [&](int row, int inset, int) {
        fill_rect(x + inset, y + row, width - 2 * inset, 1, color);
    });
}

void Canvas::draw_round_rect(int x, int y, int width, int height, int radius,
                             GrayLevel color)
{
    // Traces the filled contour, including horizontal steps between adjacent rows.
    // 沿填充区域的轮廓描边，并覆盖相邻行之间的水平阶梯。
    rounded_rect_rows(width, height, radius, [&](int row, int inset, int previous_inset) {
        if (row == 0 || row == height - 1) {
            fill_rect(x + inset, y + row, width - 2 * inset, 1, color);
            return;
        }
        const int edge_width = std::max(1, previous_inset - inset);
        fill_rect(x + inset, y + row, edge_width, 1, color);
        fill_rect(x + width - inset - edge_width, y + row, edge_width, 1, color);
    });
}

void Canvas::physical_to_logical(int physical_x,
                                 int physical_y,
                                 int &logical_x,
                                 int &logical_y) const
{
    switch (rotation_) {
    case CanvasRotation::Deg90Clockwise:
        logical_x = physical_y;
        logical_y = physical_width_ - 1 - physical_x;
        break;
    case CanvasRotation::Deg180:
        logical_x = physical_width_ - 1 - physical_x;
        logical_y = physical_height_ - 1 - physical_y;
        break;
    case CanvasRotation::Deg90CounterClockwise:
        logical_x = physical_height_ - 1 - physical_y;
        logical_y = physical_x;
        break;
    case CanvasRotation::Deg0:
    default:
        logical_x = physical_x;
        logical_y = physical_y;
        break;
    }
}
