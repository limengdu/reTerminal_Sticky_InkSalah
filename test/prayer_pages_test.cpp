#include <cassert>
#include <cstring>
#include <vector>

#include "arabic_prayer_assets.h"
#include "canvas.h"
#include "prayer_pages.h"

PrayerDay make_day()
{
    PrayerDay value = {};
    value.date = {2026U, 8U, 28U};
    value.minutes[0] = 285U;
    value.minutes[1] = 365U;
    value.minutes[2] = 720U;
    value.minutes[3] = 930U;
    value.minutes[4] = 1080U;
    value.minutes[5] = 1150U;
    value.hijri_year = 1448U;
    value.hijri_month = 2U;
    value.hijri_day = 15U;
    std::strcpy(value.hijri_month_name, "Safar");
    std::strcpy(value.weekday, "Friday");
    return value;
}

size_t dark_pixels(const Canvas &canvas)
{
    size_t count = 0U;
    for (int y = 0; y < canvas.height(); ++y) {
        for (int x = 0; x < canvas.width(); ++x) {
            if (canvas.pixel_at(x, y) <= GrayLevel::DarkGray) {
                ++count;
            }
        }
    }
    return count;
}

size_t dark_pixels_below(const Canvas &canvas, int start_y)
{
    size_t count = 0U;
    for (int y = start_y; y < canvas.height(); ++y) {
        for (int x = 0; x < canvas.width(); ++x) {
            if (canvas.pixel_at(x, y) <= GrayLevel::DarkGray) {
                ++count;
            }
        }
    }
    return count;
}

int main()
{
    std::vector<uint8_t> buffer(800U * 480U / 4U, 0xFFU);
    Canvas canvas(800U, 480U, buffer.data(), buffer.size());
    SalahSettings settings = {};
    settings_store_defaults(settings);
    settings.configured = true;
    std::strcpy(settings.wifi_ssid, "Home");
    PrayerDay today = make_day();
    PrayerPageModel model = {};
    model.settings = &settings;
    model.today = &today;
    model.next = {PrayerId::Asr, 930U, 42U, false};
    model.current_minutes = 888U;
    model.online = true;

    prayer_page_render_schedule(canvas, model);
    assert(dark_pixels(canvas) > 80000U);
    assert(canvas.pixel_at(30, 80) == GrayLevel::Black);

    canvas.set_rotation(CanvasRotation::Deg90Clockwise);
    prayer_page_render_schedule(canvas, model);
    assert(canvas.width() == 480U && canvas.height() == 800U);
    const size_t portrait_dark_pixels = dark_pixels(canvas);
    assert(portrait_dark_pixels > 30000U);
    assert(portrait_dark_pixels < 60000U);
    assert(canvas.pixel_at(30, 220) == GrayLevel::White);
    assert(canvas.pixel_at(54, 570) == GrayLevel::Black);
    assert(canvas.pixel_at(54, 468) == GrayLevel::White);
    assert(dark_pixels_below(canvas, 700) > 2500U);

    prayer_page_render_setup(canvas,
                             "Sticky-Salah-A1B2",
                             "sticky-salah");
    assert(canvas.pixel_at(30, 196) == GrayLevel::Black);
    assert(dark_pixels_below(canvas, 620) > 500U);

    for (PrayerId prayer : {PrayerId::Fajr,
                            PrayerId::Dhuhr,
                            PrayerId::Asr,
                            PrayerId::Maghrib,
                            PrayerId::Isha}) {
        const ArabicPrayerAsset asset = arabic_prayer_asset(prayer);
        assert(asset.data != nullptr);
        assert(asset.width > 20U && asset.height > 10U);
    }
    return 0;
}
