#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <vector>

#include "canvas.h"

namespace {

struct Rect {
    int x, y, width, height, radius;
};

struct Surface {
    std::vector<uint8_t> bytes;
    Canvas canvas;

    Surface(int width, int height)
        : bytes((width + 3) / 4 * height + 32, 0xAB),
          canvas(width, height, bytes.data() + 16, bytes.size() - 32)
    {
    }

    void check_guards() const
    {
        for (size_t i = 0; i < 16; ++i) {
            assert(bytes[i] == 0xAB);
            assert(bytes[bytes.size() - 1 - i] == 0xAB);
        }
    }
};

// Tests pixel centers against the straight sections and circular corner regions.
// 根据像素中心与直边、圆角区域的位置关系计算预期填充范围。
bool expected_fill(const Rect &rect, int x, int y)
{
    x -= rect.x;
    y -= rect.y;
    if (rect.width <= 0 || rect.height <= 0 || x < 0 || y < 0 ||
        x >= rect.width || y >= rect.height) return false;
    const int radius = std::clamp(rect.radius, 0, std::min(rect.width, rect.height) / 2);
    const int64_t dx = std::max<int64_t>(0, 2LL * radius - 1 -
                                           2LL * std::min(x, rect.width - 1 - x));
    const int64_t dy = std::max<int64_t>(0, 2LL * radius - 1 -
                                           2LL * std::min(y, rect.height - 1 - y));
    return dx * dx + dy * dy <= 4LL * radius * radius;
}

bool expected_outline(const Rect &rect, int x, int y)
{
    return expected_fill(rect, x, y) &&
           (!expected_fill(rect, x - 1, y) || !expected_fill(rect, x + 1, y) ||
            !expected_fill(rect, x, y - 1) || !expected_fill(rect, x, y + 1));
}

void verify(Surface &surface, const Rect &rect, bool outline,
            GrayLevel foreground = GrayLevel::Black,
            GrayLevel background = GrayLevel::White)
{
    Canvas &canvas = surface.canvas;
    canvas.clear(background);
    if (outline) {
        canvas.draw_round_rect(rect.x, rect.y, rect.width, rect.height, rect.radius, foreground);
    } else {
        canvas.fill_round_rect(rect.x, rect.y, rect.width, rect.height, rect.radius, foreground);
    }
    for (int y = 0; y < canvas.height(); ++y) {
        for (int x = 0; x < canvas.width(); ++x) {
            const bool occupied = outline ? expected_outline(rect, x, y) : expected_fill(rect, x, y);
            const GrayLevel expected = occupied ? foreground : background;
            if (canvas.pixel_at(x, y) != expected) {
                std::fprintf(stderr,
                             "FAIL: %s rect=(%d,%d,%d,%d,r=%d) rotation=%d pixel=(%d,%d) actual=%d expected=%d\n",
                             outline ? "outline" : "fill", rect.x, rect.y, rect.width,
                             rect.height, rect.radius, static_cast<int>(canvas.rotation()),
                             x, y, static_cast<int>(canvas.pixel_at(x, y)), static_cast<int>(expected));
                std::exit(1);
            }
        }
    }
    surface.check_guards();
}

void verify_capsule_bounds(Surface &surface, int height)
{
    Canvas &canvas = surface.canvas;
    constexpr int x = 100, y = 100, width = 195;
    canvas.clear();
    canvas.fill_round_rect(x, y, width, height, height / 2, GrayLevel::Black);
    int min_y = canvas.height(), max_y = -1, outside = 0;
    for (int row = 0; row < canvas.height(); ++row) {
        for (int column = 0; column < canvas.width(); ++column) {
            if (canvas.pixel_at(column, row) != GrayLevel::Black) continue;
            min_y = std::min(min_y, row);
            max_y = std::max(max_y, row);
            if (column < x || column >= x + width || row < y || row >= y + height) ++outside;
        }
    }
    std::printf("capsule requested_height=%d actual_height=%d outside_pixels=%d\n",
                height, max_y - min_y + 1, outside);
    assert(min_y == y && max_y == y + height - 1 && outside == 0);
}

}  // namespace

int main()
{
    Surface display(800, 480);
    for (int height : {50, 54, 66}) verify_capsule_bounds(display, height);

    for (auto rotation : {CanvasRotation::Deg0, CanvasRotation::Deg90Clockwise,
                          CanvasRotation::Deg180, CanvasRotation::Deg90CounterClockwise}) {
        display.canvas.set_rotation(rotation);
        for (const Rect rect : {Rect{100, 100, 268, 50, 25}, {100, 100, 288, 54, 27},
                                {46, 200, 195, 66, 33}, {46, 200, 399, 66, 33},
                                {41, 200, 399, 77, 12}, {70, 136, 340, 67, 33},
                                {20, 20, 66, 195, 33}}) {
            verify(display, rect, false);
            verify(display, rect, true);
        }
        for (auto color : {GrayLevel::Black, GrayLevel::DarkGray, GrayLevel::LightGray, GrayLevel::White}) {
            const auto background = color == GrayLevel::White ? GrayLevel::Black : GrayLevel::White;
            const Rect rect{100, 100, 195, 66, 33};
            verify(display, rect, false, color, background);
            const auto filled = display.bytes;
            display.canvas.draw_round_rect(rect.x, rect.y, rect.width, rect.height, rect.radius, color);
            assert(display.bytes == filled);
            verify(display, rect, true, color, background);
        }
    }

    Surface small(48, 44);
    for (auto rotation : {CanvasRotation::Deg0, CanvasRotation::Deg90Clockwise,
                          CanvasRotation::Deg180, CanvasRotation::Deg90CounterClockwise}) {
        small.canvas.set_rotation(rotation);
        for (int width = 1; width <= 12; ++width) {
            for (int height = 1; height <= 12; ++height) {
                for (int radius : {-3, 0, 1, 2, std::min(width, height) / 2, 100}) {
                    const Rect rect{15, 14, width, height, radius};
                    verify(small, rect, false);
                    verify(small, rect, true);
                }
            }
        }
        for (const Rect rect : {Rect{-5, -4, 26, 22, 11}, {35, 33, 26, 22, 11},
                                {-50, 15, 10, 10, 5}, {60, 10, 20, 20, 10},
                                {12, 12, 0, 10, 5}, {12, 12, 10, 0, 5},
                                {12, 12, -1, 10, 5}, {12, 12, 10, -1, 5},
                                {-10, -10, 70, 70, 0}}) {
            verify(small, rect, false);
            verify(small, rect, true);
        }
    }
    std::puts("PASS: rounded rectangles, shared contours, colors, clipping, and four rotations");
}
