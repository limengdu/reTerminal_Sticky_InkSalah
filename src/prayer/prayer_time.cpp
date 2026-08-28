#include "prayer_time.h"

#include <cstring>

namespace {

uint8_t days_in_month(uint16_t year, uint8_t month)
{
    constexpr uint8_t kDays[] = {
        31U, 28U, 31U, 30U, 31U, 30U,
        31U, 31U, 30U, 31U, 30U, 31U,
    };
    if (month < 1U || month > 12U) {
        return 0U;
    }
    const bool leap = (year % 4U == 0U && year % 100U != 0U) ||
                      year % 400U == 0U;
    return month == 2U && leap ? 29U : kDays[month - 1U];
}

bool two_digits(const char *text, uint8_t &value)
{
    if (text == nullptr || text[0] < '0' || text[0] > '9' ||
        text[1] < '0' || text[1] > '9') {
        return false;
    }
    value = static_cast<uint8_t>((text[0] - '0') * 10 + text[1] - '0');
    return true;
}

}  // namespace

bool prayer_parse_hhmm(const char *text, uint16_t &minutes)
{
    if (text == nullptr || std::strlen(text) < 5U || text[2] != ':') {
        return false;
    }
    uint8_t hour = 0U;
    uint8_t minute = 0U;
    if (!two_digits(text, hour) || !two_digits(text + 3, minute) ||
        hour > 23U || minute > 59U) {
        return false;
    }
    minutes = static_cast<uint16_t>(hour * 60U + minute);
    return true;
}

bool prayer_date_valid(const PrayerDate &date)
{
    return date.year >= 2000U && date.year <= 2099U &&
           date.month >= 1U && date.month <= 12U &&
           date.day >= 1U && date.day <= days_in_month(date.year, date.month);
}

bool prayer_day_valid(const PrayerDay &day)
{
    if (!prayer_date_valid(day.date)) {
        return false;
    }
    for (size_t index = 0U; index < kPrayerTimeSlotCount; ++index) {
        if (day.minutes[index] >= 24U * 60U) {
            return false;
        }
        if (index > 0U && day.minutes[index] <= day.minutes[index - 1U]) {
            return false;
        }
    }
    return day.hijri_year >= 1400U && day.hijri_month >= 1U &&
           day.hijri_month <= 12U && day.hijri_day >= 1U &&
           day.hijri_day <= 30U;
}

PrayerTimeSlot prayer_id_slot(PrayerId prayer)
{
    switch (prayer) {
    case PrayerId::Fajr:
        return PrayerTimeSlot::Fajr;
    case PrayerId::Dhuhr:
        return PrayerTimeSlot::Dhuhr;
    case PrayerId::Asr:
        return PrayerTimeSlot::Asr;
    case PrayerId::Maghrib:
        return PrayerTimeSlot::Maghrib;
    case PrayerId::Isha:
        return PrayerTimeSlot::Isha;
    case PrayerId::None:
    default:
        return PrayerTimeSlot::Count;
    }
}

bool prayer_find_next(const PrayerDay &today,
                      const PrayerDay *tomorrow,
                      uint16_t current_minutes,
                      PrayerNext &next)
{
    if (!prayer_day_valid(today) || current_minutes >= 24U * 60U) {
        return false;
    }
    constexpr PrayerId kPrayers[] = {
        PrayerId::Fajr,
        PrayerId::Dhuhr,
        PrayerId::Asr,
        PrayerId::Maghrib,
        PrayerId::Isha,
    };
    for (PrayerId prayer : kPrayers) {
        const size_t slot = static_cast<size_t>(prayer_id_slot(prayer));
        const uint16_t prayer_minutes = today.minutes[slot];
        if (current_minutes <= prayer_minutes) {
            next.prayer = prayer;
            next.time_minutes = prayer_minutes;
            next.minutes_until = prayer_minutes - current_minutes;
            next.tomorrow = false;
            return true;
        }
    }

    if (tomorrow == nullptr || !prayer_day_valid(*tomorrow) ||
        !prayer_date_equal(tomorrow->date, prayer_date_next(today.date))) {
        return false;
    }
    const uint16_t fajr = tomorrow->minutes[
        static_cast<size_t>(PrayerTimeSlot::Fajr)];
    next.prayer = PrayerId::Fajr;
    next.time_minutes = fajr;
    next.minutes_until = static_cast<uint16_t>(24U * 60U - current_minutes + fajr);
    next.tomorrow = true;
    return true;
}

uint32_t prayer_refresh_interval_seconds(uint16_t minutes_until)
{
    if (minutes_until > 60U) {
        return 10U * 60U;
    }
    if (minutes_until > 10U) {
        return 5U * 60U;
    }
    return 60U;
}

const char *prayer_id_name(PrayerId prayer)
{
    switch (prayer) {
    case PrayerId::Fajr:
        return "Fajr";
    case PrayerId::Dhuhr:
        return "Dhuhr";
    case PrayerId::Asr:
        return "Asr";
    case PrayerId::Maghrib:
        return "Maghrib";
    case PrayerId::Isha:
        return "Isha";
    case PrayerId::None:
    default:
        return "None";
    }
}

bool prayer_date_equal(const PrayerDate &left, const PrayerDate &right)
{
    return left.year == right.year && left.month == right.month &&
           left.day == right.day;
}

PrayerDate prayer_date_next(const PrayerDate &date)
{
    PrayerDate next = date;
    if (!prayer_date_valid(date)) {
        return {};
    }
    if (next.day < days_in_month(next.year, next.month)) {
        ++next.day;
        return next;
    }
    next.day = 1U;
    if (next.month < 12U) {
        ++next.month;
    } else {
        next.month = 1U;
        ++next.year;
    }
    return next;
}

void prayer_next_month(uint16_t year,
                       uint8_t month,
                       uint16_t &next_year,
                       uint8_t &next_month)
{
    if (month < 12U) {
        next_year = year;
        next_month = static_cast<uint8_t>(month + 1U);
    } else {
        next_year = static_cast<uint16_t>(year + 1U);
        next_month = 1U;
    }
}
