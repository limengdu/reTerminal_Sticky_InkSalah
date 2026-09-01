#include "settings_store.h"

#include <cstddef>
#include <cstdint>

#include "app_log.h"
#include "nvs.h"

namespace {

constexpr char kTag[] = "settings_store";
constexpr char kNamespace[] = "salah_cfg";
constexpr char kKey[] = "settings";
constexpr uint32_t kMagic = 0x53414C41U;
constexpr uint16_t kVersion = 3U;

struct SettingsBlob {
    uint32_t magic = kMagic;
    uint16_t version = kVersion;
    uint16_t payload_size = sizeof(SalahSettings);
    SalahSettings settings = {};
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

}  // namespace

esp_err_t settings_store_load(SalahSettings &settings)
{
    app_log_register_tag(kTag);
    nvs_handle_t handle = 0;
    esp_err_t result = nvs_open(kNamespace, NVS_READONLY, &handle);
    if (result != ESP_OK) {
        return result;
    }
    SettingsBlob blob = {};
    size_t size = sizeof(blob);
    result = nvs_get_blob(handle, kKey, &blob, &size);
    nvs_close(handle);
    if (result != ESP_OK) {
        return result;
    }
    if (size != sizeof(blob) || blob.magic != kMagic ||
        blob.version != kVersion || blob.payload_size != sizeof(SalahSettings) ||
        blob.checksum != checksum_bytes(&blob.settings, sizeof(blob.settings)) ||
        !settings_persistable(blob.settings)) {
        STICKY_LOGW(kTag, "settings=load validity=invalid result=rejected");
        return ESP_ERR_INVALID_CRC;
    }
    settings = blob.settings;
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
