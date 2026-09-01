#pragma once

#include "prayer_time.h"
#include "settings_model.h"

// Calculates seven local times from coordinates, date, and prayer settings.
// 根据经纬度、日期和礼拜设置计算七项本地时间。
bool prayer_calculate_day(const SalahSettings &settings,
                          const PrayerDate &date,
                          PrayerDay &day);

