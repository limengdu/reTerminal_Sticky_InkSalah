#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "battery_status_render.h"
#include "font.h"
#include "prayer_pages.h"
#include "sleep_indicator.h"
#include "status_bar_layout.h"

namespace {

struct InkBounds {
    int left = 1000;
    int top = 1000;
    int right = -1;
    int bottom = -1;
};

InkBounds visible_bounds(const Canvas &canvas, int x, int width)
{
    InkBounds bounds;
    for (int y = 20; y < 54; ++y) {
        for (int column = x; column < x + width; ++column) {
            if (canvas.pixel_at(column, y) != GrayLevel::White) continue;
            bounds.left = std::min(bounds.left, column);
            bounds.top = std::min(bounds.top, y);
            bounds.right = std::max(bounds.right, column);
            bounds.bottom = std::max(bounds.bottom, y);
        }
    }
    return bounds;
}

bool centered(const Canvas &canvas, int x, int width, const char *name)
{
    const auto bounds = visible_bounds(canvas, x, width);
    const int twice_center = bounds.top + bounds.bottom;
    if (bounds.right < bounds.left || twice_center < 75 || twice_center > 76) {
        std::fprintf(stderr, "FAIL: %s visible_y=%d..%d center=%.1f target=38\n",
                     name, bounds.top, bounds.bottom, twice_center / 2.0);
        return false;
    }
    return true;
}

}  // namespace

int main()
{
    assert(font_text_centered_y(nullptr, 20, 38, true) == 38);
    assert(font_text_centered_y("", 20, 38, true) == 38);
    assert(font_text_centered_y("   ", 20, 38, true) == 38);
    const uint8_t padded_rows[] = {0x01, 0x81, 0x01};
    assert(prayer_status_bitmap_y({1, 3, 1, padded_rows}) == 37);
    constexpr size_t size = 800U * 480U / 4U;
    std::vector<uint8_t> storage(size + 32, 0xAB);
    Canvas canvas(800, 480, storage.data() + 16, size);
    SalahSettings settings = {};
    settings_store_defaults(settings);
    settings.calculation_method = 13;
    PrayerDay today = {};
    today.date = {2026, 9, 4};
    std::strcpy(today.weekday, "Friday");
    today.hijri_year = 1448;
    today.hijri_month = 3;
    today.hijri_day = 22;
    const uint16_t times[] = {299, 393, 784, 1002, 1173, 1263, 1554};
    std::memcpy(today.minutes, times, sizeof(times));
    PrayerPageModel model = {};
    model.settings = &settings;
    model.today = &today;
    model.next = {PrayerId::Asr, 1002, 154, false};
    model.window = {PrayerId::Dhuhr, 784, 1002, 154};
    for (auto rotation : {CanvasRotation::Deg0, CanvasRotation::Deg90Clockwise,
                          CanvasRotation::Deg180, CanvasRotation::Deg90CounterClockwise}) {
        canvas.set_rotation(rotation);
        const bool portrait = canvas.width() < canvas.height();
        const int city_width = portrait ? 132 : 300;
        const int method_width = portrait ? 146 : 210;
        const auto location_touch = prayer_location_touch_area(canvas.width(), canvas.height());
        const auto settings_touch = prayer_settings_touch_area(canvas.width(), canvas.height());
        assert(location_touch.height == 60 && settings_touch.height == 60);
        const int boundary = portrait ? 308 : 558;
        assert(location_touch.x + location_touch.width == boundary);
        assert(settings_touch.x == boundary);
        for (int y : {8, 12, 38, 63, 67}) {
            assert(prayer_page_header_action_at(canvas.width() / 2, y, canvas.width(), canvas.height()) == PrayerHeaderAction::Location);
            assert(prayer_page_header_action_at(boundary - 1, y, canvas.width(), canvas.height()) == PrayerHeaderAction::Location);
            assert(prayer_page_header_action_at(boundary, y, canvas.width(), canvas.height()) == PrayerHeaderAction::Settings);
            assert(prayer_page_header_action_at(canvas.width() - 8, y, canvas.width(), canvas.height()) == PrayerHeaderAction::Settings);
        }
        for (int x : {-1, 25, 60, 115, static_cast<int>(canvas.width())})
            assert(prayer_page_header_action_at(x, 38, canvas.width(), canvas.height()) == PrayerHeaderAction::None);
        for (int y : {-1, 0, 7, 68, 72, 100, static_cast<int>(canvas.height())})
            for (int x : {canvas.width() / 2, canvas.width() - 30})
                assert(prayer_page_header_action_at(x, y, canvas.width(), canvas.height()) == PrayerHeaderAction::None);
        for (const char *city : {"Istanbul", "Shenzhen", "San Francisco Bay Area", "Agra", "IZMIR", "\xC4\xB0zmir"}) {
            std::strcpy(settings.city, city);
            for (int percent : {-1, 0, 9, 100}) {
                settings.calculation_method = percent < 0 ? 13 : percent == 0 ? 3
                                                          : percent == 9 ? 15 : kCalculationMethodCustomAngles;
                prayer_page_render_schedule(canvas, model);
                battery_status_render(canvas, percent >= 0, percent, true, false);
                sleep_indicator_render(canvas);
                bool valid = centered(canvas, 25, 28, "battery");
                valid = centered(canvas, 59, 37, "charge") && valid;
                valid = centered(canvas, 102, 28, "crescent") && valid;
                const auto location = visible_bounds(canvas, canvas.width() / 2 - city_width / 2, city_width);
                assert(location_touch.contains(location.left, location.top));
                assert(location_touch.contains(location.right, location.bottom));
                valid = centered(canvas, location.left, 18, "location pin") && valid;
                valid = centered(canvas, location.left + 24, location.right - location.left - 23, "city") && valid;
                const auto method = visible_bounds(canvas, canvas.width() - 24 - method_width, method_width);
                assert(settings_touch.contains(method.left, method.top));
                assert(settings_touch.contains(method.right, method.bottom));
                valid = centered(canvas, method.left, 22, "gear") && valid;
                valid = centered(canvas, method.left + 27, method.right - method.left - 26, "method") && valid;
                if (!valid) return EXIT_FAILURE;
            }
        }
    }
    for (size_t i = 0; i < 16; ++i) {
        assert(storage[i] == 0xAB);
        assert(storage[size + 16 + i] == 0xAB);
    }
    std::puts("PASS: visible status centers and distinct expanded touch targets in all rotations");
}
