#include "settings_model.h"

#include <cmath>
#include <cstring>

namespace {

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
    return true;
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
           settings.calculation_method <= 99U && settings.asr_school <= 1U &&
           prayer_profile_valid(settings);
}

const char *settings_method_short_name(uint8_t method)
{
    switch (method) {
    case 0U:
        return "JAFARI";
    case 1U:
        return "KARACHI";
    case 2U:
        return "ISNA";
    case 3U:
        return "MWL";
    case 4U:
        return "UMM AL-QURA";
    case 5U:
        return "EGYPT";
    case 7U:
        return "TEHRAN";
    case 8U:
        return "GULF";
    case 9U:
        return "KUWAIT";
    case 10U:
        return "QATAR";
    case 11U:
        return "MUIS";
    case 12U:
        return "UOIF";
    case 13U:
        return "DIYANET";
    case 14U:
        return "RUSSIA";
    case 15U:
        return "MOONSIGHT";
    case 16U:
        return "DUBAI";
    case 17U:
        return "JAKIM";
    case 18U:
        return "TUNISIA";
    case 19U:
        return "ALGERIA";
    case 20U:
        return "KEMENAG";
    case 21U:
        return "MOROCCO";
    case 22U:
        return "LISBON";
    case 23U:
        return "JORDAN";
    case 99U:
        return "CUSTOM";
    default:
        return "METHOD";
    }
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
    hash_bytes(hash, settings.prayer_adjustments,
               sizeof(settings.prayer_adjustments));
    return hash;
}
