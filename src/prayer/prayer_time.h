#pragma once

#include <cstddef>
#include <cstdint>

struct PrayerDate {
    uint16_t year = 0U;
    uint8_t month = 0U;
    uint8_t day = 0U;
};

enum class PrayerId : uint8_t {
    Fajr,
    Dhuhr,
    Asr,
    Maghrib,
    Isha,
    None,
};

enum class PrayerTimeSlot : uint8_t {
    Fajr,
    Sunrise,
    Dhuhr,
    Asr,
    Maghrib,
    Isha,
    Qiyam,
    Count,
};

constexpr size_t kPrayerTimeSlotCount =
    static_cast<size_t>(PrayerTimeSlot::Count);

struct PrayerDay {
    PrayerDate date = {};
    uint16_t minutes[kPrayerTimeSlotCount] = {};
    uint16_t hijri_year = 0U;
    uint8_t hijri_month = 0U;
    uint8_t hijri_day = 0U;
    char hijri_month_name[24] = {};
    char weekday[12] = {};
};

struct PrayerNext {
    PrayerId prayer = PrayerId::None;
    uint16_t time_minutes = 0U;
    uint16_t minutes_until = 0U;
    bool tomorrow = false;
};

struct PrayerWindow {
    PrayerId prayer = PrayerId::None;
    uint16_t start_minutes = 0U;
    uint16_t end_minutes = 0U;
    uint16_t minutes_remaining = 0U;
    bool start_yesterday = false;
    bool end_tomorrow = false;
    bool between_prayers = false;
    bool qiyam_active = false;
    uint16_t qiyam_start_minutes = 0U;
};

// Parses the leading HH:MM portion of a civil-time string.
// 解析民用时间字符串开头的HH:MM部分。
bool prayer_parse_hhmm(const char *text, uint16_t &minutes);

// Validates one complete date and ordered set of prayer times.
// 校验一整天的日期及按顺序排列的礼拜时间。
bool prayer_day_valid(const PrayerDay &day);

// Returns the next required prayer, rolling to tomorrow's Fajr after Isha.
// 返回下一次必行礼拜；Isha之后自动切换到次日Fajr。
bool prayer_find_next(const PrayerDay &today,
                      const PrayerDay *tomorrow,
                      uint16_t current_minutes,
                      PrayerNext &next);

// Finds the active obligatory-prayer window and the overlapping Qiyam period.
// 查找当前主命礼拜窗口，以及可能重叠的后夜礼拜时段。
bool prayer_find_window(const PrayerDay *yesterday,
                        const PrayerDay &today,
                        const PrayerDay *tomorrow,
                        uint16_t current_minutes,
                        PrayerWindow &window);

// Selects a battery-conscious display refresh interval for the countdown.
// 根据倒计时选择兼顾续航的屏幕刷新间隔。
uint32_t prayer_refresh_interval_seconds(uint16_t minutes_until);

const char *prayer_id_name(PrayerId prayer);
// Returns an ASCII-only Hijri month label suitable for the built-in font.
// 返回适合内置点阵字体显示的纯 ASCII 回历月份名称。
const char *prayer_hijri_month_name(uint8_t month);
PrayerTimeSlot prayer_id_slot(PrayerId prayer);

bool prayer_date_valid(const PrayerDate &date);
bool prayer_date_equal(const PrayerDate &left, const PrayerDate &right);
PrayerDate prayer_date_next(const PrayerDate &date);
PrayerDate prayer_date_previous(const PrayerDate &date);
void prayer_next_month(uint16_t year,
                       uint8_t month,
                       uint16_t &next_year,
                       uint8_t &next_month);
