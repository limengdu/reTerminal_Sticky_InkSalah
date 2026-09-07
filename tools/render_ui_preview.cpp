#include <cstdio>
#include <cstring>
#include <fstream>
#include <vector>

#include "canvas.h"
#include "prayer_pages.h"
#include "prayer_settings_pages.h"
#include "sleep_indicator.h"
#include "battery_status_render.h"

namespace {

PrayerDay sample_day()
{
    PrayerDay day = {};
    day.date = {2026U, 9U, 4U};
    day.minutes[0] = 299U;
    day.minutes[1] = 393U;
    day.minutes[2] = 784U;
    day.minutes[3] = 1002U;
    day.minutes[4] = 1173U;
    day.minutes[5] = 1263U;
    day.minutes[6] = 1554U;
    day.hijri_year = 1448U;
    day.hijri_month = 3U;
    day.hijri_day = 22U;
    std::strcpy(day.hijri_month_name, "Rabi al-Awwal");
    std::strcpy(day.weekday, "Friday");
    return day;
}

uint8_t gray_value(GrayLevel level)
{
    switch (level) {
    case GrayLevel::Black:
        return 0U;
    case GrayLevel::DarkGray:
        return 85U;
    case GrayLevel::LightGray:
        return 170U;
    case GrayLevel::White:
    default:
        return 255U;
    }
}

}  // namespace

int main(int argc, char **argv)
{
    if (argc != 4) {
        std::fprintf(stderr,
                     "Usage: render_ui_preview <landscape|portrait> <schedule|schedule-no-qiyam|schedule-sleep|schedule-no-qiyam-sleep|schedule-long-header|schedule-between|schedule-night|schedule-maghrib|setup|setup-return|error|settings-main|settings-main-first-time|settings-location|settings-calculation|settings-methods|settings-custom|settings-adjustments|settings-editor> <output.pgm>\n");
        return 2;
    }
    const bool portrait = std::strcmp(argv[1], "portrait") == 0;
    if (!portrait && std::strcmp(argv[1], "landscape") != 0) {
        std::fprintf(stderr, "Unknown orientation: %s\n", argv[1]);
        return 2;
    }
    const uint16_t width = portrait ? 480U : 800U;
    const uint16_t height = portrait ? 800U : 480U;
    std::vector<uint8_t> framebuffer(
        static_cast<size_t>(width) * height / 4U, 0xFFU);
    Canvas canvas(width, height, framebuffer.data(), framebuffer.size());
    if (std::strncmp(argv[2], "settings-", 9U) == 0) {
        if (!portrait) {
            std::fprintf(stderr, "Native settings preview is portrait-only.\n");
            return 2;
        }
        SalahSettings settings = {};
        settings_store_defaults(settings);
        std::strcpy(settings.city, "Istanbul");
        std::strcpy(settings.country, "Turkey");
        settings.latitude = 41.0082;
        settings.longitude = 28.9784;
        settings.utc_offset_minutes = 180;
        settings.calculation_method = 13U;
        settings.qiyam_visible = false;
        std::strcpy(settings.wifi_ssid, "Home Wi-Fi");
        settings.configured = true;
        settings.location_confirmed = true;
        DeviceSettingsUiState state = {};
        if (std::strcmp(argv[2], "settings-main-first-time") == 0) {
            state.first_time = true;
            settings.configured = false;
        } else if (std::strcmp(argv[2], "settings-location") == 0) {
            state.page = DeviceSettingsPage::Location;
        } else if (std::strcmp(argv[2], "settings-calculation") == 0) {
            state.page = DeviceSettingsPage::Calculation;
            settings.calculation_method = 3U;
        } else if (std::strcmp(argv[2], "settings-methods") == 0) {
            state.page = DeviceSettingsPage::MethodList;
            state.method_page = 2U;
        } else if (std::strcmp(argv[2], "settings-custom") == 0) {
            state.page = DeviceSettingsPage::CustomMethod;
            settings.calculation_method = kCalculationMethodCustomAngles;
        } else if (std::strcmp(argv[2], "settings-adjustments") == 0) {
            state.page = DeviceSettingsPage::Adjustments;
            settings.prayer_adjustments[0] = 2;
            settings.prayer_adjustments[3] = -3;
        } else if (std::strcmp(argv[2], "settings-editor") == 0) {
            state.page = DeviceSettingsPage::LatitudeEditor;
            std::strcpy(state.editor_value, "-90.0000");
        } else if (std::strcmp(argv[2], "settings-main") != 0) {
            std::fprintf(stderr, "Unknown settings page: %s\n", argv[2]);
            return 2;
        }
        prayer_settings_render(canvas, settings, state);
    } else if (std::strcmp(argv[2], "setup") == 0 ||
               std::strcmp(argv[2], "setup-return") == 0) {
        prayer_page_render_setup(canvas,
                                 "Sticky-Salah-A1B2",
                                 "sticky-salah",
                                 std::strcmp(argv[2], "setup-return") == 0);
    } else if (std::strcmp(argv[2], "error") == 0) {
        prayer_page_render_error(canvas,
                                 "SETUP UNAVAILABLE",
                                 "RESTART THE DEVICE");
    } else if (std::strcmp(argv[2], "schedule") == 0 ||
               std::strcmp(argv[2], "schedule-no-qiyam") == 0 ||
               std::strcmp(argv[2], "schedule-sleep") == 0 ||
               std::strcmp(argv[2], "schedule-no-qiyam-sleep") == 0 ||
               std::strcmp(argv[2], "schedule-long-header") == 0 ||
               std::strcmp(argv[2], "schedule-between") == 0 ||
               std::strcmp(argv[2], "schedule-night") == 0 ||
               std::strcmp(argv[2], "schedule-maghrib") == 0) {
        SalahSettings settings = {};
        settings_store_defaults(settings);
        settings.configured = true;
        settings.location_confirmed = true;
        std::strcpy(settings.wifi_ssid, "Home");
        std::strcpy(settings.city, "Istanbul");
        settings.calculation_method = 13U;
        settings.asr_school = 1U;
        settings.qiyam_visible = std::strcmp(argv[2], "schedule") == 0 ||
                                  std::strcmp(argv[2], "schedule-sleep") == 0;
        if (std::strcmp(argv[2], "schedule-long-header") == 0) {
            std::strcpy(settings.city, "San Francisco Bay Area");
            settings.calculation_method = 15U;
        }
        PrayerDay today = sample_day();
        PrayerPageModel model = {};
        model.settings = &settings;
        model.today = &today;
        model.next = {PrayerId::Asr, 1002U, 154U, false};
        model.window = {PrayerId::Dhuhr, 784U, 1002U, 154U};
        model.current_minutes = 848U;
        model.online = false;
        if (std::strcmp(argv[2], "schedule-between") == 0) {
            model.window.between_prayers = true;
            model.next = {PrayerId::Dhuhr, 784U, 250U, false};
        } else if (std::strcmp(argv[2], "schedule-night") == 0) {
            model.window = {PrayerId::Isha, 1263U, 1739U, 380U};
            model.next = {PrayerId::Fajr, 299U, 380U, true};
        } else if (std::strcmp(argv[2], "schedule-maghrib") == 0) {
            model.window = {PrayerId::Maghrib, 1173U, 1263U, 90U};
            model.next = {PrayerId::Isha, 1263U, 90U, false};
        }
        prayer_page_render_schedule(canvas, model);
        battery_status_render(canvas, true, 100, true, false);
        if (std::strcmp(argv[2], "schedule-sleep") == 0 ||
            std::strcmp(argv[2], "schedule-no-qiyam-sleep") == 0) {
            sleep_indicator_render(canvas);
        }
    } else {
        std::fprintf(stderr, "Unknown page: %s\n", argv[2]);
        return 2;
    }

    std::ofstream output(argv[3], std::ios::binary);
    output << "P5\n" << canvas.width() << " " << canvas.height() << "\n255\n";
    for (int y = 0; y < canvas.height(); ++y) {
        for (int x = 0; x < canvas.width(); ++x) {
            output.put(static_cast<char>(gray_value(canvas.pixel_at(x, y))));
        }
    }
    return output.good() ? 0 : 1;
}
