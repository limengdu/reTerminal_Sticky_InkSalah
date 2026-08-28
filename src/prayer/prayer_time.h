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

// Parses the leading HH:MM portion used by AlAdhan timing strings.
// 解析AlAdhan时间字符串开头的HH:MM部分。
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

// Selects a battery-conscious display refresh interval for the countdown.
// 根据倒计时选择兼顾续航的屏幕刷新间隔。
uint32_t prayer_refresh_interval_seconds(uint16_t minutes_until);

const char *prayer_id_name(PrayerId prayer);
PrayerTimeSlot prayer_id_slot(PrayerId prayer);

bool prayer_date_valid(const PrayerDate &date);
bool prayer_date_equal(const PrayerDate &left, const PrayerDate &right);
PrayerDate prayer_date_next(const PrayerDate &date);
void prayer_next_month(uint16_t year,
                       uint8_t month,
                       uint16_t &next_year,
                       uint8_t &next_month);
