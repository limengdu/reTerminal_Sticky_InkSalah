#include "settings_store.h"

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "app_log.h"
#include "nvs.h"

namespace {

constexpr char kTag[] = "settings_store";
constexpr char kNamespace[] = "salah_cfg";
constexpr char kKey[] = "settings";
constexpr uint32_t kMagic = 0x53414C41U;
constexpr uint16_t kVersion = 4U;

struct SalahSettingsV3 {
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

struct SettingsBlob {
    uint32_t magic = kMagic;
    uint16_t version = kVersion;
    uint16_t payload_size = sizeof(SalahSettings);
    SalahSettings settings = {};
    uint32_t checksum = 0U;
};

struct SettingsBlobV3 {
    uint32_t magic = kMagic;
    uint16_t version = 3U;
    uint16_t payload_size = sizeof(SalahSettingsV3);
    SalahSettingsV3 settings = {};
    uint32_t checksum = 0U;
};

uint32_t checksum_bytes(const void *data, size_t length)
{
    const auto *bytes = static_cast<const uint8_t *>(data);
    uint32_t value = 2166136261U;
    for (size_t index = 0U; index < length; ++index) {
        value ^= bytes[index];
        value *= 16777619U;
    }
    return value;
}

bool settings_persistable(const SalahSettings &settings)
{
    return settings_wifi_valid(settings) &&
           (!settings.configured || settings_store_valid(settings));
}

// Copies a version-3 payload and fills version-4 custom fields with defaults.
// 复制第三版设置，并用默认值补齐第四版新增的自定义计算字段。
void migrate_v3_settings(const SalahSettingsV3 &source,
                         SalahSettings &destination)
{
    settings_store_defaults(destination);
    destination.configured = source.configured;
    std::memcpy(destination.wifi_ssid, source.wifi_ssid,
                sizeof(source.wifi_ssid));
    std::memcpy(destination.wifi_password, source.wifi_password,
                sizeof(source.wifi_password));
    std::memcpy(destination.city, source.city, sizeof(source.city));
    std::memcpy(destination.country, source.country, sizeof(source.country));
    destination.latitude = source.latitude;
    destination.longitude = source.longitude;
    destination.location_confirmed = source.location_confirmed;
    destination.utc_offset_minutes = source.utc_offset_minutes;
    destination.calculation_method = source.calculation_method;
    destination.asr_school = source.asr_school;
    std::memcpy(destination.prayer_adjustments,
                source.prayer_adjustments,
                sizeof(source.prayer_adjustments));
    destination.sound_enabled = source.sound_enabled;
}

}  // namespace

esp_err_t settings_store_load(SalahSettings &settings)
{
    app_log_register_tag(kTag);
    nvs_handle_t handle = 0;
    esp_err_t result = nvs_open(kNamespace, NVS_READONLY, &handle);
    if (result != ESP_OK) {
        return result;
    }
    size_t size = 0U;
    result = nvs_get_blob(handle, kKey, nullptr, &size);
    if (result != ESP_OK) {
        nvs_close(handle);
        return result;
    }
    bool migrated = false;
    if (size == sizeof(SettingsBlob)) {
        SettingsBlob blob = {};
        result = nvs_get_blob(handle, kKey, &blob, &size);
        if (result == ESP_OK && blob.magic == kMagic &&
            blob.version == kVersion &&
            blob.payload_size == sizeof(SalahSettings) &&
            blob.checksum ==
                checksum_bytes(&blob.settings, sizeof(blob.settings)) &&
            settings_persistable(blob.settings)) {
            settings = blob.settings;
        } else {
            result = ESP_ERR_INVALID_CRC;
        }
    } else if (size == sizeof(SettingsBlobV3)) {
        SettingsBlobV3 blob = {};
        result = nvs_get_blob(handle, kKey, &blob, &size);
        if (result == ESP_OK && blob.magic == kMagic &&
            blob.version == 3U &&
            blob.payload_size == sizeof(SalahSettingsV3) &&
            blob.checksum ==
                checksum_bytes(&blob.settings, sizeof(blob.settings))) {
            migrate_v3_settings(blob.settings, settings);
            if (!settings_persistable(settings)) {
                result = ESP_ERR_INVALID_CRC;
            } else {
                migrated = true;
            }
        } else {
            result = ESP_ERR_INVALID_CRC;
        }
    } else {
        result = ESP_ERR_INVALID_SIZE;
    }
    nvs_close(handle);
    if (result != ESP_OK) {
        STICKY_LOGW(kTag, "settings=load validity=invalid result=rejected");
        return result;
    }
    if (migrated) {
        const esp_err_t save_result = settings_store_save(settings);
        STICKY_LOGI(kTag,
                    "settings=migrate from=3 to=4 result=%s",
                    esp_err_to_name(save_result));
    }
    STICKY_LOGI(kTag,
                "settings=load city=%s country=%s latitude=%.5f longitude=%.5f method=%u school=%u result=ok",
                settings.city,
                settings.country,
                settings.latitude,
                settings.longitude,
                static_cast<unsigned>(settings.calculation_method),
                static_cast<unsigned>(settings.asr_school));
    return ESP_OK;
}

esp_err_t settings_store_save(const SalahSettings &settings)
{
    if (!settings_persistable(settings)) {
        return ESP_ERR_INVALID_ARG;
    }
    SettingsBlob blob = {};
    blob.settings = settings;
    blob.checksum = checksum_bytes(&blob.settings, sizeof(blob.settings));

    nvs_handle_t handle = 0;
    esp_err_t result = nvs_open(kNamespace, NVS_READWRITE, &handle);
    if (result != ESP_OK) {
        return result;
    }
    result = nvs_set_blob(handle, kKey, &blob, sizeof(blob));
    if (result == ESP_OK) {
        result = nvs_commit(handle);
    }
    nvs_close(handle);
    if (result == ESP_OK) {
        STICKY_LOGI(kTag,
                    "settings=save city=%s country=%s latitude=%.5f longitude=%.5f method=%u result=ok",
                    settings.city,
                    settings.country,
                    settings.latitude,
                    settings.longitude,
                    static_cast<unsigned>(settings.calculation_method));
    }
    return result;
}
