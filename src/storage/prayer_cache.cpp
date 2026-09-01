#include "prayer_cache.h"

#include <cstddef>
#include <cstdint>

#include "app_log.h"
#include "esp_heap_caps.h"
#include "nvs.h"

namespace {

constexpr char kTag[] = "prayer_cache";
constexpr char kNamespace[] = "salah_cache";
constexpr char kKey[] = "schedule";
constexpr uint32_t kMagic = 0x50524159U;
constexpr uint16_t kVersion = 2U;

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
    // NVS transfers the complete two-month cache through a PSRAM buffer.
    // NVS通过PSRAM缓冲传输完整的两个月缓存。
    auto *blob = static_cast<CacheBlob *>(heap_caps_calloc(
        1U,
        sizeof(CacheBlob),
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (blob == nullptr) {
        return ESP_ERR_NO_MEM;
    }
    nvs_handle_t handle = 0;
    esp_err_t result = nvs_open(kNamespace, NVS_READONLY, &handle);
    if (result != ESP_OK) {
        heap_caps_free(blob);
        return result;
    }
    size_t size = sizeof(*blob);
    result = nvs_get_blob(handle, kKey, blob, &size);
    nvs_close(handle);
    if (result != ESP_OK) {
        heap_caps_free(blob);
        return result;
    }
    if (size != sizeof(*blob) || blob->magic != kMagic ||
        blob->version != kVersion ||
        blob->payload_size != sizeof(PrayerSchedule) ||
        blob->checksum != checksum_bytes(&blob->schedule,
                                         sizeof(blob->schedule)) ||
        !schedule_valid(blob->schedule)) {
        heap_caps_free(blob);
        STICKY_LOGW(kTag, "cache=load validity=invalid result=rejected");
        return ESP_ERR_INVALID_CRC;
    }
    schedule = blob->schedule;
    heap_caps_free(blob);
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
    // NVS transfers the complete two-month cache through a PSRAM buffer.
    // NVS通过PSRAM缓冲传输完整的两个月缓存。
    auto *blob = static_cast<CacheBlob *>(heap_caps_calloc(
        1U,
        sizeof(CacheBlob),
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (blob == nullptr) {
        return ESP_ERR_NO_MEM;
    }
    blob->magic = kMagic;
    blob->version = kVersion;
    blob->payload_size = sizeof(PrayerSchedule);
    blob->schedule = schedule;
    blob->checksum = checksum_bytes(&blob->schedule, sizeof(blob->schedule));

    nvs_handle_t handle = 0;
    esp_err_t result = nvs_open(kNamespace, NVS_READWRITE, &handle);
    if (result != ESP_OK) {
        heap_caps_free(blob);
        return result;
    }
    result = nvs_set_blob(handle, kKey, blob, sizeof(*blob));
    if (result == ESP_OK) {
        result = nvs_commit(handle);
    }
    nvs_close(handle);
    heap_caps_free(blob);
    if (result == ESP_OK) {
        STICKY_LOGI(kTag,
                    "cache=save days=%u result=ok",
                    static_cast<unsigned>(schedule.count));
    }
    return result;
}
