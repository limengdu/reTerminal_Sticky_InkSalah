#include "prayer_pages.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

#include "arabic_prayer_assets.h"
#include "islamic_background_asset.h"

namespace {

constexpr PrayerId kPrayers[] = {
    PrayerId::Fajr,
    PrayerId::Dhuhr,
    PrayerId::Asr,
    PrayerId::Maghrib,
    PrayerId::Isha,
};

int text_width(const char *text, uint8_t scale)
{
    return text == nullptr
               ? 0
               : static_cast<int>(std::strlen(text)) * 6 * scale;
}

void draw_centered(Canvas &canvas,
                   int center_x,
                   int y,
                   const char *text,
                   uint8_t scale,
                   GrayLevel color = GrayLevel::Black)
{
    canvas.draw_text(center_x - text_width(text, scale) / 2,
                     y,
                     text,
                     scale,
                     color);
}

void format_time(uint16_t minutes, char output[6])
{
    const uint8_t hour = static_cast<uint8_t>((minutes / 60U) % 24U);
    const uint8_t minute = static_cast<uint8_t>(minutes % 60U);
    output[0] = static_cast<char>('0' + hour / 10U);
    output[1] = static_cast<char>('0' + hour % 10U);
    output[2] = ':';
    output[3] = static_cast<char>('0' + minute / 10U);
    output[4] = static_cast<char>('0' + minute % 10U);
    output[5] = '\0';
}

void format_countdown(uint16_t minutes, char *output, size_t capacity)
{
    if (minutes == 0U) {
        std::snprintf(output, capacity, "Now");
    } else if (minutes < 60U) {
        std::snprintf(output,
                      capacity,
                      "in %u min",
                      static_cast<unsigned>(minutes));
    } else {
        std::snprintf(output,
                      capacity,
                      "in %uh %02um",
                      static_cast<unsigned>(minutes / 60U),
                      static_cast<unsigned>(minutes % 60U));
    }
}

void draw_arabic_asset(Canvas &canvas,
                       int center_x,
                       int y,
                       PrayerId prayer,
                       GrayLevel color)
{
    const ArabicPrayerAsset asset = arabic_prayer_asset(prayer);
    if (asset.data == nullptr || asset.width == 0U || asset.height == 0U) {
        return;
    }
    const int start_x = center_x - static_cast<int>(asset.width) / 2;
    for (uint16_t row = 0U; row < asset.height; ++row) {
        for (uint16_t column = 0U; column < asset.width; ++column) {
            const uint8_t packed = asset.data[
                static_cast<size_t>(row) * asset.stride + column / 8U];
            if ((packed & static_cast<uint8_t>(1U << (7U - column % 8U))) != 0U) {
                canvas.draw_pixel(start_x + column, y + row, color);
            }
        }
    }
}

void draw_background_asset(Canvas &canvas,
                           const IslamicBackgroundAsset &asset)
{
    if (asset.data == nullptr || asset.width != canvas.width() ||
        asset.height != canvas.height()) {
        return;
    }
    for (uint16_t row = 0U; row < asset.height; ++row) {
        for (uint16_t column = 0U; column < asset.width; ++column) {
            const uint8_t packed = asset.data[
                static_cast<size_t>(row) * asset.stride + column / 8U];
            if ((packed & static_cast<uint8_t>(1U << (7U - column % 8U))) != 0U) {
                canvas.draw_pixel(column, row, GrayLevel::Black);
            }
        }
    }
}

void draw_portrait_header(Canvas &canvas, const PrayerPageModel &model)
{
    // Keep the battery overlay and the two date lines on a clean white band.
    // 为电量图标和两行日期保留干净的白色顶栏。
    canvas.fill_rect(0, 0, static_cast<int>(canvas.width()), 58,
                     GrayLevel::White);
    const size_t city_length = std::strlen(model.settings->city);
    canvas.draw_text(18,
                     12,
                     model.settings->city,
                     city_length <= 15U ? 2U : 1U,
                     GrayLevel::Black);

    char gregorian[32] = {};
    std::snprintf(gregorian,
                  sizeof(gregorian),
                  "%s  %02u/%02u/%04u",
                  model.today->weekday,
                  static_cast<unsigned>(model.today->date.day),
                  static_cast<unsigned>(model.today->date.month),
                  static_cast<unsigned>(model.today->date.year));
    draw_centered(canvas, 240, 36, gregorian, 1, GrayLevel::Black);

    char hijri[48] = {};
    std::snprintf(hijri,
                  sizeof(hijri),
                  "%u %s %u AH",
                  static_cast<unsigned>(model.today->hijri_day),
                  prayer_hijri_month_name(model.today->hijri_month),
                  static_cast<unsigned>(model.today->hijri_year));
    draw_centered(canvas, 240, 49, hijri, 1, GrayLevel::Black);
}

void draw_timeline_marker(Canvas &canvas, int x, int y, bool selected)
{
    if (selected) {
        canvas.fill_circle(x, y, 7, GrayLevel::Black);
        canvas.draw_circle(x, y, 10, GrayLevel::Black);
        return;
    }
    canvas.fill_circle(x, y, 3, GrayLevel::Black);
    canvas.draw_circle(x, y, 6, GrayLevel::Black);
}

void draw_landscape_header(Canvas &canvas, const PrayerPageModel &model)
{
    canvas.fill_rect(0, 0, static_cast<int>(canvas.width()), 54,
                     GrayLevel::White);
    const size_t city_length = std::strlen(model.settings->city);
    canvas.draw_text(18,
                     12,
                     model.settings->city,
                     city_length <= 18U ? 2U : 1U,
                     GrayLevel::Black);

    char gregorian[32] = {};
    std::snprintf(gregorian,
                  sizeof(gregorian),
                  "%s  %02u/%02u/%04u",
                  model.today->weekday,
                  static_cast<unsigned>(model.today->date.day),
                  static_cast<unsigned>(model.today->date.month),
                  static_cast<unsigned>(model.today->date.year));
    draw_centered(canvas, 410, 12, gregorian, 2, GrayLevel::Black);

    char hijri[48] = {};
    std::snprintf(hijri,
                  sizeof(hijri),
                  "%u %s %u AH",
                  static_cast<unsigned>(model.today->hijri_day),
                  prayer_hijri_month_name(model.today->hijri_month),
                  static_cast<unsigned>(model.today->hijri_year));
    draw_centered(canvas, 410, 37, hijri, 1, GrayLevel::Black);
    canvas.draw_text(622,
                     37,
                     model.online ? "SYNCED" : "CACHED",
                     1,
                     GrayLevel::Black);
}

void draw_landscape_schedule(Canvas &canvas, const PrayerPageModel &model)
{
    draw_background_asset(canvas, islamic_landscape_background_asset());
    draw_landscape_header(canvas, model);

    draw_centered(canvas,
                  200,
                  142,
                  model.next.tomorrow ? "TOMORROW'S FIRST PRAYER"
                                      : "NEXT PRAYER",
                  2,
                  GrayLevel::Black);
    draw_centered(canvas,
                  200,
                  178,
                  prayer_id_name(model.next.prayer),
                  6,
                  GrayLevel::Black);
    char next_time[6] = {};
    format_time(model.next.time_minutes, next_time);
    draw_centered(canvas, 200, 232, next_time, 7, GrayLevel::Black);
    draw_arabic_asset(canvas,
                      200,
                      293,
                      model.next.prayer,
                      GrayLevel::Black);
    char countdown[24] = {};
    format_countdown(model.next.minutes_until,
                     countdown,
                     sizeof(countdown));
    draw_centered(canvas, 200, 338, countdown, 2, GrayLevel::Black);

    char sunrise[16] = {};
    format_time(model.today->minutes[
                    static_cast<size_t>(PrayerTimeSlot::Sunrise)],
                next_time);
    std::snprintf(sunrise, sizeof(sunrise), "SUNRISE %s", next_time);
    draw_centered(canvas, 200, 378, sunrise, 1, GrayLevel::Black);

    canvas.draw_text(456, 126, "TODAY'S PRAYERS", 2, GrayLevel::Black);
    constexpr int kTimelineX = 454;
    constexpr int kFirstCenterY = 166;
    constexpr int kRowStep = 47;
    canvas.draw_line(kTimelineX,
                     kFirstCenterY,
                     kTimelineX,
                     kFirstCenterY + 4 * kRowStep,
                     GrayLevel::Black);
    for (size_t index = 0U; index < 5U; ++index) {
        const PrayerId prayer = kPrayers[index];
        const uint16_t prayer_minutes = model.today->minutes[
            static_cast<size_t>(prayer_id_slot(prayer))];
        const bool selected = prayer == model.next.prayer && !model.next.tomorrow;
        const int center_y = kFirstCenterY + static_cast<int>(index) * kRowStep;
        draw_timeline_marker(canvas, kTimelineX, center_y, selected);
        if (selected) {
            canvas.draw_line(476, center_y + 20, 780, center_y + 20,
                             GrayLevel::Black);
        }
        canvas.draw_text(478,
                         center_y - 7,
                         prayer_id_name(prayer),
                         2,
                         GrayLevel::Black);
        char time[6] = {};
        format_time(prayer_minutes, time);
        canvas.draw_text(714, center_y - 7, time, 2, GrayLevel::Black);
        draw_arabic_asset(canvas,
                          632,
                          center_y - 16,
                          prayer,
                          GrayLevel::Black);
    }
}

void draw_portrait_schedule(Canvas &canvas, const PrayerPageModel &model)
{
    draw_background_asset(canvas, islamic_prayer_background_asset());
    draw_portrait_header(canvas, model);

    draw_centered(canvas,
                  240,
                  178,
                  model.next.tomorrow ? "TOMORROW'S FIRST PRAYER"
                                      : "NEXT PRAYER",
                  2,
                  GrayLevel::Black);
    draw_centered(canvas,
                  240,
                  212,
                  prayer_id_name(model.next.prayer),
                  6,
                  GrayLevel::Black);
    char next_time[6] = {};
    format_time(model.next.time_minutes, next_time);
    draw_centered(canvas, 240, 268, next_time, 8, GrayLevel::Black);
    draw_arabic_asset(canvas,
                      240,
                      335,
                      model.next.prayer,
                      GrayLevel::Black);
    char countdown[24] = {};
    format_countdown(model.next.minutes_until,
                     countdown,
                     sizeof(countdown));
    draw_centered(canvas, 240, 378, countdown, 2, GrayLevel::Black);

    canvas.draw_line(96, 411, 216, 411, GrayLevel::Black);
    canvas.draw_circle(240, 411, 4, GrayLevel::Black);
    canvas.draw_line(264, 411, 384, 411, GrayLevel::Black);
    canvas.draw_text(24, 428, "TODAY'S PRAYERS", 2, GrayLevel::Black);
    char sunrise[16] = {};
    format_time(model.today->minutes[
                    static_cast<size_t>(PrayerTimeSlot::Sunrise)],
                next_time);
    std::snprintf(sunrise, sizeof(sunrise), "SUNRISE %s", next_time);
    canvas.draw_text(352, 431, sunrise, 1, GrayLevel::Black);

    constexpr int kTimelineX = 44;
    constexpr int kFirstCenterY = 468;
    constexpr int kRowStep = 51;
    canvas.draw_line(kTimelineX,
                     kFirstCenterY,
                     kTimelineX,
                     kFirstCenterY + 4 * kRowStep,
                     GrayLevel::Black);
    for (size_t index = 0U; index < 5U; ++index) {
        const PrayerId prayer = kPrayers[index];
        const uint16_t prayer_minutes = model.today->minutes[
            static_cast<size_t>(prayer_id_slot(prayer))];
        const bool selected = prayer == model.next.prayer && !model.next.tomorrow;
        const int center_y = kFirstCenterY + static_cast<int>(index) * kRowStep;
        draw_timeline_marker(canvas, kTimelineX, center_y, selected);
        if (selected) {
            canvas.draw_line(64, center_y + 22, 454, center_y + 22,
                             GrayLevel::Black);
        }
        canvas.draw_text(68,
                         center_y - 10,
                         prayer_id_name(prayer),
                         3,
                         GrayLevel::Black);
        char time[6] = {};
        format_time(prayer_minutes, time);
        canvas.draw_text(374, center_y - 10, time, 3, GrayLevel::Black);
        draw_arabic_asset(canvas,
                          278,
                          center_y - 16,
                          prayer,
                          GrayLevel::Black);
    }
}

}  // namespace

void prayer_page_render_setup(Canvas &canvas,
                              const char *ap_ssid,
                              const char *ap_password)
{
    canvas.clear();
    if (canvas.width() < canvas.height()) {
        draw_background_asset(canvas, islamic_setup_background_asset());
        canvas.fill_rect(88, 136, 304, 96, GrayLevel::White);
        draw_centered(canvas,
                      static_cast<int>(canvas.width()) / 2,
                      150,
                      "STICKY SALAH",
                      4);
        draw_centered(canvas,
                      static_cast<int>(canvas.width()) / 2,
                      198,
                      "FIRST-TIME WI-FI SETUP",
                      2,
                      GrayLevel::Black);

        draw_centered(canvas,
                      static_cast<int>(canvas.width()) / 2,
                      270,
                      "1  CONNECT YOUR PHONE TO",
                      2,
                      GrayLevel::Black);
        draw_centered(canvas,
                      static_cast<int>(canvas.width()) / 2,
                      310,
                      ap_ssid,
                      std::strlen(ap_ssid) <= 31U ? 2U : 1U,
                      GrayLevel::Black);
        canvas.draw_line(100, 350, 380, 350, GrayLevel::Black);

        draw_centered(canvas,
                      static_cast<int>(canvas.width()) / 2,
                      370,
                      "2  ENTER THE PASSWORD",
                      2,
                      GrayLevel::Black);
        draw_centered(canvas,
                      static_cast<int>(canvas.width()) / 2,
                      410,
                      ap_password,
                      3,
                      GrayLevel::Black);
        canvas.draw_line(100, 454, 380, 454, GrayLevel::Black);

        draw_centered(canvas,
                      static_cast<int>(canvas.width()) / 2,
                      476,
                      "3  OPEN THE SETUP PAGE",
                      2,
                      GrayLevel::Black);
        draw_centered(canvas,
                      static_cast<int>(canvas.width()) / 2,
                      520,
                      "192.168.4.1",
                      3,
                      GrayLevel::Black);
        draw_centered(canvas,
                      static_cast<int>(canvas.width()) / 2,
                      582,
                      "THE PAGE SHOULD OPEN AUTOMATICALLY",
                      1,
                      GrayLevel::Black);
        return;
    }
    draw_centered(canvas,
                  static_cast<int>(canvas.width()) / 2,
                  54,
                  "STICKY SALAH",
                  5);
    draw_centered(canvas,
                  static_cast<int>(canvas.width()) / 2,
                  128,
                  "FIRST-TIME SETUP",
                  3,
                  GrayLevel::DarkGray);
    canvas.draw_rect(72, 188, static_cast<int>(canvas.width()) - 144, 218);
    canvas.draw_text(104, 218, "1. CONNECT PHONE TO", 2);
    draw_centered(canvas,
                  static_cast<int>(canvas.width()) / 2,
                  258,
                  ap_ssid,
                  3);
    canvas.draw_text(104, 310, "2. PASSWORD", 2);
    draw_centered(canvas,
                  static_cast<int>(canvas.width()) / 2,
                  344,
                  ap_password,
                  3);
    draw_centered(canvas,
                  static_cast<int>(canvas.width()) / 2,
                  430,
                  "OPEN 192.168.4.1",
                  2,
                  GrayLevel::DarkGray);
}

void prayer_page_render_error(Canvas &canvas,
                              const char *title,
                              const char *message)
{
    canvas.clear();
    if (canvas.width() < canvas.height()) {
        draw_centered(canvas,
                      static_cast<int>(canvas.width()) / 2,
                      160,
                      title,
                      3);
        canvas.draw_rect(30,
                         252,
                         static_cast<int>(canvas.width()) - 60,
                         176);
        draw_centered(canvas,
                      static_cast<int>(canvas.width()) / 2,
                      324,
                      message,
                      1,
                      GrayLevel::DarkGray);
        draw_centered(canvas,
                      static_cast<int>(canvas.width()) / 2,
                      548,
                      "PRESS AI BUTTON TO RETRY",
                      2);
        return;
    }
    draw_centered(canvas,
                  static_cast<int>(canvas.width()) / 2,
                  116,
                  title,
                  4);
    canvas.draw_rect(72, 188, static_cast<int>(canvas.width()) - 144, 144);
    draw_centered(canvas,
                  static_cast<int>(canvas.width()) / 2,
                  242,
                  message,
                  2,
                  GrayLevel::DarkGray);
    draw_centered(canvas,
                  static_cast<int>(canvas.width()) / 2,
                  382,
                  "PRESS AI BUTTON TO RETRY",
                  2);
}

void prayer_page_render_schedule(Canvas &canvas,
                                 const PrayerPageModel &model)
{
    canvas.clear();
    if (model.settings == nullptr || model.today == nullptr ||
        model.next.prayer == PrayerId::None) {
        prayer_page_render_error(canvas,
                                 "SCHEDULE UNAVAILABLE",
                                 "CONNECT WI-FI TO REFRESH TIMES");
        return;
    }
    if (canvas.width() >= canvas.height()) {
        draw_landscape_schedule(canvas, model);
    } else {
        draw_portrait_schedule(canvas, model);
    }
}
