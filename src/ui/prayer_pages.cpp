#include "prayer_pages.h"

#include <cstdio>
#include <cstring>

#include "arabic_prayer_assets.h"
#include "ui_style.h"

namespace {

constexpr const char *kNames[] = {"FAJR", "Sunrise", "DHUHR", "ASR", "MAGHRIB", "ISHA", "Qiyam"};
constexpr const char *kMonths[] = {
    "", "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December",
};

void arabic(Canvas &canvas, int center, int y, ArabicPrayerAsset asset)
{
    if (asset.data != nullptr)
        ui::bitmap(canvas, center - asset.width / 2, y,
                   {asset.width, asset.height, asset.stride, asset.data});
}

void background(Canvas &canvas, const IslamicBackgroundAsset &asset)
{
    canvas.clear();
    ui::bitmap(canvas, 0, 0, asset);
}

void time_parts(uint16_t minutes, char time[6], const char *&period)
{
    const unsigned hour = (minutes / 60U) % 24U;
    std::snprintf(time, 6, "%02u:%02u", hour % 12U == 0U ? 12U : hour % 12U,
                  static_cast<unsigned>(minutes % 60U));
    period = hour < 12U ? "AM" : "PM";
}

void date_header(Canvas &canvas, const PrayerPageModel &model, bool portrait)
{
    const int center = canvas.width() / 2;
    const int city_center = center;
    const int city_width = portrait ? 132 : 300;
    char location[100] = {};
    const int status_y = 26;
    const auto pin = inksalah_location_asset();
    ui::fit(model.settings->city, location, sizeof(location), city_width - pin.width - 6,
              20, TextWeight::Bold);
    const int location_width = Canvas::text_width(location, 20, TextWeight::Bold);
    const int location_left = city_center - (location_width + pin.width + 6) / 2;
    ui::bitmap(canvas, location_left, status_y, pin, GrayLevel::White);
    ui::centered(canvas, location_left + pin.width + 6 + location_width / 2,
                  status_y, location, 20, TextWeight::Bold, GrayLevel::White);
    char method[48] = {};
    std::snprintf(method, sizeof(method), "Method: %s",
                  settings_method_short_name(model.settings->calculation_method));
    const int method_right = canvas.width() - 24;
    const auto gear = inksalah_status_gear_asset();
    const int method_width = portrait ? 146 : 210;
    const int text_width = method_width - gear.width - 5;
    if (Canvas::text_width(method, 20, TextWeight::Bold) > text_width)
        std::snprintf(method, sizeof(method), "%s",
                      settings_method_short_name(model.settings->calculation_method));
    char fitted_method[48] = {};
    ui::fit(method, fitted_method, sizeof(fitted_method), text_width, 20, TextWeight::Bold);
    const int label_width = Canvas::text_width(fitted_method, 20, TextWeight::Bold);
    const int method_left = method_right - label_width - 5 - gear.width;
    ui::bitmap(canvas, method_left, status_y, gear, GrayLevel::White);
    ui::centered(canvas, method_right - label_width / 2, status_y, fitted_method,
                  20, TextWeight::Bold, GrayLevel::White);
    char date[80] = {};
    const unsigned month = model.today->date.month;
    std::snprintf(date, sizeof(date), "%02u %s, %04u (%.3s.)",
                  static_cast<unsigned>(model.today->date.day),
                  month <= 12U ? kMonths[month] : "",
                  static_cast<unsigned>(model.today->date.year),
                  model.today->weekday);
    ui::fitted_centered(canvas, center, portrait ? 115 : 72, date,
                         portrait ? 432 : 290, portrait ? 26 : 22,
                         TextWeight::Bold, GrayLevel::White);
    char hijri[80] = {};
    std::snprintf(hijri, sizeof(hijri), "%u %s, %u",
                  static_cast<unsigned>(model.today->hijri_day),
                  model.today->hijri_month == 3U ? "Rabi ul Awal" :
                    prayer_hijri_month_name(model.today->hijri_month),
                  static_cast<unsigned>(model.today->hijri_year));
    ui::fitted_centered(canvas, center, portrait ? 156 : 103, hijri,
                         portrait ? 420 : 360, portrait ? 22 : 20,
                         TextWeight::Regular, GrayLevel::White);
}

void time_badge(Canvas &canvas, int x, int y, uint16_t minutes, int width, int height)
{
    char time[6] = {};
    const char *period = nullptr;
    time_parts(minutes, time, period);
    canvas.fill_rect(x, y, width, height, GrayLevel::Black);
    const int content = Canvas::text_width(time, 28, TextWeight::Bold) + 8 +
                        Canvas::text_width(period, 20);
    const int start = x + (width - content) / 2;
    canvas.draw_text_sized(start, y + (height - 28) / 2, time, 28,
                           GrayLevel::White, TextWeight::Bold);
    canvas.draw_text_sized(start + Canvas::text_width(time, 28, TextWeight::Bold) + 8,
                           y + (height - 20) / 2 + 2, period, 20, GrayLevel::White);
}

// Centers separately measured clock values and AM/PM labels in one capsule.
// 将分别测量的钟点数字和 AM/PM 标签作为整体居中排入胶囊。
void time_range(Canvas &canvas, int center, int y, uint16_t start, uint16_t end,
                bool portrait)
{
    char first[6] = {}, last[6] = {};
    const char *first_period = nullptr, *last_period = nullptr;
    time_parts(start, first, first_period);
    time_parts(end, last, last_period);
    const char *parts[] = {first, first_period, "-", last, last_period};
    const uint8_t size = portrait ? 28 : 32;
    const uint8_t sizes[] = {size, 20, 20, size, 20};
    const int width = portrait ? 268 : 288;
    const int height = portrait ? 50 : 54;
    int content_width = 0;
    for (int index = 0; index < 5; ++index)
        content_width += Canvas::text_width(parts[index], sizes[index], TextWeight::Bold);
    content_width += 4 * 5;
    canvas.fill_round_rect(center - width / 2, y, width, height, height / 2, GrayLevel::Black);
    int x = center - content_width / 2;
    for (int index = 0; index < 5; ++index) {
        canvas.draw_text_sized(x, y + (height - sizes[index]) / 2, parts[index], sizes[index],
                               GrayLevel::White, TextWeight::Bold);
        x += Canvas::text_width(parts[index], sizes[index], TextWeight::Bold) + 5;
    }
}

void hero(Canvas &canvas, const PrayerPageModel &model, bool portrait)
{
    const int center = portrait ? 240 : 208;
    const int caption_y = portrait ? 268 : 202;
    const bool between = model.window.between_prayers;
    ui::centered(canvas, center, caption_y,
                  between ? "Between Prayers" : "Current Prayer", portrait ? 20 : 26,
                  TextWeight::Regular, GrayLevel::DarkGray);
    const PrayerId prayer = between ? model.next.prayer : model.window.prayer;
    const auto slot = prayer_id_slot(prayer);
    const char *name = slot == PrayerTimeSlot::Count ? "" : kNames[static_cast<size_t>(slot)];
    const ArabicPrayerAsset label = arabic_prayer_hero_asset(prayer, portrait);
    const uint8_t name_size = portrait ? 26 : 40;
    const int name_width = Canvas::text_width(name, name_size, TextWeight::Bold);
    const int group_width = name_width + 12 + label.width;
    const int name_y = portrait ? 296 : 242;
    canvas.draw_text_sized(center - group_width / 2, name_y, name, name_size,
                           GrayLevel::Black, TextWeight::Bold);
    arabic(canvas, center - group_width / 2 + name_width + 12 + label.width / 2,
           name_y + (name_size - label.height) / 2, label);
    const int range_y = portrait ? 330 : 304;
    const int width = portrait ? 268 : 288;
    if (between) {
        char next_time[16] = {}, line[32] = {};
        prayer_page_format_time(model.next.time_minutes, next_time, sizeof(next_time));
        std::snprintf(line, sizeof(line), "Starts at %s", next_time);
        ui::pill(canvas, center - width / 2, range_y, width, portrait ? 50 : 54, line, true, 24);
    } else {
        time_range(canvas, center, range_y,
                    model.window.start_minutes, model.window.end_minutes, portrait);
    }
    char remaining[28] = {};
    const uint16_t minutes = between ? model.next.minutes_until : model.window.minutes_remaining;
    std::snprintf(remaining, sizeof(remaining), "%uh %02um",
                  static_cast<unsigned>(minutes / 60U),
                  static_cast<unsigned>(minutes % 60U));
    const int countdown_y = portrait ? 388 : 366;
    const int height = portrait ? 50 : 52;
    const int remaining_size = portrait ? 28 : 32;
    const auto clock = inksalah_clock_asset();
    const int label_width = Canvas::text_width("Next in", 20, TextWeight::Bold);
    const int content_width = clock.width + 16 + label_width + 10 +
                              Canvas::text_width(remaining, remaining_size, TextWeight::Bold);
    const int left = center - content_width / 2;
    canvas.draw_round_rect(center - width / 2, countdown_y, width, height, height / 2);
    canvas.draw_round_rect(center - width / 2 + 1, countdown_y + 1,
                           width - 2, height - 2, height / 2 - 1);
    ui::bitmap(canvas, left, countdown_y + (height - clock.height) / 2, clock);
    canvas.draw_text_sized(left + clock.width + 16, countdown_y + (height - 20) / 2,
                           "Next in", 20, GrayLevel::Black, TextWeight::Bold);
    canvas.draw_text_sized(left + clock.width + 16 + label_width + 10,
                           countdown_y + (height - remaining_size) / 2, remaining,
                           remaining_size, GrayLevel::Black, TextWeight::Bold);
}

void times(Canvas &canvas, const PrayerPageModel &model, bool portrait)
{
    const int count = model.settings->qiyam_visible ? 7 : 6;
    const int step = portrait ? (count == 7 ? 48 : 54) : (count == 7 ? 45 : 52);
    const int first_y = portrait ? (count == 7 ? 454 : 462) : 154;
    const int name_right = portrait ? 222 : 570;
    const int time_x = portrait ? 246 : 592;
    for (int index = 0; index < count; ++index) {
        const int y = first_y + index * step;
        const auto slot = static_cast<PrayerTimeSlot>(index);
        const uint8_t size = !portrait && count == 7 ? 22 : 24;
        const int x = name_right - Canvas::text_width(kNames[index], size);
        canvas.draw_text_sized(x, y, kNames[index], size);
        const ArabicPrayerAsset label = arabic_time_asset(slot);
        arabic(canvas, name_right - label.width / 2, y + size - 1, label);
        time_badge(canvas, time_x, y, model.today->minutes[index],
                    116, 43);
    }
}

}  // namespace

void prayer_page_format_time(uint16_t minutes, char *output, size_t capacity)
{
    char time[6] = {};
    const char *period = nullptr;
    time_parts(minutes, time, period);
    std::snprintf(output, capacity, "%s %s", time, period);
}

bool prayer_page_settings_action_at(int x, int y, int width, int height)
{
    const int left = width < height ? 304 : 560;
    return x >= left && x < width - 16 && y >= 16 && y < 62;
}

void prayer_page_render_schedule(Canvas &canvas, const PrayerPageModel &model)
{
    if (model.settings == nullptr || model.today == nullptr || model.next.prayer == PrayerId::None) {
        prayer_page_render_error(canvas, "Schedule unavailable", "Check location and prayer settings");
        return;
    }
    const bool portrait = canvas.width() < canvas.height();
    background(canvas, portrait ? islamic_prayer_background_asset() : islamic_landscape_background_asset());
    canvas.set_prayer_header(true);
    date_header(canvas, model, portrait);
    hero(canvas, model, portrait);
    times(canvas, model, portrait);
}

void prayer_page_render_setup(Canvas &canvas, const char *ap_ssid,
                              const char *ap_password, bool can_return)
{
    const bool portrait = canvas.width() < canvas.height();
    if (portrait) background(canvas, islamic_setup_background_asset());
    else {
        canvas.clear(GrayLevel::Black);
        canvas.fill_round_rect(36, 34, 728, 408, 80, GrayLevel::White);
    }
    const int center = canvas.width() / 2;
    const int brand_y = portrait ? 116 : 61;
    const int brand_width = Canvas::text_width("INKSALAH", 36, TextWeight::Bold);
    ui::bitmap(canvas, portrait ? 97 : center - (brand_width + 46) / 2,
                portrait ? 111 : brand_y, inksalah_book_asset(),
                portrait ? GrayLevel::White : GrayLevel::Black);
    canvas.draw_text_sized(portrait ? 149 : center - (brand_width + 46) / 2 + 46, brand_y,
                           "INKSALAH", 36, GrayLevel::Black, TextWeight::Bold);
    ui::centered(canvas, center, brand_y + 48, "Powered by reTerminal Sticky", 20);
    ui::pill(canvas, center - 174, portrait ? 222 : 145, 348, 50,
              can_return ? "Wi-Fi Setup" : "First-Time Wi-Fi Setup", true, 24);
    const char *steps[] = {"01 Connect Your Phone to", "02 Enter The Passwords", "03 Open The Setup Page"};
    const char *values[] = {ap_ssid, ap_password, "192.168.4.1"};
    for (int index = 0; index < 3; ++index) {
        const int left = portrait ? 84 : 80 + index * 225;
        const int y = portrait ? 316 + index * 110 : 223;
        canvas.draw_text_sized(left, y, steps[index], 20, GrayLevel::DarkGray);
        if (portrait && index == 2) {
            canvas.draw_text_sized(left, y + 30, "(Just in case it doesn't opt in)", 20);
        }
        char fitted[80] = {};
        ui::fit(values[index], fitted, sizeof(fitted), portrait ? 330 : 208,
                 portrait ? 24 : 22, TextWeight::Bold);
        canvas.draw_text_sized(left, y + (portrait && index == 2 ? 61 : 39),
                               fitted, portrait ? 24 : 22, GrayLevel::Black, TextWeight::Bold);
        canvas.draw_line(left, y + (portrait && index == 2 ? 98 : 78),
                          left + (portrait ? 328 : 198), y + (portrait && index == 2 ? 98 : 78));
    }
    if (can_return && portrait) ui::pill(canvas, 164, 660, 152, 44, "Back", false, 22);
}

bool prayer_page_setup_back_action_at(int x, int y, bool can_return)
{
    return can_return && x >= 164 && x < 316 && y >= 660 && y < 704;
}

void prayer_page_render_error(Canvas &canvas, const char *title, const char *message)
{
    canvas.clear();
    const int center = canvas.width() / 2;
    ui::fitted_centered(canvas, center, canvas.height() / 3, title, canvas.width() - 64,
                         28, TextWeight::Bold);
    ui::fitted_centered(canvas, center, canvas.height() / 3 + 64, message, canvas.width() - 64, 20);
    ui::centered(canvas, center, canvas.height() - 120, "Swipe up to open settings", 20);
}
