#include <cassert>
#include <cstring>

#include "prayer_time.h"

namespace {

PrayerDay make_day(uint16_t year, uint8_t month, uint8_t day)
{
    PrayerDay value = {};
    value.date = {year, month, day};
    value.minutes[static_cast<size_t>(PrayerTimeSlot::Fajr)] = 285U;
    value.minutes[static_cast<size_t>(PrayerTimeSlot::Sunrise)] = 365U;
    value.minutes[static_cast<size_t>(PrayerTimeSlot::Dhuhr)] = 720U;
    value.minutes[static_cast<size_t>(PrayerTimeSlot::Asr)] = 930U;
    value.minutes[static_cast<size_t>(PrayerTimeSlot::Maghrib)] = 1080U;
    value.minutes[static_cast<size_t>(PrayerTimeSlot::Isha)] = 1150U;
    value.hijri_year = 1448U;
    value.hijri_month = 2U;
    value.hijri_day = day;
    std::strcpy(value.hijri_month_name, "Safar");
    std::strcpy(value.weekday, "Friday");
    return value;
}

}  // namespace

int main()
{
    uint16_t minutes = 0U;
    assert(prayer_parse_hhmm("04:43 (WIB)", minutes));
    assert(minutes == 283U);
    assert(!prayer_parse_hhmm("24:00", minutes));
    assert(!prayer_parse_hhmm("4:43", minutes));

    PrayerDay today = make_day(2026U, 8U, 28U);
    PrayerDay tomorrow = make_day(2026U, 8U, 29U);
    assert(prayer_day_valid(today));

    PrayerNext next = {};
    assert(prayer_find_next(today, &tomorrow, 280U, next));
    assert(next.prayer == PrayerId::Fajr);
    assert(next.minutes_until == 5U);
    assert(!next.tomorrow);

    assert(prayer_find_next(today, &tomorrow, 285U, next));
    assert(next.prayer == PrayerId::Fajr);
    assert(next.minutes_until == 0U);

    assert(prayer_find_next(today, &tomorrow, 286U, next));
    assert(next.prayer == PrayerId::Dhuhr);
    assert(next.minutes_until == 434U);

    assert(prayer_find_next(today, &tomorrow, 1200U, next));
    assert(next.prayer == PrayerId::Fajr);
    assert(next.tomorrow);
    assert(next.minutes_until == 525U);
    assert(!prayer_find_next(today, nullptr, 1200U, next));

    assert(prayer_refresh_interval_seconds(61U) == 600U);
    assert(prayer_refresh_interval_seconds(60U) == 300U);
    assert(prayer_refresh_interval_seconds(11U) == 300U);
    assert(prayer_refresh_interval_seconds(10U) == 60U);

    assert(std::strcmp(prayer_hijri_month_name(3U), "Rabi al-Awwal") == 0);
    assert(std::strcmp(prayer_hijri_month_name(6U), "Jumada al-Akhirah") == 0);
    assert(std::strcmp(prayer_hijri_month_name(11U), "Dhu al-Qadah") == 0);
    assert(std::strcmp(prayer_hijri_month_name(0U), "Hijri") == 0);

    const PrayerDate leap_day = prayer_date_next({2028U, 2U, 28U});
    assert(prayer_date_equal(leap_day, {2028U, 2U, 29U}));
    const PrayerDate new_year = prayer_date_next({2026U, 12U, 31U});
    assert(prayer_date_equal(new_year, {2027U, 1U, 1U}));

    uint16_t next_year = 0U;
    uint8_t next_month = 0U;
    prayer_next_month(2026U, 12U, next_year, next_month);
    assert(next_year == 2027U && next_month == 1U);
    return 0;
}
