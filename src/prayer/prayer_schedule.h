#pragma once

#include <cstddef>
#include <cstdint>

#include "prayer_time.h"

constexpr size_t kPrayerScheduleMaxDays = 64U;

struct PrayerSchedule {
    uint16_t count = 0U;
    PrayerDay days[kPrayerScheduleMaxDays] = {};
};

void prayer_schedule_clear(PrayerSchedule &schedule);
bool prayer_schedule_add(PrayerSchedule &schedule, const PrayerDay &day);
const PrayerDay *prayer_schedule_find(const PrayerSchedule &schedule,
                                      const PrayerDate &date);
bool prayer_schedule_contains(const PrayerSchedule &schedule,
                              const PrayerDate &date);
