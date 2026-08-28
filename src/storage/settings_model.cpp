#include "settings_model.h"

#include <cstring>

namespace {

bool terminated(const char *text, size_t capacity)
{
    return text != nullptr && std::memchr(text, '\0', capacity) != nullptr;
}

}  // namespace

void settings_store_defaults(SalahSettings &settings)
{
    settings = {};
    std::strncpy(settings.city, "Jakarta", sizeof(settings.city) - 1U);
    std::strncpy(settings.country, "Indonesia", sizeof(settings.country) - 1U);
    settings.utc_offset_minutes = 420;
    settings.calculation_method = 20U;
    settings.asr_school = 0U;
    settings.sound_enabled = false;
}

bool settings_store_valid(const SalahSettings &settings)
{
    return settings.configured &&
           terminated(settings.wifi_ssid, sizeof(settings.wifi_ssid)) &&
           terminated(settings.wifi_password, sizeof(settings.wifi_password)) &&
           terminated(settings.city, sizeof(settings.city)) &&
           terminated(settings.country, sizeof(settings.country)) &&
           settings.wifi_ssid[0] != '\0' && settings.city[0] != '\0' &&
           settings.country[0] != '\0' &&
           settings.utc_offset_minutes >= -12 * 60 &&
           settings.utc_offset_minutes <= 14 * 60 &&
           settings.calculation_method <= 99U && settings.asr_school <= 1U;
}
