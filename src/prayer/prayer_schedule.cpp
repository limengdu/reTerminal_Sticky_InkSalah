#include "prayer_schedule.h"

void prayer_schedule_clear(PrayerSchedule &schedule)
{
    schedule = {};
}

bool prayer_schedule_add(PrayerSchedule &schedule, const PrayerDay &day)
{
    if (!prayer_day_valid(day)) {
        return false;
    }
    for (uint16_t index = 0U; index < schedule.count; ++index) {
        if (prayer_date_equal(schedule.days[index].date, day.date)) {
            schedule.days[index] = day;
            return true;
        }
    }
    if (schedule.count >= kPrayerScheduleMaxDays) {
        return false;
    }
    schedule.days[schedule.count++] = day;
    return true;
}

const PrayerDay *prayer_schedule_find(const PrayerSchedule &schedule,
                                      const PrayerDate &date)
{
    for (uint16_t index = 0U; index < schedule.count; ++index) {
        if (prayer_date_equal(schedule.days[index].date, date)) {
            return &schedule.days[index];
        }
    }
    return nullptr;
}

bool prayer_schedule_contains(const PrayerSchedule &schedule,
                              const PrayerDate &date)
{
    return prayer_schedule_find(schedule, date) != nullptr;
}
