#include <cassert>
#include <vector>

#include "canvas.h"
#include "sleep_indicator.h"

namespace {

size_t black_pixel_count(const Canvas &canvas,
                         int left,
                         int top,
                         int right,
                         int bottom)
{
    size_t count = 0U;
    for (int y = top; y < bottom; ++y) {
        for (int x = left; x < right; ++x) {
            if (canvas.pixel_at(x, y) == GrayLevel::Black) {
                ++count;
            }
        }
    }
    return count;
}

void verify_indicator(Canvas &canvas)
{
    canvas.clear(GrayLevel::White);
    sleep_indicator_render(canvas);

    const size_t badge_pixels = black_pixel_count(canvas, 8, 8, 36, 36);
    assert(badge_pixels > 60U);
    assert(badge_pixels < 150U);
    assert(black_pixel_count(canvas, 0, 0, canvas.width(), 8) == 0U);
    assert(black_pixel_count(canvas, 0, 36, canvas.width(), canvas.height()) == 0U);
    assert(black_pixel_count(canvas, 36, 8, canvas.width(), 36) == 0U);
    assert(canvas.pixel_at(12, 21) == GrayLevel::Black);
    assert(canvas.pixel_at(21, 21) == GrayLevel::White);
    assert(canvas.pixel_at(21, 30) == GrayLevel::Black);
}

}  // namespace

int main()
{
    std::vector<uint8_t> buffer(800U * 480U / 4U, 0xFFU);
    Canvas canvas(800U, 480U, buffer.data(), buffer.size());

    verify_indicator(canvas);
    canvas.set_rotation(CanvasRotation::Deg90CounterClockwise);
    assert(canvas.width() == 480U);
    assert(canvas.height() == 800U);
    verify_indicator(canvas);
    return 0;
}
