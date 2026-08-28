#pragma once

#include "esp_err.h"
#include "prayer_schedule.h"
#include "settings_model.h"

// Downloads the current and following calendar months into one local schedule.
// 下载当前月和下一个月，并合并为一份本地礼拜时间表。
esp_err_t aladhan_fetch_two_month_schedule(const SalahSettings &settings,
                                           const PrayerDate &current_date,
                                           PrayerSchedule &schedule);
