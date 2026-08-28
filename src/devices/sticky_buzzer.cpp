#include "sticky_buzzer.h"

#include <atomic>
#include <cstddef>

#include "app_log.h"
#include "driver/ledc.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "pin_config.h"

namespace {

constexpr char kTag[] = "sticky_buzzer";
constexpr ledc_mode_t kSpeedMode = LEDC_LOW_SPEED_MODE;
constexpr ledc_timer_t kTimer = LEDC_TIMER_0;
constexpr ledc_channel_t kChannel = LEDC_CHANNEL_0;
constexpr uint32_t kDefaultFrequencyHz = 1047U;
constexpr TickType_t kCheckInterval = pdMS_TO_TICKS(20);
constexpr uint32_t kTaskStackSize = 3072U;
constexpr UBaseType_t kTaskPriority = 4U;

struct BuzzerNote {
    uint32_t frequency_hz;
    uint32_t duration_ms;
    uint32_t gap_ms;
    uint32_t duty;
};

constexpr BuzzerNote kPrayerNotes[] = {
    {784U, 110U, 70U, 140U},
    {988U, 130U, 70U, 140U},
    {1319U, 260U, 0U, 160U},
};

TaskHandle_t s_buzzer_task = nullptr;
std::atomic<StickyBuzzerPattern> s_pattern = StickyBuzzerPattern::None;

esp_err_t set_duty(uint32_t duty)
{
    ESP_RETURN_ON_ERROR(
        ledc_set_duty(kSpeedMode, kChannel, duty), kTag, "set duty");
    return ledc_update_duty(kSpeedMode, kChannel);
}

bool wait_while_active(uint32_t duration_ms)
{
    TickType_t remaining = pdMS_TO_TICKS(duration_ms);
    while (remaining > 0U &&
           s_pattern.load() == StickyBuzzerPattern::Prayer) {
        const TickType_t slice = remaining > kCheckInterval
                                     ? kCheckInterval
                                     : remaining;
        vTaskDelay(slice);
        remaining -= slice;
    }
    return s_pattern.load() == StickyBuzzerPattern::Prayer;
}

esp_err_t play_note(const BuzzerNote &note)
{
    ESP_RETURN_ON_ERROR(
        ledc_set_freq(kSpeedMode, kTimer, note.frequency_hz),
        kTag,
        "set note frequency");
    if (s_pattern.load() != StickyBuzzerPattern::Prayer) {
        return ESP_OK;
    }
    ESP_RETURN_ON_ERROR(set_duty(note.duty), kTag, "start note");
    const bool keep_playing = wait_while_active(note.duration_ms);
    ESP_RETURN_ON_ERROR(set_duty(0U), kTag, "finish note");
    if (keep_playing) {
        wait_while_active(note.gap_ms);
    }
    return ESP_OK;
}

// Owns LEDC playback so display refresh remains independent.
// 在独立任务中驱动LEDC，让屏幕刷新与提醒音互不阻塞。
void buzzer_task(void *)
{
    while (true) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        esp_err_t result = ESP_OK;
        for (const BuzzerNote &note : kPrayerNotes) {
            if (s_pattern.load() != StickyBuzzerPattern::Prayer) {
                break;
            }
            result = play_note(note);
            if (result != ESP_OK) {
                break;
            }
        }
        set_duty(0U);
        StickyBuzzerPattern expected = StickyBuzzerPattern::Prayer;
        s_pattern.compare_exchange_strong(expected,
                                          StickyBuzzerPattern::None);
        if (result == ESP_OK) {
            STICKY_LOGD(kTag, "buzzer=prayer state=finished result=ok");
        } else {
            STICKY_LOGW(kTag,
                        "buzzer=prayer state=failed result=%s",
                        esp_err_to_name(result));
        }
    }
}

}  // namespace

esp_err_t sticky_buzzer_init()
{
    app_log_register_tag(kTag);

    ledc_timer_config_t timer_config = {};
    timer_config.speed_mode = kSpeedMode;
    timer_config.timer_num = kTimer;
    timer_config.duty_resolution = LEDC_TIMER_10_BIT;
    timer_config.freq_hz = kDefaultFrequencyHz;
    timer_config.clk_cfg = LEDC_AUTO_CLK;
    ESP_RETURN_ON_ERROR(
        ledc_timer_config(&timer_config), kTag, "configure timer");

    ledc_channel_config_t channel_config = {};
    channel_config.gpio_num = PIN_BUZZER;
    channel_config.speed_mode = kSpeedMode;
    channel_config.channel = kChannel;
    channel_config.intr_type = LEDC_INTR_DISABLE;
    channel_config.timer_sel = kTimer;
    channel_config.duty = 0U;
    channel_config.hpoint = 0;
    ESP_RETURN_ON_ERROR(
        ledc_channel_config(&channel_config), kTag, "configure channel");

    if (xTaskCreate(buzzer_task,
                    "sticky_buzzer",
                    kTaskStackSize,
                    nullptr,
                    kTaskPriority,
                    &s_buzzer_task) != pdPASS) {
        return ESP_ERR_NO_MEM;
    }

    STICKY_LOGI(kTag,
                "buzzer=ready pin=%d patterns=prayer result=ok",
                PIN_BUZZER);
    return ESP_OK;
}

esp_err_t sticky_buzzer_play_prayer_chime()
{
    if (s_buzzer_task == nullptr) {
        return ESP_ERR_INVALID_STATE;
    }
    s_pattern.store(StickyBuzzerPattern::Prayer);
    xTaskNotifyGive(s_buzzer_task);
    STICKY_LOGI(kTag,
                "buzzer=prayer state=started pattern=gentle_three_note");
    return ESP_OK;
}

esp_err_t sticky_buzzer_stop()
{
    const StickyBuzzerPattern previous = s_pattern.exchange(
        StickyBuzzerPattern::None);
    ESP_RETURN_ON_ERROR(set_duty(0U), kTag, "stop reminder");
    if (previous != StickyBuzzerPattern::None) {
        STICKY_LOGI(kTag, "buzzer=stopped previous=prayer");
    }
    return ESP_OK;
}
