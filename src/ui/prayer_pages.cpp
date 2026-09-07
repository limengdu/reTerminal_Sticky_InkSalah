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

void draw_clock(Canvas &canvas, int x, int y)
{
    canvas.fill_circle(x, y, 11);
    canvas.fill_circle(x - 10, y - 10, 4);
    canvas.fill_circle(x + 10, y - 10, 4);
    canvas.draw_line(x, y - 7, x, y, GrayLevel::White);
    canvas.draw_line(x, y, x + 5, y, GrayLevel::White);
    canvas.draw_line(x - 5, y + 9, x - 8, y + 14);
    canvas.draw_line(x + 5, y + 9, x + 8, y + 14);
}

void date_header(Canvas &canvas, const PrayerPageModel &model, bool portrait)
{
    const int center = canvas.width() / 2;
    const int city_center = portrait ? 347 : center;
    const int city_width = portrait ? 145 : 580;
    char location[100] = {};
    if (portrait) std::snprintf(location, sizeof(location), "%s", model.settings->city);
    else std::snprintf(location, sizeof(location), "%s  /  Method: %s",
                       model.settings->city,
                       settings_method_short_name(model.settings->calculation_method));
    ui::fitted_centered(canvas, city_center, portrait ? 35 : 8,
                         location, city_width, portrait ? 22 : 20,
                         TextWeight::Bold, GrayLevel::White);
    if (portrait) {
        char method[48] = {};
        std::snprintf(method, sizeof(method), "Method: %s",
                       settings_method_short_name(model.settings->calculation_method));
        ui::fitted_centered(canvas, 347, 66, method, 170, 20,
                             TextWeight::Regular, GrayLevel::White);
    }
    ui::bitmap(canvas, canvas.width() - 52, portrait ? 43 : 43,
                inksalah_gear_asset(), GrayLevel::White);
    char date[80] = {};
    const unsigned month = model.today->date.month;
    std::snprintf(date, sizeof(date), "%02u %s, %04u (%.3s.)",
                  static_cast<unsigned>(model.today->date.day),
                  month <= 12U ? kMonths[month] : "",
                  static_cast<unsigned>(model.today->date.year),
                  model.today->weekday);
    ui::fitted_centered(canvas, center, portrait ? 115 : 39, date,
                         portrait ? 432 : 650, portrait ? 26 : 24,
                         TextWeight::Bold, GrayLevel::White);
    char hijri[80] = {};
    std::snprintf(hijri, sizeof(hijri), "%u %s, %u",
                  static_cast<unsigned>(model.today->hijri_day),
                  prayer_hijri_month_name(model.today->hijri_month),
                  static_cast<unsigned>(model.today->hijri_year));
    ui::fitted_centered(canvas, center, portrait ? 156 : 74, hijri,
                         portrait ? 420 : 620, 22,
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

void time_range(Canvas &canvas, int center, int y, uint16_t start, uint16_t end)
{
    char first[12] = {}, last[12] = {}, range[32] = {};
    prayer_page_format_time(start, first, sizeof(first));
    prayer_page_format_time(end, last, sizeof(last));
    std::snprintf(range, sizeof(range), "%s - %s", first, last);
    ui::pill(canvas, center - 158, y, 316, 50, range, true, 26);
}

void hero(Canvas &canvas, const PrayerPageModel &model, bool portrait)
{
    const int center = portrait ? 240 : 203;
    const int caption_y = portrait ? 268 : 192;
    const bool between = model.window.between_prayers;
    ui::centered(canvas, center, caption_y,
                  between ? "Between Prayers" : "Current Prayer", 20,
                  TextWeight::Regular, GrayLevel::DarkGray);
    const PrayerId prayer = between ? model.next.prayer : model.window.prayer;
    const auto slot = prayer_id_slot(prayer);
    const char *name = slot == PrayerTimeSlot::Count ? "" : kNames[static_cast<size_t>(slot)];
    const ArabicPrayerAsset label = arabic_prayer_asset(prayer);
    const int name_width = Canvas::text_width(name, 28, TextWeight::Bold);
    const int group_width = name_width + 12 + label.width;
    const int name_y = caption_y + 29;
    canvas.draw_text_sized(center - group_width / 2, name_y, name, 28,
                           GrayLevel::Black, TextWeight::Bold);
    arabic(canvas, center - group_width / 2 + name_width + 12 + label.width / 2,
           name_y + 2, label);
    const int range_y = portrait ? 335 : 286;
    if (between) {
        char next_time[16] = {}, line[32] = {};
        prayer_page_format_time(model.next.time_minutes, next_time, sizeof(next_time));
        std::snprintf(line, sizeof(line), "Starts at %s", next_time);
        ui::pill(canvas, center - 158, range_y, 316, 50, line, true, 26);
    } else {
        time_range(canvas, center, range_y,
                    model.window.start_minutes, model.window.end_minutes);
    }
    char remaining[28] = {};
    const uint16_t minutes = between ? model.next.minutes_until : model.window.minutes_remaining;
    std::snprintf(remaining, sizeof(remaining), "%uh %02um",
                  static_cast<unsigned>(minutes / 60U),
                  static_cast<unsigned>(minutes % 60U));
    const int countdown_y = range_y + 59;
    canvas.draw_round_rect(center - 158, countdown_y, 316, 49, 24);
    draw_clock(canvas, center - 125, countdown_y + 24);
    canvas.draw_text_sized(center - 99, countdown_y + 15, "Next in", 20,
                           GrayLevel::Black, TextWeight::Bold);
    ui::centered(canvas, center + 55, countdown_y + 10, remaining, 28, TextWeight::Bold);
    if (!portrait && model.settings->qiyam_visible && model.window.qiyam_active)
        ui::centered(canvas, center, 410, "Qiyam time", 20);
    if (!portrait && !model.window.qiyam_active) {
        char next_time[16] = {}, line[48] = {};
        prayer_page_format_time(model.next.time_minutes, next_time, sizeof(next_time));
        std::snprintf(line, sizeof(line), "Next: %s  %s", prayer_id_name(model.next.prayer), next_time);
        ui::fitted_centered(canvas, center, 415, line, 310, 20);
    }
}

void times(Canvas &canvas, const PrayerPageModel &model, bool portrait)
{
    const int count = model.settings->qiyam_visible ? 7 : 6;
    const int step = portrait ? (count == 7 ? 48 : 54) : (count == 7 ? 46 : 53);
    const int first_y = portrait ? (count == 7 ? 456 : 468) : 127;
    const int name_right = portrait ? 226 : 530;
    const int time_x = portrait ? 249 : 618;
    for (int index = 0; index < count; ++index) {
        const int y = first_y + index * step;
        const auto slot = static_cast<PrayerTimeSlot>(index);
        const bool selected = (!model.window.between_prayers &&
                               slot == prayer_id_slot(model.window.prayer)) ||
                              (slot == PrayerTimeSlot::Qiyam && model.window.qiyam_active);
        const uint8_t size = portrait ? (count == 7 ? 24 : 26) : 22;
        const int text_y = portrait ? y : y + 11;
        const int x = portrait ? name_right - Canvas::text_width(kNames[index], size)
                               : 432;
        canvas.draw_text_sized(x, text_y, kNames[index], size);
        const ArabicPrayerAsset label = arabic_time_asset(slot);
        arabic(canvas, portrait ? name_right - label.width / 2 : 576,
               portrait ? y + size - 1 : y + 10, label);
        time_badge(canvas, time_x, y, model.today->minutes[index],
                    portrait ? 130 : 143, portrait ? 43 : 40);
        if (selected) canvas.fill_circle(portrait ? 392 : 765, y + 20, 3);
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
    (void)height;
    const int top = 34;
    return x >= width - 62 && x < width - 12 && y >= top && y < top + 52;
}

void prayer_page_render_schedule(Canvas &canvas, const PrayerPageModel &model)
{
    if (model.settings == nullptr || model.today == nullptr || model.next.prayer == PrayerId::None) {
        prayer_page_render_error(canvas, "Schedule unavailable", "Check location and prayer settings");
        return;
    }
    const bool portrait = canvas.width() < canvas.height();
    background(canvas, portrait ? islamic_prayer_background_asset() : islamic_landscape_background_asset());
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
