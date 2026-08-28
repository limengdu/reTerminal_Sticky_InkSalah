#include "prayer_app.h"

#include <algorithm>
#include <cstdint>

#include "aladhan_client.h"
#include "app_log.h"
#include "board_power.h"
#include "canvas.h"
#include "display_orientation.h"
#include "esp_attr.h"
#include "esp_sleep.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "prayer_cache.h"
#include "prayer_pages.h"
#include "provisioning_portal.h"
#include "settings_store.h"
#include "sticky_button.h"
#include "sticky_buzzer.h"
#include "sticky_display.h"
#include "sticky_imu.h"
#include "sticky_rtc.h"
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

void show_setup(Canvas &canvas)
{
    char ap_ssid[32] = {};
    const esp_err_t ap_result = wifi_service_start_setup_ap(
        ap_ssid, sizeof(ap_ssid));
    if (ap_result != ESP_OK) {
        prayer_page_render_error(canvas,
                                 "SETUP UNAVAILABLE",
                                 "RESTART THE DEVICE");
        sticky_display_refresh_monochrome();
        board_power_enter_deep_sleep(kErrorRetryUs);
    }

    canvas.set_rotation(salah_default_rotation());
    sticky_display_set_battery_overlay_enabled(false);
    prayer_page_render_setup(canvas,
                             ap_ssid,
                             kStickySalahSetupPassword);
    sticky_display_refresh_monochrome();

    SalahSettings saved = {};
    const esp_err_t portal_result = provisioning_portal_run(saved);
    if (portal_result != ESP_OK) {
        prayer_page_render_error(canvas,
                                 "SETUP UNAVAILABLE",
                                 "RESTART THE DEVICE");
        sticky_display_refresh_monochrome();
        board_power_enter_deep_sleep(kErrorRetryUs);
    }
    wifi_service_stop();
    vTaskDelay(pdMS_TO_TICKS(250));
    esp_restart();
    while (true) {
        vTaskDelay(portMAX_DELAY);
    }
}

bool setup_requested()
{
    const TickType_t started = xTaskGetTickCount();
    while (xTaskGetTickCount() - started < kInputWindow) {
        StickyButtonEvent event = StickyButtonEvent::None;
        if (sticky_button_take_event(event) &&
            event == StickyButtonEvent::DoubleClick) {
            return true;
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
    return false;
}

bool needs_schedule_refresh(const PrayerSchedule &schedule,
                            const PrayerDate &today)
{
    return !prayer_schedule_contains(schedule, today) ||
           !prayer_schedule_contains(schedule, prayer_date_next(today));
}

bool refresh_network_data(const SalahSettings &settings,
                          StickyRtcDateTime &now,
                          PrayerSchedule &schedule,
                          bool schedule_missing,
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
    const PrayerDate local_date = prayer_date(now);
    if (schedule_missing || needs_schedule_refresh(schedule, local_date)) {
        PrayerSchedule downloaded = {};
        const esp_err_t fetch_result = aladhan_fetch_two_month_schedule(
            settings, local_date, downloaded);
        if (fetch_result == ESP_OK && prayer_cache_save(downloaded) == ESP_OK) {
            schedule = downloaded;
        }
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
        show_setup(canvas);
    }

    PrayerSchedule schedule = {};
    const bool cache_loaded = prayer_cache_load(schedule) == ESP_OK;
    StickyRtcDateTime now = {};
    const bool rtc_ready = current_time(now);
    const PrayerDate initial_date = prayer_date(now);
    const bool schedule_missing = !cache_loaded || !rtc_ready ||
                                  needs_schedule_refresh(schedule, initial_date);
    const bool woke_from_deep_sleep =
        esp_sleep_get_wakeup_cause() != ESP_SLEEP_WAKEUP_UNDEFINED;
    const bool daily_sync_due = rtc_ready &&
                                s_last_time_sync_date != date_key(now);

    bool online = false;
    if (schedule_missing || !woke_from_deep_sleep || daily_sync_due) {
        refresh_network_data(settings,
                             now,
                             schedule,
                             schedule_missing,
                             online);
    }

    const PrayerDate today_date = prayer_date(now);
    const PrayerDay *today = prayer_schedule_find(schedule, today_date);
    const PrayerDay *tomorrow = prayer_schedule_find(
        schedule, prayer_date_next(today_date));
    PrayerNext next = {};
    const uint16_t current_minutes = static_cast<uint16_t>(
        now.hour * 60U + now.minute);
    if (today == nullptr ||
        !prayer_find_next(*today, tomorrow, current_minutes, next)) {
        canvas.set_rotation(detect_rotation());
        prayer_page_render_error(canvas,
                                 "SCHEDULE UNAVAILABLE",
                                 "CONNECT WI-FI TO REFRESH TIMES");
        sticky_display_refresh_monochrome();
        if (setup_requested()) {
            show_setup(canvas);
        }
        sticky_display_sleep();
        board_power_enter_deep_sleep(kErrorRetryUs);
    }

    canvas.set_rotation(detect_rotation());
    sticky_display_set_battery_overlay_enabled(true);
    PrayerPageModel model = {};
    model.settings = &settings;
    model.today = today;
    model.tomorrow = tomorrow;
    model.next = next;
    model.current_minutes = current_minutes;
    model.online = online;
    prayer_page_render_schedule(canvas, model);
    sticky_display_refresh_monochrome();

    if (next.minutes_until == 0U && settings.sound_enabled) {
        sticky_buzzer_play_prayer_chime();
        vTaskDelay(pdMS_TO_TICKS(700));
    }
    if (setup_requested()) {
        show_setup(canvas);
    }
    const uint64_t sleep_us = sleep_interval_us(next, now);
    STICKY_LOGI(kTag,
                "cycle=complete prayer=%s minutes_until=%u online=%d sleep_s=%llu",
                prayer_id_name(next.prayer),
                static_cast<unsigned>(next.minutes_until),
                online,
                static_cast<unsigned long long>(sleep_us / 1000000ULL));
    sticky_display_sleep();
    board_power_enter_deep_sleep(sleep_us);
}
