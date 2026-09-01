#include "prayer_pages.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

#include "arabic_prayer_assets.h"
#include "islamic_background_asset.h"

namespace {

struct TimelineEntry {
    const char *name;
    PrayerTimeSlot slot;
    PrayerId prayer;
};

constexpr TimelineEntry kTimeline[] = {
    {"Fajr", PrayerTimeSlot::Fajr, PrayerId::Fajr},
    {"Sunrise", PrayerTimeSlot::Sunrise, PrayerId::None},
    {"Dhuhr", PrayerTimeSlot::Dhuhr, PrayerId::Dhuhr},
    {"Asr", PrayerTimeSlot::Asr, PrayerId::Asr},
    {"Maghrib", PrayerTimeSlot::Maghrib, PrayerId::Maghrib},
    {"Isha", PrayerTimeSlot::Isha, PrayerId::Isha},
    {"Qiyam", PrayerTimeSlot::Qiyam, PrayerId::None},
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

void ellipsize(const char *text,
               size_t maximum_characters,
               char *output,
               size_t capacity)
{
    if (output == nullptr || capacity == 0U) {
        return;
    }
    output[0] = '\0';
    if (text == nullptr) {
        return;
    }
    const size_t length = std::strlen(text);
    if (length <= maximum_characters) {
        std::snprintf(output, capacity, "%s", text);
        return;
    }
    const size_t kept = maximum_characters > 3U
                            ? maximum_characters - 3U
                            : 0U;
    std::snprintf(output,
                  capacity,
                  "%.*s...",
                  static_cast<int>(kept),
                  text);
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

void format_remaining(uint16_t minutes, char *output, size_t capacity)
{
    if (minutes < 60U) {
        std::snprintf(output,
                      capacity,
                      "%u min remaining",
                      static_cast<unsigned>(minutes));
    } else {
        std::snprintf(output,
                      capacity,
                      "%uh %02um remaining",
                      static_cast<unsigned>(minutes / 60U),
                      static_cast<unsigned>(minutes % 60U));
    }
}

void format_source(const SalahSettings &settings,
                   char *output,
                   size_t capacity)
{
    std::snprintf(
        output,
        capacity,
        "%s / %s / LOCAL",
        settings_method_short_name(settings.calculation_method),
        settings_asr_school_name(settings.asr_school));
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
    constexpr int kPageCenterX = 240;
    // Center three metadata rows above the mihrab ornament.
    // 在米哈拉布花纹上方居中排列三行信息。
    canvas.fill_rect(0, 0, static_cast<int>(canvas.width()), 68,
                     GrayLevel::White);
    char city[20] = {};
    ellipsize(model.settings->city, 18U, city, sizeof(city));
    draw_centered(canvas,
                  kPageCenterX,
                  4,
                  city,
                  2U,
                  GrayLevel::Black);

    char gregorian[32] = {};
    std::snprintf(gregorian,
                  sizeof(gregorian),
                  "%s  %02u/%02u/%04u",
                  model.today->weekday,
                  static_cast<unsigned>(model.today->date.day),
                  static_cast<unsigned>(model.today->date.month),
                  static_cast<unsigned>(model.today->date.year));
    draw_centered(canvas,
                  kPageCenterX,
                  26,
                  gregorian,
                  2,
                  GrayLevel::Black);

    char hijri[48] = {};
    std::snprintf(hijri,
                  sizeof(hijri),
                  "%u %s %u AH",
                  static_cast<unsigned>(model.today->hijri_day),
                  prayer_hijri_month_name(model.today->hijri_month),
                  static_cast<unsigned>(model.today->hijri_year));
    char source[40] = {};
    format_source(*model.settings, source, sizeof(source));
    char metadata[96] = {};
    std::snprintf(metadata,
                  sizeof(metadata),
                  "%s  %s",
                  hijri,
                  source);
    draw_centered(canvas,
                  kPageCenterX,
                  48,
                  metadata,
                  2,
                  GrayLevel::Black);
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
    constexpr int kPageCenterX = 400;
    canvas.fill_rect(0, 0, static_cast<int>(canvas.width()), 60,
                     GrayLevel::White);
    char city[24] = {};
    ellipsize(model.settings->city, 18U, city, sizeof(city));
    char gregorian[32] = {};
    std::snprintf(gregorian,
                  sizeof(gregorian),
                  "%s  %02u/%02u/%04u",
                  model.today->weekday,
                  static_cast<unsigned>(model.today->date.day),
                  static_cast<unsigned>(model.today->date.month),
                  static_cast<unsigned>(model.today->date.year));
    char first_row[72] = {};
    std::snprintf(first_row,
                  sizeof(first_row),
                  "%s    %s",
                  city,
                  gregorian);
    draw_centered(canvas,
                  kPageCenterX,
                  12,
                  first_row,
                  2,
                  GrayLevel::Black);

    char hijri[48] = {};
    std::snprintf(hijri,
                  sizeof(hijri),
                  "%u %s %u AH",
                  static_cast<unsigned>(model.today->hijri_day),
                  prayer_hijri_month_name(model.today->hijri_month),
                  static_cast<unsigned>(model.today->hijri_year));
    char rule[32] = {};
    std::snprintf(
        rule,
        sizeof(rule),
        "%s / %s",
        settings_method_short_name(model.settings->calculation_method),
        settings_asr_school_name(model.settings->asr_school));
    char second_row[96] = {};
    std::snprintf(second_row,
                  sizeof(second_row),
                  "%s    %s    %s",
                  rule,
                  hijri,
                  model.online ? "SYNCED" : "LOCAL");
    draw_centered(canvas,
                  kPageCenterX,
                  37,
                  second_row,
                  2,
                  GrayLevel::Black);
}

void draw_landscape_schedule(Canvas &canvas, const PrayerPageModel &model)
{
    draw_background_asset(canvas, islamic_landscape_background_asset());
    draw_landscape_header(canvas, model);

    const bool between = model.window.between_prayers;
    draw_centered(canvas,
                  200,
                  236,
                  between ? "BETWEEN PRAYER WINDOWS"
                          : "CURRENT PRAYER WINDOW",
                  2,
                  GrayLevel::Black);
    draw_centered(canvas,
                  200,
                  264,
                  between ? "Next: Dhuhr"
                          : prayer_id_name(model.window.prayer),
                  between ? 4 : 6,
                  GrayLevel::Black);
    char start[6] = {};
    char end[6] = {};
    format_time(model.window.start_minutes, start);
    format_time(model.window.end_minutes, end);
    char range[20] = {};
    std::snprintf(range, sizeof(range), "%s - %s", start, end);
    draw_centered(canvas, 200, 322, range, 4, GrayLevel::Black);
    if (!between) {
        draw_arabic_asset(canvas,
                          200,
                          362,
                          model.window.prayer,
                          GrayLevel::Black);
    }
    char remaining[28] = {};
    format_remaining(model.window.minutes_remaining,
                     remaining,
                     sizeof(remaining));
    draw_centered(canvas, 200, 398, remaining, 2, GrayLevel::Black);
    if (model.window.qiyam_active) {
        char qiyam_time[6] = {};
        char qiyam[28] = {};
        format_time(model.window.qiyam_start_minutes, qiyam_time);
        std::snprintf(qiyam,
                      sizeof(qiyam),
                      "QIYAM SINCE %s",
                      qiyam_time);
        draw_centered(canvas, 200, 420, qiyam, 2, GrayLevel::Black);
    }
    char next_time[6] = {};
    format_time(model.next.time_minutes, next_time);
    char next_line[36] = {};
    std::snprintf(next_line,
                  sizeof(next_line),
                  "NEXT %s %s",
                  prayer_id_name(model.next.prayer),
                  next_time);
    draw_centered(canvas, 220, 444, next_line, 2, GrayLevel::Black);

    constexpr int kTableCenterX = 565;
    constexpr int kEnglishX = 440;
    constexpr int kArabicCenterX = 575;
    constexpr int kTimeX = 630;
    constexpr int kFirstCenterY = 250;
    constexpr int kRowStep = 34;
    draw_centered(canvas,
                  kTableCenterX,
                  214,
                  "TODAY'S TIMES",
                  2,
                  GrayLevel::Black);
    for (size_t index = 0U; index < 7U; ++index) {
        const TimelineEntry &entry = kTimeline[index];
        const uint16_t prayer_minutes = model.today->minutes[
            static_cast<size_t>(entry.slot)];
        const bool selected =
            (entry.prayer != PrayerId::None &&
             entry.prayer == model.window.prayer) ||
            (entry.slot == PrayerTimeSlot::Qiyam &&
             model.window.qiyam_active);
        const int center_y = kFirstCenterY +
                             static_cast<int>(index) * kRowStep;
        const TextWeight weight = selected ? TextWeight::Bold
                                           : TextWeight::Regular;
        canvas.draw_text(kEnglishX,
                         center_y - 7,
                         entry.name,
                         2,
                         GrayLevel::Black,
                         weight);
        char time[6] = {};
        format_time(prayer_minutes, time);
        canvas.draw_text(kTimeX,
                         center_y - 7,
                         time,
                         2,
                         GrayLevel::Black,
                         weight);
        draw_arabic_asset(canvas,
                          kArabicCenterX,
                          center_y - 16,
                          entry.prayer,
                          GrayLevel::Black);
    }
}

void draw_portrait_schedule(Canvas &canvas, const PrayerPageModel &model)
{
    draw_background_asset(canvas, islamic_prayer_background_asset());
    draw_portrait_header(canvas, model);

    const bool between = model.window.between_prayers;
    draw_centered(canvas,
                  240,
                  190,
                  between ? "BETWEEN PRAYER WINDOWS"
                          : "CURRENT PRAYER WINDOW",
                  2,
                  GrayLevel::Black);
    draw_centered(canvas,
                  240,
                  218,
                  between ? "Next: Dhuhr"
                          : prayer_id_name(model.window.prayer),
                  between ? 4 : 6,
                  GrayLevel::Black);
    char start[6] = {};
    char end[6] = {};
    format_time(model.window.start_minutes, start);
    format_time(model.window.end_minutes, end);
    char range[20] = {};
    std::snprintf(range, sizeof(range), "%s - %s", start, end);
    draw_centered(canvas, 240, 272, range, 5, GrayLevel::Black);
    if (!between) {
        draw_arabic_asset(canvas,
                          240,
                          318,
                          model.window.prayer,
                          GrayLevel::Black);
    }
    char remaining[28] = {};
    format_remaining(model.window.minutes_remaining,
                     remaining,
                     sizeof(remaining));
    draw_centered(canvas, 240, 354, remaining, 2, GrayLevel::Black);
    if (model.window.qiyam_active) {
        char qiyam_time[6] = {};
        char qiyam[28] = {};
        format_time(model.window.qiyam_start_minutes, qiyam_time);
        std::snprintf(qiyam,
                      sizeof(qiyam),
                      "QIYAM SINCE %s",
                      qiyam_time);
        draw_centered(canvas, 240, 378, qiyam, 2, GrayLevel::Black);
    }

    canvas.draw_line(96, 404, 216, 404, GrayLevel::Black);
    canvas.draw_circle(240, 404, 4, GrayLevel::Black);
    canvas.draw_line(264, 404, 384, 404, GrayLevel::Black);
    canvas.draw_text(24, 420, "TODAY'S TIMES", 2, GrayLevel::Black);

    constexpr int kTimelineX = 44;
    constexpr int kFirstCenterY = 456;
    constexpr int kRowStep = 29;
    canvas.draw_line(kTimelineX,
                     kFirstCenterY,
                     kTimelineX,
                     kFirstCenterY + 6 * kRowStep,
                     GrayLevel::Black);
    for (size_t index = 0U; index < 7U; ++index) {
        const TimelineEntry &entry = kTimeline[index];
        const uint16_t prayer_minutes = model.today->minutes[
            static_cast<size_t>(entry.slot)];
        const bool selected =
            (entry.prayer != PrayerId::None &&
             entry.prayer == model.window.prayer) ||
            (entry.slot == PrayerTimeSlot::Qiyam &&
             model.window.qiyam_active);
        const int center_y = kFirstCenterY + static_cast<int>(index) * kRowStep;
        draw_timeline_marker(canvas, kTimelineX, center_y, selected);
        if (selected) {
            canvas.draw_line(64, center_y + 14, 454, center_y + 14,
                             GrayLevel::Black);
        }
        canvas.draw_text(68,
                         center_y - 10,
                         entry.name,
                         2,
                         GrayLevel::Black);
        char time[6] = {};
        format_time(prayer_minutes, time);
        canvas.draw_text(390, center_y - 10, time, 2, GrayLevel::Black);
        draw_arabic_asset(canvas,
                          278,
                          center_y - 16,
                          entry.prayer,
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
                      352,
                      "1  CONNECT YOUR PHONE TO",
                      2,
                      GrayLevel::Black);
        draw_centered(canvas,
                      static_cast<int>(canvas.width()) / 2,
                      386,
                      ap_ssid,
                      2U,
                      GrayLevel::Black);
        canvas.draw_line(100, 426, 380, 426, GrayLevel::Black);

        draw_centered(canvas,
                      static_cast<int>(canvas.width()) / 2,
                      446,
                      "2  ENTER THE PASSWORD",
                      2,
                      GrayLevel::Black);
        draw_centered(canvas,
                      static_cast<int>(canvas.width()) / 2,
                      480,
                      ap_password,
                      3,
                      GrayLevel::Black);
        canvas.draw_line(100, 520, 380, 520, GrayLevel::Black);

        draw_centered(canvas,
                      static_cast<int>(canvas.width()) / 2,
                      540,
                      "3  OPEN THE SETUP PAGE",
                      2,
                      GrayLevel::Black);
        draw_centered(canvas,
                      static_cast<int>(canvas.width()) / 2,
                      580,
                      "192.168.4.1",
                      3,
                      GrayLevel::Black);
        draw_centered(canvas,
                      static_cast<int>(canvas.width()) / 2,
                      620,
                      "PAGE OPENS AUTOMATICALLY",
                      2,
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
                      2,
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
