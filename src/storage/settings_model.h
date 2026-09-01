#pragma once

#include <cstddef>
#include <cstdint>

constexpr size_t kPrayerAdjustmentCount = 7U;

enum class PrayerAdjustment : uint8_t {
    Fajr,
    Sunrise,
    Dhuhr,
    Asr,
    Maghrib,
    Isha,
    Qiyam,
    Count,
};

struct SalahSettings {
    bool configured = false;
    char wifi_ssid[33] = {};
    char wifi_password[65] = {};
    char city[48] = {};
    char country[48] = {};
    double latitude = 22.5431;
    double longitude = 114.0579;
    bool location_confirmed = false;
    int16_t utc_offset_minutes = 480;
    uint8_t calculation_method = 3U;
    uint8_t asr_school = 1U;
    int8_t prayer_adjustments[kPrayerAdjustmentCount] = {};
    bool sound_enabled = false;
};

void settings_store_defaults(SalahSettings &settings);
bool settings_store_valid(const SalahSettings &settings);
bool settings_wifi_valid(const SalahSettings &settings);
const char *settings_method_short_name(uint8_t method);
const char *settings_asr_school_name(uint8_t school);
uint32_t settings_prayer_profile_signature(const SalahSettings &settings);
