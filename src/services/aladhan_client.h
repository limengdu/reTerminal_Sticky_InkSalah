#pragma once

#include "esp_err.h"
#include "prayer_schedule.h"
#include "settings_model.h"

// Downloads the current and following calendar months into one local schedule.
// 下载当前月和下一个月，并合并为一份本地礼拜时间表。
esp_err_t aladhan_fetch_two_month_schedule(const SalahSettings &settings,
                                           const PrayerDate &current_date,
                                           PrayerSchedule &schedule);

// Confirms that one city/country pair returns all required prayer times.
// 确认国家和城市组合能够返回完整的礼拜时间。
esp_err_t aladhan_validate_location(const SalahSettings &settings);
