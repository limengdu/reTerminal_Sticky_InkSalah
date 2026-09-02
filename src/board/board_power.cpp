#include "board_power.h"

#include <cstdio>

#include "app_log.h"
#include "driver/gpio.h"
#include "esp_sleep.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "pin_config.h"

namespace {

constexpr char kTag[] = "sticky_power";
constexpr TickType_t kPowerSettleDelay = pdMS_TO_TICKS(100);
constexpr TickType_t kReleasePollInterval = pdMS_TO_TICKS(20);
constexpr TickType_t kReleaseDebounceTime = pdMS_TO_TICKS(60);

esp_err_t set_power_level(gpio_num_t pin, uint32_t level)
{
    // Centralize GPIO writes so every failed latch operation has the same log shape.
    // 集中处理锁存引脚写入，让所有失败日志都保持相同格式。
    const esp_err_t result = gpio_set_level(pin, level);
    if (result != ESP_OK) {
        STICKY_LOGE(kTag,
                    "step=set_level pin=%d level=%lu result=%s",
                    static_cast<int>(pin),
                    static_cast<unsigned long>(level),
                    esp_err_to_name(result));
    }
    return result;
}

esp_err_t hold_output(gpio_num_t pin, int level)
{
    gpio_hold_dis(pin);
    esp_err_t result = gpio_set_direction(pin, GPIO_MODE_OUTPUT);
    if (result != ESP_OK) {
        return result;
    }
    result = gpio_set_level(pin, level);
    return result == ESP_OK ? gpio_hold_en(pin) : result;
}

void wait_for_sleep_chord_release()
{
    STICKY_LOGI(kTag,
                "power=sleep_chord state=waiting_for_release pins=%d,%d",
                PIN_SIDE_BUTTON_LEFT,
                PIN_SIDE_BUTTON_RIGHT);
    while (gpio_get_level(
               static_cast<gpio_num_t>(PIN_SIDE_BUTTON_LEFT)) == 0 ||
           gpio_get_level(
               static_cast<gpio_num_t>(PIN_SIDE_BUTTON_RIGHT)) == 0) {
        vTaskDelay(kReleasePollInterval);
    }
    vTaskDelay(kReleaseDebounceTime);
}

esp_err_t configure_active_low_wakeup_input(gpio_num_t pin)
{
    gpio_hold_dis(pin);
    esp_err_t result = gpio_set_direction(pin, GPIO_MODE_INPUT);
    if (result == ESP_OK) {
        result = gpio_pullup_en(pin);
    }
    if (result == ESP_OK) {
        result = gpio_pulldown_dis(pin);
    }
    if (result == ESP_OK) {
        result = gpio_hold_en(pin);
    }
    return result;
}

esp_err_t configure_active_high_wakeup_input(gpio_num_t pin)
{
    gpio_hold_dis(pin);
    esp_err_t result = gpio_set_direction(pin, GPIO_MODE_INPUT);
    if (result == ESP_OK) {
        result = gpio_pulldown_en(pin);
    }
    if (result == ESP_OK) {
        result = gpio_pullup_dis(pin);
    }
    if (result == ESP_OK) {
        result = gpio_hold_en(pin);
    }
    return result;
}

const char *wakeup_source_name(esp_sleep_wakeup_cause_t cause,
                               uint64_t external_status)
{
    if (cause == ESP_SLEEP_WAKEUP_EXT0) {
        return "touch_gesture";
    }
    if (cause == ESP_SLEEP_WAKEUP_TIMER) {
        return "timer";
    }
    if (cause == ESP_SLEEP_WAKEUP_EXT1) {
        if ((external_status & (1ULL << PIN_IMU_INT)) != 0U) {
            return "orientation";
        }
        if ((external_status & (1ULL << PIN_TOP_BUTTON)) != 0U) {
            return "button";
        }
        return "external";
    }
    return cause == ESP_SLEEP_WAKEUP_UNDEFINED ? "cold_boot" : "other";
}

}  // namespace

esp_err_t board_power_init()
{
    app_log_register_tag(kTag);

    const esp_sleep_wakeup_cause_t wake_cause =
        esp_sleep_get_wakeup_cause();
    const bool woke_from_sleep =
        wake_cause != ESP_SLEEP_WAKEUP_UNDEFINED;
    const uint64_t external_wakeup_status =
        wake_cause == ESP_SLEEP_WAKEUP_EXT1
            ? esp_sleep_get_ext1_wakeup_status()
            : 0U;
    STICKY_LOGI(kTag,
                "step=begin wake_from_sleep=%s wake_source=%s wake_cause=%d wake_mask=0x%llX hold_pin=%d lock_pin=%d",
                woke_from_sleep ? "true" : "false",
                wakeup_source_name(wake_cause, external_wakeup_status),
                static_cast<int>(wake_cause),
                static_cast<unsigned long long>(external_wakeup_status),
                PIN_POWER_HOLD,
                PIN_POWER_LOCK);

    if (woke_from_sleep) {
        // Preload retained outputs before releasing deep-sleep holds.
        // 在释放深度睡眠保持前预先恢复输出电平。
        gpio_set_direction(static_cast<gpio_num_t>(PIN_POWER_HOLD),
                           GPIO_MODE_OUTPUT);
        gpio_set_level(static_cast<gpio_num_t>(PIN_POWER_HOLD), 1);
        gpio_set_direction(static_cast<gpio_num_t>(PIN_POWER_LOCK),
                           GPIO_MODE_OUTPUT);
        gpio_set_level(static_cast<gpio_num_t>(PIN_POWER_LOCK), 1);
    }

    gpio_deep_sleep_hold_dis();
    constexpr int kHeldPins[] = {
        PIN_POWER_HOLD,
        PIN_POWER_LOCK,
        PIN_TOP_BUTTON,
        PIN_EPD_EN,
        PIN_TOUCH_EN,
        PIN_TOUCH_RST,
        PIN_TOUCH_INT,
        PIN_SD_EN,
        PIN_BUZZER,
        PIN_IMU_INT,
    };
    for (int held_pin : kHeldPins) {
        gpio_hold_dis(static_cast<gpio_num_t>(held_pin));
    }

    gpio_config_t config = {};
    config.pin_bit_mask =
        (1ULL << PIN_POWER_HOLD) | (1ULL << PIN_POWER_LOCK);
    config.mode = GPIO_MODE_OUTPUT;
    config.pull_up_en = GPIO_PULLUP_DISABLE;
    config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    config.intr_type = GPIO_INTR_DISABLE;

    esp_err_t result = gpio_config(&config);
    if (result != ESP_OK) {
        STICKY_LOGE(kTag,
                    "step=configure_outputs result=%s",
                    esp_err_to_name(result));
        return result;
    }

    result = set_power_level(static_cast<gpio_num_t>(PIN_POWER_HOLD), 1);
    if (result != ESP_OK) {
        return result;
    }

    result = set_power_level(static_cast<gpio_num_t>(PIN_POWER_LOCK), 1);
    if (result != ESP_OK) {
        return result;
    }

    vTaskDelay(kPowerSettleDelay);
    STICKY_LOGI(kTag, "step=ready hold=high lock=high result=ok");
    return ESP_OK;
}

bool board_power_woke_from_orientation()
{
    return esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT1 &&
           (esp_sleep_get_ext1_wakeup_status() &
            (1ULL << PIN_IMU_INT)) != 0U;
}


[[noreturn]] void board_power_enter_deep_sleep(
    uint64_t timer_wakeup_us,
    bool orientation_wakeup_enabled,
    bool touch_wakeup_enabled,
    uint8_t touch_wakeup_level)
{
    const gpio_num_t button_wake_pin =
        static_cast<gpio_num_t>(PIN_TOP_BUTTON);
    const gpio_num_t orientation_wake_pin =
        static_cast<gpio_num_t>(PIN_IMU_INT);
    const gpio_num_t touch_wake_pin =
        static_cast<gpio_num_t>(PIN_TOUCH_INT);
    wait_for_sleep_chord_release();

    // Keep the board latch active, retain the touch controller only when
    // gesture wake is enabled, and power down the remaining peripherals.
    // 保持板级锁存；仅在启用手势唤醒时保留触摸控制器，其余外设进入断电状态。
    ESP_ERROR_CHECK(hold_output(
        static_cast<gpio_num_t>(PIN_POWER_HOLD), 1));
    ESP_ERROR_CHECK(hold_output(
        static_cast<gpio_num_t>(PIN_POWER_LOCK), 1));
    ESP_ERROR_CHECK(hold_output(
        static_cast<gpio_num_t>(PIN_EPD_EN), 0));
    ESP_ERROR_CHECK(hold_output(
        static_cast<gpio_num_t>(PIN_TOUCH_EN),
        touch_wakeup_enabled ? 1 : 0));
    ESP_ERROR_CHECK(hold_output(
        static_cast<gpio_num_t>(PIN_TOUCH_RST),
        touch_wakeup_enabled ? 1 : 0));
    ESP_ERROR_CHECK(hold_output(
        static_cast<gpio_num_t>(PIN_SD_EN), 0));
    ESP_ERROR_CHECK(hold_output(
        static_cast<gpio_num_t>(PIN_BUZZER), 0));

    ESP_ERROR_CHECK(configure_active_low_wakeup_input(button_wake_pin));
    uint64_t external_wakeup_mask = 1ULL << PIN_TOP_BUTTON;
    if (orientation_wakeup_enabled) {
        ESP_ERROR_CHECK(
            configure_active_low_wakeup_input(orientation_wake_pin));
        external_wakeup_mask |= 1ULL << PIN_IMU_INT;
    }
    if (touch_wakeup_enabled) {
        if (touch_wakeup_level == 0U) {
            ESP_ERROR_CHECK(configure_active_low_wakeup_input(touch_wake_pin));
        } else {
            ESP_ERROR_CHECK(configure_active_high_wakeup_input(touch_wake_pin));
        }
        ESP_ERROR_CHECK(esp_sleep_enable_ext0_wakeup(
            touch_wake_pin, touch_wakeup_level));
    }
    ESP_ERROR_CHECK(esp_sleep_enable_ext1_wakeup_io(
        external_wakeup_mask, ESP_EXT1_WAKEUP_ANY_LOW));
    if (timer_wakeup_us > 0U) {
        ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(timer_wakeup_us));
    }
    gpio_deep_sleep_hold_en();

    STICKY_LOGI(kTag,
                "power=deep_sleep wake_button=%d wake_orientation=%d orientation_enabled=%d wake_touch=%d touch_enabled=%d touch_level=%u timer_us=%llu result=entering",
                PIN_TOP_BUTTON,
                PIN_IMU_INT,
                orientation_wakeup_enabled,
                PIN_TOUCH_INT,
                touch_wakeup_enabled,
                static_cast<unsigned>(touch_wakeup_level),
                static_cast<unsigned long long>(timer_wakeup_us));
    std::fflush(stdout);
    vTaskDelay(pdMS_TO_TICKS(50));
    esp_deep_sleep_start();
}
