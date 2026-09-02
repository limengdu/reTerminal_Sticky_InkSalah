#include "settings_model.h"

#include <cmath>
#include <cstring>

namespace {

constexpr CalculationMethodDefinition kCalculationMethods[] = {
    {0U, "JAFARI", 160U, 140U, 0U, 40U, 0},
    {1U, "KARACHI", 180U, 180U, 0U},
    {2U, "ISNA", 150U, 150U, 0U},
    {3U, "MWL", 180U, 170U, 0U},
    {4U, "UMM AL-QURA", 185U, 0U, 90U},
    {5U, "EGYPT", 195U, 175U, 0U},
    {7U, "TEHRAN", 177U, 140U, 0U, 45U, 0},
    {8U, "GULF", 195U, 0U, 90U},
    {9U, "KUWAIT", 180U, 175U, 0U},
    {10U, "QATAR", 180U, 0U, 90U},
    {11U, "MUIS", 200U, 180U, 0U},
    {12U, "UOIF", 120U, 120U, 0U},
    {13U, "DIYANET", 180U, 170U, 0U},
    {14U, "RUSSIA", 160U, 150U, 0U},
    {15U, "MOONSIGHTING", 180U, 180U, 0U, 0U, 0, true},
    {16U, "DUBAI", 182U, 182U, 0U},
    {17U, "JAKIM", 200U, 180U, 0U},
    {18U, "TUNISIA", 180U, 180U, 0U},
    {19U, "ALGERIA", 180U, 170U, 0U},
    {20U, "KEMENAG", 200U, 180U, 0U},
    {21U, "MOROCCO", 190U, 170U, 0U},
    {22U, "PORTUGAL", 180U, 0U, 77U, 0U, 3},
    {23U, "JORDAN", 180U, 180U, 0U, 0U, 5},
    {kCalculationMethodCustomAngles,
     "CUSTOM ANGLES", 180U, 170U, 0U},
    {kCalculationMethodCustomInterval,
     "CUSTOM INTERVAL", 180U, 0U, 90U},
};

bool terminated(const char *text, size_t capacity)
{
    return text != nullptr && std::memchr(text, '\0', capacity) != nullptr;
}

void hash_bytes(uint32_t &hash, const void *data, size_t length)
{
    const auto *bytes = static_cast<const uint8_t *>(data);
    for (size_t index = 0U; index < length; ++index) {
        hash ^= bytes[index];
        hash *= 16777619U;
    }
}

bool prayer_profile_valid(const SalahSettings &settings)
{
    for (size_t index = 0U; index < kPrayerAdjustmentCount; ++index) {
        if (settings.prayer_adjustments[index] < -30 ||
            settings.prayer_adjustments[index] > 30) {
            return false;
        }
    }
    return settings.custom_fajr_angle_tenths >=
               kMinimumTwilightAngleTenths &&
           settings.custom_fajr_angle_tenths <=
               kMaximumTwilightAngleTenths &&
           settings.custom_isha_angle_tenths >=
               kMinimumTwilightAngleTenths &&
           settings.custom_isha_angle_tenths <=
               kMaximumTwilightAngleTenths &&
           settings.custom_isha_interval_minutes >=
               kMinimumIshaIntervalMinutes &&
           settings.custom_isha_interval_minutes <=
               kMaximumIshaIntervalMinutes;
}

}  // namespace

void settings_store_defaults(SalahSettings &settings)
{
    settings = {};
    std::strncpy(settings.city, "Shenzhen", sizeof(settings.city) - 1U);
    std::strncpy(settings.country, "China", sizeof(settings.country) - 1U);
    settings.latitude = 22.5431;
    settings.longitude = 114.0579;
    settings.location_confirmed = false;
    settings.utc_offset_minutes = 480;
    settings.calculation_method = 3U;
    settings.asr_school = 1U;
    settings.sound_enabled = false;
    settings.custom_fajr_angle_tenths = 180U;
    settings.custom_isha_angle_tenths = 170U;
    settings.custom_isha_interval_minutes = 90U;
    settings.qiyam_visible = true;
}

bool settings_wifi_valid(const SalahSettings &settings)
{
    return terminated(settings.wifi_ssid, sizeof(settings.wifi_ssid)) &&
           terminated(settings.wifi_password, sizeof(settings.wifi_password)) &&
           settings.wifi_ssid[0] != '\0';
}

bool settings_store_valid(const SalahSettings &settings)
{
    return settings.configured && settings_wifi_valid(settings) &&
           terminated(settings.city, sizeof(settings.city)) &&
           terminated(settings.country, sizeof(settings.country)) &&
           settings.city[0] != '\0' && settings.country[0] != '\0' &&
           settings.location_confirmed && std::isfinite(settings.latitude) &&
           std::isfinite(settings.longitude) &&
           settings.latitude >= -90.0 && settings.latitude <= 90.0 &&
           settings.longitude >= -180.0 && settings.longitude <= 180.0 &&
           settings.utc_offset_minutes >= -12 * 60 &&
           settings.utc_offset_minutes <= 14 * 60 &&
           settings_calculation_method_valid(settings.calculation_method) &&
           settings.asr_school <= 1U &&
           prayer_profile_valid(settings);
}

size_t settings_calculation_method_count()
{
    return sizeof(kCalculationMethods) / sizeof(kCalculationMethods[0]);
}

const CalculationMethodDefinition *settings_calculation_method_at(
    size_t index)
{
    return index < settings_calculation_method_count()
               ? &kCalculationMethods[index]
               : nullptr;
}

const CalculationMethodDefinition *settings_calculation_method_by_id(
    uint8_t method)
{
    for (const CalculationMethodDefinition &definition :
         kCalculationMethods) {
        if (definition.id == method) {
            return &definition;
        }
    }
    return nullptr;
}

bool settings_calculation_method_valid(uint8_t method)
{
    return settings_calculation_method_by_id(method) != nullptr;
}

const char *settings_method_short_name(uint8_t method)
{
    const CalculationMethodDefinition *definition =
        settings_calculation_method_by_id(method);
    return definition == nullptr ? "METHOD" : definition->short_name;
}

const char *settings_asr_school_name(uint8_t school)
{
    return school == 1U ? "HANAFI" : "STANDARD";
}

uint32_t settings_prayer_profile_signature(const SalahSettings &settings)
{
    // Binds cached prayer days to every setting that changes Adhan times.
    // 将缓存日程绑定到所有会改变喚礼时间的设置。
    uint32_t hash = 2166136261U;
    hash_bytes(hash, settings.city, std::strlen(settings.city));
    hash_bytes(hash, settings.country, std::strlen(settings.country));
    hash_bytes(hash, &settings.latitude, sizeof(settings.latitude));
    hash_bytes(hash, &settings.longitude, sizeof(settings.longitude));
    hash_bytes(hash, &settings.utc_offset_minutes,
               sizeof(settings.utc_offset_minutes));
    hash_bytes(hash, &settings.calculation_method,
               sizeof(settings.calculation_method));
    hash_bytes(hash, &settings.asr_school, sizeof(settings.asr_school));
    hash_bytes(hash, &settings.custom_fajr_angle_tenths,
               sizeof(settings.custom_fajr_angle_tenths));
    hash_bytes(hash, &settings.custom_isha_angle_tenths,
               sizeof(settings.custom_isha_angle_tenths));
    hash_bytes(hash, &settings.custom_isha_interval_minutes,
               sizeof(settings.custom_isha_interval_minutes));
    hash_bytes(hash, settings.prayer_adjustments,
               sizeof(settings.prayer_adjustments));
    return hash;
}
