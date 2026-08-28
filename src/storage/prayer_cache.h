#pragma once

#include "esp_err.h"
#include "prayer_schedule.h"

esp_err_t prayer_cache_load(PrayerSchedule &schedule);
esp_err_t prayer_cache_save(const PrayerSchedule &schedule);
