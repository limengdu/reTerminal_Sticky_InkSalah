#include <cassert>
#include <cstring>
#include <vector>

#include "arabic_prayer_assets.h"
#include "canvas.h"
#include "font.h"
#include "prayer_pages.h"
#include "ui_style.h"

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
        assert(asset.data != nullptr && asset.width > 20 && asset.height >= 18);
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
        assert(canvas.pixel_at(0, 30) == GrayLevel::Black);
        assert(canvas.pixel_at(canvas.width() / 2, 185) == GrayLevel::Black);
        const std::vector<uint8_t> six = storage;
        settings.qiyam_visible = true;
        prayer_page_render_schedule(canvas, model);
        assert(storage != six);
        model.window.between_prayers = true;
        prayer_page_render_schedule(canvas, model);
        model.window.between_prayers = false;
        assert(prayer_page_settings_action_at(canvas.width() - 40, 55, canvas.width(), canvas.height()));
        assert(!prayer_page_settings_action_at(100, 100, canvas.width(), canvas.height()));
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
