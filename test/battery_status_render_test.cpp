#include <cassert>
#include <vector>

#include "battery_status_render.h"
#include "sleep_indicator.h"
#include "status_bar_layout.h"

namespace {

void verify_footprint(Canvas &canvas, bool prayer, bool sleep_layout, GrayLevel background)
{
    const int width = canvas.width();
    const int height = canvas.height();
    canvas.clear(background);
    canvas.set_prayer_header(prayer);
    const auto area = battery_status_area(canvas, sleep_layout);
    assert(area.x >= 0 && area.y >= 0 && area.x + area.width <= width);
    if (prayer) {
        assert(area.x == 24 && area.y == 24);
        assert(area.x + area.width < sleep_status_area(canvas).x);
    } else {
        assert(area.y == 4 && area.x > width / 2);
    }
    for (int percent : {-1, 0, 9, 100, 101}) {
        battery_status_render(canvas, percent >= 0, percent, true, sleep_layout);
        int changed = 0;
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                if (canvas.pixel_at(x, y) == background) continue;
                assert(x >= area.x && x < area.x + area.width);
                assert(y >= area.y && y < area.y + area.height);
                ++changed;
            }
        }
        assert(changed > 50);
        battery_status_render_clear(canvas, sleep_layout);
        for (int y = area.y; y < area.y + area.height; ++y)
            for (int x = area.x; x < area.x + area.width; ++x)
                assert(canvas.pixel_at(x, y) == background);
    }
    if (prayer) {
        sleep_indicator_render(canvas);
        const auto moon = sleep_status_area(canvas);
        const auto visible = canvas.pixel_at(moon.x + 4, moon.y + 13);
        assert(visible != background);
        battery_status_render(canvas, true, 100, false, true);
        battery_status_render_clear(canvas, false);
        assert(canvas.pixel_at(moon.x + 4, moon.y + 13) == visible);
    }
    canvas.clear();
    assert(!canvas.prayer_header());
    assert(battery_status_area(canvas, false).x == width - 104);
}

}  // namespace

int main()
{
    constexpr size_t size = 800U * 480U / 4U;
    std::vector<uint8_t> buffer(size + 32, 0xAB);
    Canvas canvas(800, 480, buffer.data() + 16, size);
    for (const auto rotation : {CanvasRotation::Deg0, CanvasRotation::Deg90Clockwise,
                                CanvasRotation::Deg180, CanvasRotation::Deg90CounterClockwise}) {
        canvas.set_rotation(rotation);
        for (bool prayer : {false, true})
            for (bool sleep_layout : {false, true})
                for (auto background : {GrayLevel::Black, GrayLevel::White})
                    verify_footprint(canvas, prayer, sleep_layout, background);
    }
    for (size_t i = 0; i < 16; ++i) {
        assert(buffer[i] == 0xAB);
        assert(buffer[size + 16 + i] == 0xAB);
    }
    return 0;
}
