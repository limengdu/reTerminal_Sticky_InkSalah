#include "time_sync.h"

#include <ctime>

#include "app_log.h"
#include "esp_sntp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace {

constexpr char kTag[] = "time_sync";
constexpr std::time_t kMinimumValidEpoch = 1735689600;

bool epoch_to_local(std::time_t epoch,
                    int16_t utc_offset_minutes,
                    StickyRtcDateTime &local_time)
{
    if (epoch < kMinimumValidEpoch) {
        return false;
    }
    epoch += static_cast<std::time_t>(utc_offset_minutes) * 60;
    std::tm value = {};
    if (gmtime_r(&epoch, &value) == nullptr) {
        return false;
    }
    local_time.year = static_cast<uint16_t>(value.tm_year + 1900);
    local_time.month = static_cast<uint8_t>(value.tm_mon + 1);
    local_time.day = static_cast<uint8_t>(value.tm_mday);
    local_time.hour = static_cast<uint8_t>(value.tm_hour);
    local_time.minute = static_cast<uint8_t>(value.tm_min);
    local_time.second = static_cast<uint8_t>(value.tm_sec);
    return true;
}

}  // namespace

bool time_sync_local_now(int16_t utc_offset_minutes,
                         StickyRtcDateTime &local_time)
{
    return epoch_to_local(std::time(nullptr), utc_offset_minutes, local_time);
}

esp_err_t time_sync_from_network(int16_t utc_offset_minutes,
                                 StickyRtcDateTime &local_time,
                                 uint32_t timeout_ms)
{
    app_log_register_tag(kTag);
    if (esp_sntp_enabled()) {
        esp_sntp_stop();
    }
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, const_cast<char *>("pool.ntp.org"));
    esp_sntp_setservername(1, const_cast<char *>("time.cloudflare.com"));
    esp_sntp_init();

    const TickType_t started = xTaskGetTickCount();
    const TickType_t timeout_ticks = pdMS_TO_TICKS(timeout_ms);
    while (xTaskGetTickCount() - started < timeout_ticks) {
        if (time_sync_local_now(utc_offset_minutes, local_time)) {
            const esp_err_t rtc_result = sticky_rtc_write(local_time);
            if (rtc_result != ESP_OK) {
                return rtc_result;
            }
            STICKY_LOGI(kTag,
                        "time=sync offset_minutes=%d result=ok",
                        static_cast<int>(utc_offset_minutes));
            return ESP_OK;
        }
        vTaskDelay(pdMS_TO_TICKS(250));
    }
    STICKY_LOGW(kTag, "time=sync result=timeout");
    return ESP_ERR_TIMEOUT;
}
