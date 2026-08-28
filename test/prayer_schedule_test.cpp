#include <cassert>
#include <cstring>

#include "prayer_schedule.h"

PrayerDay make_day(uint8_t day, uint16_t fajr)
{
    PrayerDay value = {};
    value.date = {2026U, 8U, day};
    value.minutes[0] = fajr;
    value.minutes[1] = 360U;
    value.minutes[2] = 720U;
    value.minutes[3] = 930U;
    value.minutes[4] = 1080U;
    value.minutes[5] = 1150U;
    value.hijri_year = 1448U;
    value.hijri_month = 2U;
    value.hijri_day = day;
    std::strcpy(value.hijri_month_name, "Safar");
    std::strcpy(value.weekday, "Friday");
    return value;
}

int main()
{
    PrayerSchedule schedule = {};
    assert(prayer_schedule_add(schedule, make_day(28U, 285U)));
    assert(prayer_schedule_add(schedule, make_day(29U, 286U)));
    assert(schedule.count == 2U);
    assert(prayer_schedule_contains(schedule, {2026U, 8U, 28U}));
    assert(!prayer_schedule_contains(schedule, {2026U, 8U, 30U}));

    assert(prayer_schedule_add(schedule, make_day(28U, 284U)));
    assert(schedule.count == 2U);
    const PrayerDay *updated = prayer_schedule_find(
        schedule, {2026U, 8U, 28U});
    assert(updated != nullptr && updated->minutes[0] == 284U);

    prayer_schedule_clear(schedule);
    assert(schedule.count == 0U);
    return 0;
}
