#include "prayer_app.h"

#include <algorithm>
#include <cstdint>

#include "app_log.h"
#include "board_power.h"
#include "canvas.h"
#include "device_settings.h"
#include "display_orientation.h"
#include "esp_attr.h"
#include "esp_sleep.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "prayer_calculator.h"
#include "prayer_schedule.h"
#include "prayer_pages.h"
#include "provisioning_portal.h"
#include "settings_store.h"
#include "sticky_buzzer.h"
#include "sticky_display.h"
#include "sticky_imu.h"
#include "sticky_rtc.h"
#include "sticky_touch.h"
#include "sticky_touch_gesture.h"
#include "time_sync.h"
#include "wifi_service.h"

namespace {

constexpr char kTag[] = "prayer_app";
constexpr uint32_t kNetworkTimeoutMs = 16000U;
constexpr uint32_t kTimeSyncTimeoutMs = 12000U;
constexpr TickType_t kOrientationSettle = pdMS_TO_TICKS(1300);
constexpr TickType_t kInputWindow = pdMS_TO_TICKS(1200);
constexpr uint64_t kErrorRetryUs = 5ULL * 60ULL * 1000000ULL;

RTC_DATA_ATTR uint32_t s_last_time_sync_date = 0U;
RTC_DATA_ATTR uint8_t s_last_rotation =
    static_cast<uint8_t>(CanvasRotation::Deg90CounterClockwise);

uint32_t date_key(const StickyRtcDateTime &value)
{
    return static_cast<uint32_t>(value.year) * 10000U +
           static_cast<uint32_t>(value.month) * 100U + value.day;
}

PrayerDate prayer_date(const StickyRtcDateTime &value)
{
    return {value.year, value.month, value.day};
}

bool current_time(StickyRtcDateTime &value)
{
    if (sticky_rtc_read(value) == ESP_OK) {
        return true;
    }
    return sticky_rtc_seed_from_build_time(value) == ESP_OK;
}

CanvasRotation detect_rotation()
{
    if (sticky_imu_start_monitoring() != ESP_OK) {
        return static_cast<CanvasRotation>(s_last_rotation);
    }
    vTaskDelay(kOrientationSettle);
    StickyImuState state = {};
    const esp_err_t result = sticky_imu_get_state(state);
    sticky_imu_stop_monitoring();
    if (result != ESP_OK || !state.valid) {
        return static_cast<CanvasRotation>(s_last_rotation);
    }
    const CanvasRotation rotation = salah_rotation_from_orientation(
        state.orientation, static_cast<CanvasRotation>(s_last_rotation));
    s_last_rotation = static_cast<uint8_t>(rotation);
    return rotation;
}

[[noreturn]] void enter_deep_sleep(uint64_t timer_wakeup_us)
{
    const esp_err_t imu_result =
        sticky_imu_prepare_orientation_wakeup();
    const bool orientation_wakeup_ready = imu_result == ESP_OK;
    if (!orientation_wakeup_ready) {
        STICKY_LOGW(kTag,
                    "sleep=orientation_wakeup state=disabled result=%s",
                    esp_err_to_name(imu_result));
    }

    uint8_t touch_wakeup_level = 1U;
    const esp_err_t touch_result =
        sticky_touch_prepare_gesture_wakeup(touch_wakeup_level);
    const bool touch_wakeup_ready = touch_result == ESP_OK;
    if (!touch_wakeup_ready) {
        STICKY_LOGW(kTag,
                    "sleep=touch_wakeup state=disabled result=%s",
                    esp_err_to_name(touch_result));
    }

    const esp_err_t display_result = sticky_display_sleep();
    if (display_result != ESP_OK) {
        STICKY_LOGW(kTag,
                    "sleep=display state=power_down_failed result=%s",
                    esp_err_to_name(display_result));
    }
    board_power_enter_deep_sleep(timer_wakeup_us,
                                 orientation_wakeup_ready,
                                 touch_wakeup_ready,
                                 touch_wakeup_level);
}

void show_setup(Canvas &canvas, const SalahSettings *existing_settings)
{
    char ap_ssid[32] = {};
    const esp_err_t ap_result = wifi_service_start_setup_ap(
        ap_ssid, sizeof(ap_ssid));
    if (ap_result != ESP_OK) {
        prayer_page_render_error(canvas,
                                 "SETUP UNAVAILABLE",
                                 "RESTART THE DEVICE");
        sticky_display_refresh_monochrome();
        enter_deep_sleep(kErrorRetryUs);
    }

    canvas.set_rotation(salah_default_rotation());
    sticky_display_set_battery_overlay_enabled(false);
    prayer_page_render_setup(canvas,
                             ap_ssid,
                             kStickySalahSetupPassword);
    sticky_display_refresh_monochrome();

    SalahSettings saved = {};
    const esp_err_t portal_result = provisioning_portal_run(
        existing_settings, saved);
    if (portal_result != ESP_OK) {
        prayer_page_render_error(canvas,
                                 "SETUP UNAVAILABLE",
                                 "RESTART THE DEVICE");
        sticky_display_refresh_monochrome();
        enter_deep_sleep(kErrorRetryUs);
    }
    wifi_service_stop();
    vTaskDelay(pdMS_TO_TICKS(250));
    esp_restart();
    while (true) {
        vTaskDelay(portMAX_DELAY);
    }
}

StickyVerticalSwipe classify_swipe(Canvas &canvas,
                                   uint16_t start_x,
                                   uint16_t start_y,
                                   uint16_t end_x,
                                   uint16_t end_y,
                                   uint32_t duration_ms)
{
    int logical_start_x = 0;
    int logical_start_y = 0;
    int logical_end_x = 0;
    int logical_end_y = 0;
    canvas.physical_to_logical(start_x, start_y,
                               logical_start_x, logical_start_y);
    canvas.physical_to_logical(end_x, end_y,
                               logical_end_x, logical_end_y);
    return sticky_touch_classify_vertical_swipe(
        {logical_start_x, logical_start_y,
         logical_end_x, logical_end_y, duration_ms},
        canvas.width(), canvas.height());
}

bool wake_gesture_requests_settings(Canvas &canvas)
{
    StickyTouchWakeGesture gesture = {};
    if (!sticky_touch_take_wake_gesture(gesture)) {
        return false;
    }
    const bool directional_swipe = gesture.id == 0xAAU ||
                                   gesture.id == 0xBBU ||
                                   gesture.id == 0xABU ||
                                   gesture.id == 0xBAU;
    const bool requested = directional_swipe &&
        classify_swipe(canvas,
                       gesture.start_x, gesture.start_y,
                       gesture.end_x, gesture.end_y, 0U) ==
            StickyVerticalSwipe::UpFromBottom;
    STICKY_LOGI(kTag,
                "settings=wake_gesture id=0x%02X requested=%d",
                static_cast<unsigned>(gesture.id), requested);
    return requested;
}

bool settings_requested_by_swipe(Canvas &canvas)
{
    const esp_err_t touch_result = sticky_touch_init();
    if (touch_result != ESP_OK) {
        STICKY_LOGW(kTag,
                    "settings=touch_input result=%s",
                    esp_err_to_name(touch_result));
        return false;
    }
    if (wake_gesture_requests_settings(canvas)) {
        return true;
    }
    sticky_touch_clear_interaction();
    const TickType_t started = xTaskGetTickCount();
    while (xTaskGetTickCount() - started < kInputWindow) {
        StickyTouchInteraction interaction = {};
        if (sticky_touch_take_interaction(interaction)) {
            const StickyVerticalSwipe swipe = classify_swipe(
                canvas,
                interaction.start_x, interaction.start_y,
                interaction.end_x, interaction.end_y,
                interaction.ended_at_ms - interaction.started_at_ms);
            if (swipe == StickyVerticalSwipe::UpFromBottom) {
                STICKY_LOGI(kTag,
                            "settings=swipe direction=up_from_bottom result=open");
                return true;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
    return false;
}

[[noreturn]] void restart_after_settings(Canvas &canvas,
                                         SalahSettings &settings,
                                         bool first_time)
{
    const DeviceSettingsResult result = device_settings_run(
        canvas, settings, first_time);
    if (result == DeviceSettingsResult::WifiRequested) {
        show_setup(canvas, &settings);
    }
    if (result == DeviceSettingsResult::Failed) {
        // The settings flow already presents the specific hardware error.
        // 设置流程已经显示具体的硬件错误，此处直接等待自动重试。
        enter_deep_sleep(kErrorRetryUs);
    }
    vTaskDelay(pdMS_TO_TICKS(250));
    esp_restart();
    while (true) {
        vTaskDelay(portMAX_DELAY);
    }
}

bool calculate_local_schedule(const SalahSettings &settings,
                              const PrayerDate &today,
                              PrayerSchedule &schedule)
{
    prayer_schedule_clear(schedule);
    const PrayerDate dates[] = {
        prayer_date_previous(today), today, prayer_date_next(today)};
    for (const PrayerDate &date : dates) {
        PrayerDay day = {};
        if (!prayer_calculate_day(settings, date, day) ||
            !prayer_schedule_add(schedule, day)) {
            return false;
        }
    }
    return true;
}

bool refresh_network_time(const SalahSettings &settings,
                          StickyRtcDateTime &now,
                          bool &online)
{
    online = false;
    if (wifi_service_connect(settings, kNetworkTimeoutMs) != ESP_OK) {
        return false;
    }
    online = true;
    const bool synced = time_sync_from_network(
                            settings.utc_offset_minutes,
                            now,
                            kTimeSyncTimeoutMs) == ESP_OK;
    if (synced) {
        s_last_time_sync_date = date_key(now);
    }
    wifi_service_stop();
    return synced;
}

uint64_t sleep_interval_us(const PrayerNext &next,
                           const StickyRtcDateTime &now)
{
    const uint32_t interval = prayer_refresh_interval_seconds(
        next.minutes_until);
    const uint32_t sleep_seconds = std::max<uint32_t>(
        15U, interval > now.second ? interval - now.second : interval);
    return static_cast<uint64_t>(sleep_seconds) * 1000000ULL;
}

}  // namespace

[[noreturn]] void prayer_app_run(Canvas &canvas)
{
    app_log_register_tag(kTag);
    if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_UNDEFINED) {
        s_last_rotation = static_cast<uint8_t>(salah_default_rotation());
    }
    SalahSettings settings = {};
    if (settings_store_load(settings) != ESP_OK) {
        show_setup(canvas, nullptr);
    }
    if (!settings_wifi_valid(settings)) {
        show_setup(canvas, nullptr);
    }
    if (!settings.configured) {
        restart_after_settings(canvas, settings, true);
    }

    canvas.set_rotation(static_cast<CanvasRotation>(s_last_rotation));
    if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0 &&
        sticky_touch_init() == ESP_OK &&
        wake_gesture_requests_settings(canvas)) {
        restart_after_settings(canvas, settings, false);
    }

    StickyRtcDateTime now = {};
    const bool rtc_ready = current_time(now);
    const bool woke_from_deep_sleep =
        esp_sleep_get_wakeup_cause() != ESP_SLEEP_WAKEUP_UNDEFINED;
    const bool daily_sync_due = rtc_ready &&
                                s_last_time_sync_date != date_key(now);

    bool online = false;
    if (!rtc_ready || !woke_from_deep_sleep || daily_sync_due) {
        refresh_network_time(settings, now, online);
    }

    const PrayerDate today_date = prayer_date(now);
    PrayerSchedule schedule = {};
    const bool schedule_ready = calculate_local_schedule(
        settings, today_date, schedule);
    const PrayerDay *yesterday = prayer_schedule_find(
        schedule, prayer_date_previous(today_date));
    const PrayerDay *today = prayer_schedule_find(schedule, today_date);
    const PrayerDay *tomorrow = prayer_schedule_find(
        schedule, prayer_date_next(today_date));
    PrayerNext next = {};
    const uint16_t current_minutes = static_cast<uint16_t>(
        now.hour * 60U + now.minute);
    PrayerWindow window = {};
    if (!schedule_ready || today == nullptr ||
        !prayer_find_next(*today, tomorrow, current_minutes, next) ||
        !prayer_find_window(
            yesterday, *today, tomorrow, current_minutes, window)) {
        canvas.set_rotation(detect_rotation());
        prayer_page_render_error(canvas,
                                 "SCHEDULE UNAVAILABLE",
                                 "CHECK LOCATION AND PRAYER SETTINGS");
        sticky_display_refresh_monochrome();
        if (settings_requested_by_swipe(canvas)) {
            restart_after_settings(canvas, settings, false);
        }
        enter_deep_sleep(kErrorRetryUs);
    }

    canvas.set_rotation(detect_rotation());
    sticky_display_set_battery_overlay_enabled(true);
    PrayerPageModel model = {};
    model.settings = &settings;
    model.today = today;
    model.yesterday = yesterday;
    model.tomorrow = tomorrow;
    model.next = next;
    model.window = window;
    model.current_minutes = current_minutes;
    model.online = online;
    prayer_page_render_schedule(canvas, model);
    sticky_display_refresh_monochrome();

    if (next.minutes_until == 0U && settings.sound_enabled) {
        sticky_buzzer_play_prayer_chime();
        vTaskDelay(pdMS_TO_TICKS(700));
    }
    if (settings_requested_by_swipe(canvas)) {
        restart_after_settings(canvas, settings, false);
    }
    const uint64_t sleep_us = sleep_interval_us(next, now);
    STICKY_LOGI(kTag,
                "cycle=complete prayer=%s minutes_until=%u online=%d sleep_s=%llu",
                prayer_id_name(next.prayer),
                static_cast<unsigned>(next.minutes_until),
                online,
                static_cast<unsigned long long>(sleep_us / 1000000ULL));
    enter_deep_sleep(sleep_us);
}
