#pragma once

#include <cstddef>
#include <cstdint>

constexpr size_t kPrayerAdjustmentCount = 7U;
constexpr uint8_t kCalculationMethodCustomAngles = 99U;
constexpr uint8_t kCalculationMethodCustomInterval = 100U;
constexpr uint16_t kMinimumTwilightAngleTenths = 10U;
constexpr uint16_t kMaximumTwilightAngleTenths = 300U;
constexpr uint16_t kMinimumIshaIntervalMinutes = 1U;
constexpr uint16_t kMaximumIshaIntervalMinutes = 240U;

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

struct CalculationMethodDefinition {
    uint8_t id = 3U;
    const char *short_name = "MWL";
    uint16_t fajr_angle_tenths = 180U;
    uint16_t isha_angle_tenths = 170U;
    uint16_t isha_interval_minutes = 0U;
    uint16_t maghrib_angle_tenths = 0U;
    int8_t maghrib_offset_minutes = 0;
    bool seasonal_twilight = false;
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
    uint16_t custom_fajr_angle_tenths = 180U;
    uint16_t custom_isha_angle_tenths = 170U;
    uint16_t custom_isha_interval_minutes = 90U;
    bool qiyam_visible = true;
};

void settings_store_defaults(SalahSettings &settings);
bool settings_store_valid(const SalahSettings &settings);
bool settings_wifi_valid(const SalahSettings &settings);
size_t settings_calculation_method_count();
const CalculationMethodDefinition *settings_calculation_method_at(
    size_t index);
const CalculationMethodDefinition *settings_calculation_method_by_id(
    uint8_t method);
bool settings_calculation_method_valid(uint8_t method);
const char *settings_method_short_name(uint8_t method);
const char *settings_asr_school_name(uint8_t school);
uint32_t settings_prayer_profile_signature(const SalahSettings &settings);
