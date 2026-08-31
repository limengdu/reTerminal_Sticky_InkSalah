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

void draw_islamic_background(Canvas &canvas)
{
    const IslamicBackgroundAsset asset = islamic_prayer_background_asset();
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

void draw_header(Canvas &canvas, const PrayerPageModel &model)
{
    char date[32] = {};
    std::snprintf(date,
                  sizeof(date),
                  "%s  %02u/%02u/%04u",
                  model.today->weekday,
                  static_cast<unsigned>(model.today->date.day),
                  static_cast<unsigned>(model.today->date.month),
                  static_cast<unsigned>(model.today->date.year));
    canvas.draw_text(18, 16, model.settings->city, 2, GrayLevel::Black);
    draw_centered(canvas,
                  static_cast<int>(canvas.width()) / 2,
                  16,
                  date,
                  2,
                  GrayLevel::DarkGray);
    char hijri[48] = {};
    std::snprintf(hijri,
                  sizeof(hijri),
                  "%u %s %u AH",
                  static_cast<unsigned>(model.today->hijri_day),
                  prayer_hijri_month_name(model.today->hijri_month),
                  static_cast<unsigned>(model.today->hijri_year));
    draw_centered(canvas,
                  static_cast<int>(canvas.width()) / 2,
                  42,
                  hijri,
                  1,
                  GrayLevel::DarkGray);
    canvas.draw_text(static_cast<int>(canvas.width()) - 170,
                     42,
                     model.online ? "SYNCED" : "CACHED",
                     1,
                     GrayLevel::DarkGray);
}

void draw_next_card(Canvas &canvas,
                    const PrayerPageModel &model,
                    int x,
                    int y,
                    int width,
                    int height)
{
    canvas.fill_rect(x, y, width, height, GrayLevel::Black);
    draw_centered(canvas,
                  x + width / 2,
                  y + 18,
                  "NEXT PRAYER",
                  2,
                  GrayLevel::White);
    const char *name = prayer_id_name(model.next.prayer);
    const uint8_t name_scale = width >= 700 ? 7U : 5U;
    draw_centered(canvas,
                  x + width / 2,
                  y + 54,
                  name,
                  name_scale,
                  GrayLevel::White);
    char time[6] = {};
    format_time(model.next.time_minutes, time);
    draw_centered(canvas,
                  x + width / 2,
                  y + 54 + name_scale * 8,
                  time,
                  width >= 700 ? 6U : 5U,
                  GrayLevel::White);
    char countdown[24] = {};
    format_countdown(model.next.minutes_until, countdown, sizeof(countdown));
    draw_centered(canvas,
                  x + width / 2,
                  y + height - 38,
                  countdown,
                  3,
                  GrayLevel::White);
    draw_arabic_asset(canvas,
                      x + width / 2,
                      y + height - 70,
                      model.next.prayer,
                      GrayLevel::White);
}

void draw_landscape_schedule(Canvas &canvas, const PrayerPageModel &model)
{
    draw_header(canvas, model);
    draw_next_card(canvas, model, 18, 70, 764, 214);

    constexpr int kCardGap = 8;
    constexpr int kMargin = 18;
    const int card_width =
        (static_cast<int>(canvas.width()) - 2 * kMargin - 4 * kCardGap) / 5;
    const int card_y = 302;
    for (size_t index = 0U; index < 5U; ++index) {
        const PrayerId prayer = kPrayers[index];
        const uint16_t prayer_minutes = model.today->minutes[
            static_cast<size_t>(prayer_id_slot(prayer))];
        const bool selected = prayer == model.next.prayer && !model.next.tomorrow;
        const bool past = model.current_minutes > prayer_minutes;
        const int x = kMargin + static_cast<int>(index) * (card_width + kCardGap);
        const GrayLevel background = selected ? GrayLevel::Black
                                              : GrayLevel::White;
        const GrayLevel foreground = selected
                                         ? GrayLevel::White
                                         : (past ? GrayLevel::LightGray
                                                 : GrayLevel::Black);
        canvas.fill_rect(x, card_y, card_width, 156, background);
        canvas.draw_rect(x, card_y, card_width, 156,
                         past && !selected ? GrayLevel::LightGray
                                           : GrayLevel::Black);
        draw_centered(canvas,
                      x + card_width / 2,
                      card_y + 16,
                      prayer_id_name(prayer),
                      2,
                      foreground);
        char time[6] = {};
        format_time(prayer_minutes, time);
        draw_centered(canvas,
                      x + card_width / 2,
                      card_y + 64,
                      time,
                      3,
                      foreground);
        draw_arabic_asset(canvas,
                          x + card_width / 2,
                          card_y + 112,
                          prayer,
                          foreground);
    }
}

void draw_portrait_schedule(Canvas &canvas, const PrayerPageModel &model)
{
    draw_islamic_background(canvas);
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
        draw_centered(canvas,
                      static_cast<int>(canvas.width()) / 2,
                      74,
                      "STICKY SALAH",
                      4);
        draw_centered(canvas,
                      static_cast<int>(canvas.width()) / 2,
                      132,
                      "FIRST-TIME SETUP",
                      2,
                      GrayLevel::DarkGray);
        canvas.draw_rect(30,
                         196,
                         static_cast<int>(canvas.width()) - 60,
                         360);
        canvas.draw_text(50, 228, "1. CONNECT PHONE TO", 2);
        draw_centered(canvas,
                      static_cast<int>(canvas.width()) / 2,
                      280,
                      ap_ssid,
                      2);
        canvas.draw_text(50, 352, "2. PASSWORD", 2);
        draw_centered(canvas,
                      static_cast<int>(canvas.width()) / 2,
                      406,
                      ap_password,
                      3);
        draw_centered(canvas,
                      static_cast<int>(canvas.width()) / 2,
                      642,
                      "OPEN HTTP://192.168.4.1",
                      2,
                      GrayLevel::DarkGray);
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
