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

size_t asset_pixels_in_rect(const IslamicBackgroundAsset &asset,
                            int left,
                            int top,
                            int right,
                            int bottom)
{
    size_t count = 0U;
    for (int y = top; y < bottom; ++y) {
        for (int x = left; x < right; ++x) {
            const uint8_t packed = asset.data[
                static_cast<size_t>(y) * asset.stride + x / 8];
            if ((packed & static_cast<uint8_t>(1U << (7 - x % 8))) != 0U) {
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
    assert(dark_pixels_in_rect(canvas, 0, 60, 160, 120) > 400U);
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

    const IslamicBackgroundAsset portrait_background =
        islamic_prayer_background_asset();
    assert(asset_pixels_in_rect(
               portrait_background, 0, 0, 480, 60) == 0U);
    assert(asset_pixels_in_rect(
               portrait_background, 0, 60, 480, 190) > 4000U);
    assert(asset_pixels_in_rect(
               portrait_background, 0, 182, 480, 638) == 0U);
    assert(asset_pixels_in_rect(
               portrait_background, 0, 638, 480, 780) > 4000U);
    const size_t portrait_left_pixels = asset_pixels_in_rect(
        portrait_background, 0, 0, 240, 800);
    const size_t portrait_right_pixels = asset_pixels_in_rect(
        portrait_background, 240, 0, 480, 800);
    assert(asset_pixels_in_rect(
               portrait_background, 0, 780, 480, 800) == 0U);
    assert(portrait_left_pixels > portrait_right_pixels
               ? portrait_left_pixels - portrait_right_pixels < 1000U
               : portrait_right_pixels - portrait_left_pixels < 1000U);

    const IslamicBackgroundAsset landscape_background =
        islamic_landscape_background_asset();
    assert(asset_pixels_in_rect(
               landscape_background, 0, 0, 800, 60) == 0U);
    assert(asset_pixels_in_rect(
               landscape_background, 0, 60, 800, 230) > 6000U);
    assert(asset_pixels_in_rect(
               landscape_background, 105, 230, 375, 450) == 0U);
    assert(asset_pixels_in_rect(
               landscape_background, 425, 230, 695, 450) == 0U);
    assert(asset_pixels_in_rect(
               landscape_background, 380, 110, 420, 400) > 500U);
    assert(asset_pixels_in_rect(
               landscape_background, 0, 380, 110, 480) > 300U);
    assert(asset_pixels_in_rect(
               landscape_background, 690, 380, 800, 480) > 300U);
    const size_t landscape_left_pixels = asset_pixels_in_rect(
        landscape_background, 0, 0, 400, 480);
    const size_t landscape_right_pixels = asset_pixels_in_rect(
        landscape_background, 400, 0, 800, 480);
    assert(landscape_left_pixels > landscape_right_pixels
               ? landscape_left_pixels - landscape_right_pixels < 1500U
               : landscape_right_pixels - landscape_left_pixels < 1500U);

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
    assert(landscape_background.width == 800U);
    assert(landscape_background.height == 480U);
    return 0;
}
