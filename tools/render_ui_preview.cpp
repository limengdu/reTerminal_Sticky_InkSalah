#include <cstdio>
#include <cstring>
#include <fstream>
#include <vector>

#include "canvas.h"
#include "prayer_pages.h"

namespace {

PrayerDay sample_day()
{
    PrayerDay day = {};
    day.date = {2026U, 8U, 28U};
    day.minutes[0] = 283U;
    day.minutes[1] = 364U;
    day.minutes[2] = 719U;
    day.minutes[3] = 921U;
    day.minutes[4] = 1075U;
    day.minutes[5] = 1147U;
    day.hijri_year = 1448U;
    day.hijri_month = 2U;
    day.hijri_day = 15U;
    std::strcpy(day.hijri_month_name, "Safar");
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
                     "Usage: render_ui_preview <landscape|portrait> <schedule|setup|error> <output.pgm>\n");
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
    if (std::strcmp(argv[2], "setup") == 0) {
        prayer_page_render_setup(canvas,
                                 "Sticky-Salah-A1B2",
                                 "sticky-salah");
    } else if (std::strcmp(argv[2], "error") == 0) {
        prayer_page_render_error(canvas,
                                 "SETUP UNAVAILABLE",
                                 "RESTART THE DEVICE");
    } else if (std::strcmp(argv[2], "schedule") == 0) {
        SalahSettings settings = {};
        settings_store_defaults(settings);
        settings.configured = true;
        std::strcpy(settings.wifi_ssid, "Home");
        std::strcpy(settings.city, "Shenzhen");
        PrayerDay today = sample_day();
        PrayerPageModel model = {};
        model.settings = &settings;
        model.today = &today;
        model.next = {PrayerId::Asr, 921U, 78U, false};
        model.current_minutes = 843U;
        model.online = false;
        prayer_page_render_schedule(canvas, model);
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
