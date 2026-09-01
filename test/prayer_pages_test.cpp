#include <cassert>
#include <cstring>
#include <vector>

#include "arabic_prayer_assets.h"
#include "canvas.h"
#include "islamic_background_asset.h"
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
    value.minutes[6] = 1510U;
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

size_t dark_pixels_in_rect(const Canvas &canvas,
                           int left,
                           int top,
                           int right,
                           int bottom)
{
    size_t count = 0U;
    for (int y = top; y < bottom; ++y) {
        for (int x = left; x < right; ++x) {
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
    settings.location_confirmed = true;
    std::strcpy(settings.wifi_ssid, "Home");
    settings.calculation_method = 3U;
    settings.asr_school = 1U;
    PrayerDay today = make_day();
    PrayerPageModel model = {};
    model.settings = &settings;
    model.today = &today;
    model.next = {PrayerId::Maghrib, 1080U, 120U, false};
    model.window = {PrayerId::Asr, 930U, 1080U, 120U};
    model.current_minutes = 960U;
    model.online = true;

    prayer_page_render_schedule(canvas, model);
    const size_t landscape_dark_pixels = dark_pixels(canvas);
    assert(landscape_dark_pixels > 25000U);
    assert(landscape_dark_pixels < 55000U);
    assert(canvas.pixel_at(7, 55) == GrayLevel::Black);
    assert(dark_pixels_below(canvas, 400) > 1000U);

    canvas.set_rotation(CanvasRotation::Deg90Clockwise);
    prayer_page_render_schedule(canvas, model);
    assert(canvas.width() == 480U && canvas.height() == 800U);
    const size_t portrait_dark_pixels = dark_pixels(canvas);
    assert(portrait_dark_pixels > 30000U);
    assert(portrait_dark_pixels < 60000U);
    assert(canvas.pixel_at(44, 590) == GrayLevel::Black);
    assert(dark_pixels_below(canvas, 700) > 2500U);

    prayer_page_render_setup(canvas,
                             "Sticky-Salah-A1B2",
                             "sticky-salah");
    const size_t setup_dark_pixels = dark_pixels(canvas);
    assert(setup_dark_pixels > 35000U);
    assert(setup_dark_pixels < 65000U);
    assert(canvas.pixel_at(12, 12) == GrayLevel::Black);
    assert(canvas.pixel_at(240, 250) == GrayLevel::White);
    assert(dark_pixels_in_rect(canvas, 100, 250, 380, 330) == 0U);
    assert(dark_pixels_in_rect(canvas, 90, 340, 390, 650) > 2500U);
    assert(dark_pixels_below(canvas, 620) > 5000U);

    for (PrayerId prayer : {PrayerId::Fajr,
                            PrayerId::Dhuhr,
                            PrayerId::Asr,
                            PrayerId::Maghrib,
                            PrayerId::Isha}) {
        const ArabicPrayerAsset asset = arabic_prayer_asset(prayer);
        assert(asset.data != nullptr);
        assert(asset.width > 20U && asset.height > 10U);
    }

    const IslamicBackgroundAsset setup_background =
        islamic_setup_background_asset();
    assert(setup_background.width == 480U);
    assert(setup_background.height == 800U);
    const IslamicBackgroundAsset landscape_background =
        islamic_landscape_background_asset();
    assert(landscape_background.width == 800U);
    assert(landscape_background.height == 480U);
    return 0;
}
