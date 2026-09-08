#include <algorithm>
#include <cassert>
#include <cstring>
#include <vector>

#include "arabic_prayer_assets.h"
#include "canvas.h"
#include "font.h"
#include "prayer_pages.h"
#include "ui_style.h"

namespace {

bool inside_capsule(int x, int y, int width, int height)
{
    if (x < 0 || y < 0 || x >= width || y >= height) return false;
    const int radius = height / 2;
    const int dx = std::max(0, 2 * radius - 1 - 2 * std::min(x, width - 1 - x));
    const int dy = std::max(0, 2 * radius - 1 - 2 * std::min(y, height - 1 - y));
    return dx * dx + dy * dy <= 4 * radius * radius;
}

// Checks every border pixel while leaving the countdown text and icon unrestricted.
// 检查边框的每个像素，内部倒计时文字和图标保持独立。
void check_countdown_border(const Canvas &canvas)
{
    const bool portrait = canvas.width() < canvas.height();
    const int width = portrait ? 268 : 288;
    const int height = portrait ? 50 : 52;
    const int left = (portrait ? 240 : 208) - width / 2;
    const int top = portrait ? 388 : 366;
    for (int y = -1; y <= height; ++y) {
        for (int x = -1; x <= width; ++x) {
            if (inside_capsule(x - 2, y - 2, width - 4, height - 4)) continue;
            const auto expected = inside_capsule(x, y, width, height)
                                      ? GrayLevel::Black : GrayLevel::White;
            assert(canvas.pixel_at(left + x, top + y) == expected);
        }
    }
}

}  // namespace

int main()
{
    char time[16] = {};
    prayer_page_format_time(0, time, sizeof(time));
    assert(std::strcmp(time, "12:00 AM") == 0);
    prayer_page_format_time(720, time, sizeof(time));
    assert(std::strcmp(time, "12:00 PM") == 0);
    prayer_page_format_time(1574, time, sizeof(time));
    assert(std::strcmp(time, "02:14 AM") == 0);
    assert(Canvas::text_width("WWW", 24) > Canvas::text_width("iii", 24) * 2);
    assert(font_glyph(font_face(24, false), 0x0130).codepoint == 0x0130);
    assert(font_face(1, false).size == 20);
    assert(font_face(25, true).bold);
    const char *broken = "\xE2";
    assert(font_next_codepoint(broken) == '?');
    assert(*broken == '\0');
    char fitted[80] = {};
    ui::fit("Very long city name with multiple words", fitted, sizeof(fitted), 145, 22, TextWeight::Bold);
    assert(Canvas::text_width(fitted, 22, TextWeight::Bold) <= 145);
    for (int slot = 0; slot < 7; ++slot) {
        auto asset = arabic_time_asset(static_cast<PrayerTimeSlot>(slot));
        assert(asset.data != nullptr && asset.width > 20 && asset.height >= 14);
    }
    for (auto prayer : {PrayerId::Fajr, PrayerId::Dhuhr, PrayerId::Asr,
                        PrayerId::Maghrib, PrayerId::Isha}) {
        const auto row = arabic_prayer_asset(prayer);
        const auto portrait = arabic_prayer_hero_asset(prayer, true);
        const auto landscape = arabic_prayer_hero_asset(prayer, false);
        assert(row.width < portrait.width && portrait.width < landscape.width);
        assert(row.height < portrait.height && portrait.height < landscape.height);
    }
    for (auto asset : {inksalah_book_asset(), inksalah_gear_asset()}) {
        size_t pixels = 0;
        for (int y = 0; y < asset.height; ++y)
            for (int x = 0; x < asset.width; ++x)
                if ((asset.data[y * asset.stride + x / 8] & (1U << (7 - x % 8))) != 0U) ++pixels;
        assert(pixels > 100U && pixels < static_cast<size_t>(asset.width * asset.height));
    }

    constexpr size_t size = 800U * 480U / 4U;
    std::vector<uint8_t> storage(size + 32U, 0xABU);
    Canvas canvas(800, 480, storage.data() + 16, size);
    SalahSettings settings = {};
    settings_store_defaults(settings);
    std::strcpy(settings.city, "Istanbul");
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
        settings.qiyam_visible = false;
        prayer_page_render_schedule(canvas, model);
        check_countdown_border(canvas);
        assert(canvas.prayer_header());
        assert(canvas.pixel_at(0, 30) == GrayLevel::Black);
        assert(canvas.pixel_at(canvas.width() / 2, 185) == GrayLevel::Black);
        const std::vector<uint8_t> six = storage;
        settings.qiyam_visible = true;
        prayer_page_render_schedule(canvas, model);
        check_countdown_border(canvas);
        assert(storage != six);
        model.window.between_prayers = true;
        prayer_page_render_schedule(canvas, model);
        check_countdown_border(canvas);
        model.window.between_prayers = false;
        assert(prayer_page_header_action_at(canvas.width() - 40, 55,
                                             canvas.width(), canvas.height()) == PrayerHeaderAction::Settings);
        assert(prayer_page_header_action_at(100, 100,
                                             canvas.width(), canvas.height()) == PrayerHeaderAction::None);
        assert(prayer_page_header_action_at(canvas.width() / 2, 35,
                                             canvas.width(), canvas.height()) == PrayerHeaderAction::Location);
        std::strcpy(settings.city, "San Francisco Bay Area");
        const uint8_t methods[] = {0U, 13U, 15U, kCalculationMethodCustomAngles};
        for (uint8_t method : methods) {
            settings.calculation_method = method;
            prayer_page_render_schedule(canvas, model);
        }
        for (auto prayer : {PrayerId::Fajr, PrayerId::Dhuhr, PrayerId::Asr,
                            PrayerId::Maghrib, PrayerId::Isha}) {
            model.window.prayer = prayer;
            model.window.start_minutes = 1263;
            model.window.end_minutes = 1739;
            model.window.minutes_remaining = 476;
            prayer_page_render_schedule(canvas, model);
        }
        model.window.prayer = PrayerId::Dhuhr;
        for (size_t i = 0; i < 16; ++i) {
            assert(storage[i] == 0xABU);
            assert(storage[size + 16 + i] == 0xABU);
        }
    }
    canvas.set_rotation(CanvasRotation::Deg90Clockwise);
    for (const char *text : {"01 Connect Your Phone to", "02 Enter The Passwords",
                             "03 Open The Setup Page", "(Just in case it doesn't opt in)"}) {
        assert(Canvas::text_width(text, 20) <= 328);
    }
    prayer_page_render_setup(canvas, "Sticky-Salah-A1B2", "sticky-salah", false);
    assert(!canvas.prayer_header());
    const std::vector<uint8_t> first_setup = storage;
    assert(!prayer_page_setup_back_action_at(240, 690, false));
    prayer_page_render_setup(canvas, "Sticky-Salah-A1B2", "sticky-salah", true);
    assert(first_setup != storage);
    assert(prayer_page_setup_back_action_at(240, 690, true));
    assert(!prayer_page_setup_back_action_at(240, 650, true));
    model.today = nullptr;
    prayer_page_render_schedule(canvas, model);
    return 0;
}
