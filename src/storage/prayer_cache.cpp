#include "prayer_cache.h"

#include <cstddef>
#include <cstdint>

#include "app_log.h"
#include "nvs.h"

namespace {

constexpr char kTag[] = "prayer_cache";
constexpr char kNamespace[] = "salah_cache";
constexpr char kKey[] = "schedule";
constexpr uint32_t kMagic = 0x50524159U;
constexpr uint16_t kVersion = 1U;

struct CacheBlob {
    uint32_t magic = kMagic;
    uint16_t version = kVersion;
    uint16_t payload_size = sizeof(PrayerSchedule);
    PrayerSchedule schedule = {};
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

bool schedule_valid(const PrayerSchedule &schedule)
{
    if (schedule.count == 0U || schedule.count > kPrayerScheduleMaxDays) {
        return false;
    }
    for (uint16_t index = 0U; index < schedule.count; ++index) {
        if (!prayer_day_valid(schedule.days[index])) {
            return false;
        }
    }
    return true;
}

}  // namespace

esp_err_t prayer_cache_load(PrayerSchedule &schedule)
{
    app_log_register_tag(kTag);
    nvs_handle_t handle = 0;
    esp_err_t result = nvs_open(kNamespace, NVS_READONLY, &handle);
    if (result != ESP_OK) {
        return result;
    }
    CacheBlob blob = {};
    size_t size = sizeof(blob);
    result = nvs_get_blob(handle, kKey, &blob, &size);
    nvs_close(handle);
    if (result != ESP_OK) {
        return result;
    }
    if (size != sizeof(blob) || blob.magic != kMagic ||
        blob.version != kVersion || blob.payload_size != sizeof(PrayerSchedule) ||
        blob.checksum != checksum_bytes(&blob.schedule, sizeof(blob.schedule)) ||
        !schedule_valid(blob.schedule)) {
        STICKY_LOGW(kTag, "cache=load validity=invalid result=rejected");
        return ESP_ERR_INVALID_CRC;
    }
    schedule = blob.schedule;
    STICKY_LOGI(kTag,
                "cache=load days=%u result=ok",
                static_cast<unsigned>(schedule.count));
    return ESP_OK;
}

esp_err_t prayer_cache_save(const PrayerSchedule &schedule)
{
    if (!schedule_valid(schedule)) {
        return ESP_ERR_INVALID_ARG;
    }
    CacheBlob blob = {};
    blob.schedule = schedule;
    blob.checksum = checksum_bytes(&blob.schedule, sizeof(blob.schedule));

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
                    "cache=save days=%u result=ok",
                    static_cast<unsigned>(schedule.count));
    }
    return result;
}
