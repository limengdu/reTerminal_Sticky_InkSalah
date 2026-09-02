#include "prayer_app.h"

#include <algorithm>
#include <cstdint>

#include "app_log.h"
#include "board_charger.h"
#include "board_power.h"
#include "canvas.h"
#include "device_settings.h"
#include "display_orientation.h"
#include "esp_attr.h"
#include "esp_sleep.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "prayer_calculator.h"
#include "prayer_input_policy.h"
#include "prayer_schedule.h"
#include "prayer_pages.h"
#include "provisioning_portal.h"
#include "settings_store.h"
#include "sleep_indicator.h"
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
constexpr TickType_t kOrientationSettle = pdMS_TO_TICKS(650);
constexpr uint64_t kErrorRetryUs = 5ULL * 60ULL * 1000000ULL;
constexpr uint32_t kPoweredErrorRefreshMs = 60000U;
constexpr uint32_t kTransitionCleanupQuietMs = 1200U;

RTC_DATA_ATTR uint32_t s_last_time_sync_date = 0U;
RTC_DATA_ATTR uint8_t s_last_rotation =
    static_cast<uint8_t>(CanvasRotation::Deg90CounterClockwise);

uint32_t monotonic_ms()
{
    return static_cast<uint32_t>(esp_timer_get_time() / 1000LL);
}

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

[[noreturn]] void enter_deep_sleep(Canvas &canvas,
                                   uint64_t timer_wakeup_us)
{
    const esp_err_t imu_result =
        sticky_imu_prepare_orientation_wakeup();
    const bool orientation_wakeup_ready = imu_result == ESP_OK;
    if (!orientation_wakeup_ready) {
        STICKY_LOGW(kTag,
                    "sleep=orientation_wakeup state=disabled result=%s",
                    esp_err_to_name(imu_result));
    }

    const esp_err_t touch_result = sticky_touch_stop();
    if (touch_result != ESP_OK) {
        STICKY_LOGW(kTag,
                    "sleep=touch state=stop_failed result=%s",
                    esp_err_to_name(touch_result));
    }

    sleep_indicator_render(canvas);
    sticky_display_cancel_app_transition_refresh();
    const esp_err_t indicator_result =
        sticky_display_refresh_monochrome();
    if (indicator_result != ESP_OK) {
        STICKY_LOGW(kTag,
                    "sleep=indicator position=top_left result=%s",
                    esp_err_to_name(indicator_result));
    } else {
        STICKY_LOGI(kTag,
                    "sleep=indicator position=top_left result=ok");
    }

    const esp_err_t display_result = sticky_display_sleep();
    if (display_result != ESP_OK) {
        STICKY_LOGW(kTag,
                    "sleep=display state=power_down_failed result=%s",
                    esp_err_to_name(display_result));
    }
    board_power_enter_deep_sleep(timer_wakeup_us,
                                 orientation_wakeup_ready,
                                 false,
                                 0U);
}

struct SetupCancelContext {
    Canvas *canvas = nullptr;
};

// Converts setup-page taps and left-edge swipes into one portal cancel signal.
// 将配网页面的按钮点击和左侧滑动统一转换为门户取消信号。
bool setup_cancel_requested(void *context)
{
    auto *cancel = static_cast<SetupCancelContext *>(context);
    if (cancel == nullptr || cancel->canvas == nullptr) {
        return false;
    }
    StickyTouchInteraction interaction = {};
    if (sticky_touch_take_interaction(interaction)) {
        int start_x = 0;
        int start_y = 0;
        int end_x = 0;
        int end_y = 0;
        cancel->canvas->physical_to_logical(
            interaction.start_x, interaction.start_y, start_x, start_y);
        cancel->canvas->physical_to_logical(
            interaction.end_x, interaction.end_y, end_x, end_y);
        const StickyLogicalTouchPath path = {
            start_x,
            start_y,
            end_x,
            end_y,
            interaction.ended_at_ms - interaction.started_at_ms,
        };
        if (sticky_touch_classify_horizontal_swipe(
                path, cancel->canvas->width(), cancel->canvas->height()) ==
            StickyHorizontalSwipe::RightFromLeft) {
            return true;
        }
    }
    StickyTouchPress press = {};
    if (!sticky_touch_take_press(press)) {
        return false;
    }
    int x = 0;
    int y = 0;
    cancel->canvas->physical_to_logical(press.x, press.y, x, y);
    return prayer_page_setup_back_action_at(x, y, true);
}

bool show_setup(Canvas &canvas, const SalahSettings *existing_settings)
{
    const bool can_return = existing_settings != nullptr &&
                            settings_wifi_valid(*existing_settings);
    char ap_ssid[32] = {};
    const esp_err_t ap_result = wifi_service_start_setup_ap(
        ap_ssid, sizeof(ap_ssid));
    if (ap_result != ESP_OK) {
        prayer_page_render_error(canvas,
                                 "SETUP UNAVAILABLE",
                                 "RESTART THE DEVICE");
        sticky_display_refresh_monochrome();
        enter_deep_sleep(canvas, kErrorRetryUs);
    }

    canvas.set_rotation(salah_default_rotation());
    sticky_display_set_battery_overlay_enabled(false);
    prayer_page_render_setup(canvas,
                             ap_ssid,
                             kStickySalahSetupPassword,
                             can_return);
    sticky_display_refresh_monochrome();

    SetupCancelContext cancel_context = {&canvas};
    if (can_return) {
        if (sticky_touch_init() != ESP_OK) {
            wifi_service_stop();
            return false;
        }
        sticky_touch_clear_press();
        sticky_touch_clear_interaction();
    }

    SalahSettings saved = {};
    ProvisioningPortalExit portal_exit = ProvisioningPortalExit::Cancelled;
    const esp_err_t portal_result = provisioning_portal_run(
        existing_settings,
        saved,
        portal_exit,
        can_return ? setup_cancel_requested : nullptr,
        can_return ? &cancel_context : nullptr);
    if (portal_result != ESP_OK) {
        prayer_page_render_error(canvas,
                                 "SETUP UNAVAILABLE",
                                 "RESTART THE DEVICE");
        sticky_display_refresh_monochrome();
        enter_deep_sleep(canvas, kErrorRetryUs);
    }
    wifi_service_stop();
    if (can_return) {
        sticky_touch_stop();
    }
    if (portal_exit == ProvisioningPortalExit::Cancelled) {
        sticky_display_prepare_app_transition_refresh();
        return false;
    }
    vTaskDelay(pdMS_TO_TICKS(250));
    esp_restart();
    while (true) {
        vTaskDelay(portMAX_DELAY);
    }
    return true;
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

enum class PrayerInputResult : uint8_t {
    SleepRequested,
    SettingsRequested,
    RefreshRequested,
};

PrayerInputResult wait_for_prayer_input(Canvas &canvas,
                                        uint32_t refresh_interval_ms,
                                        uint32_t &last_user_activity_ms)
{
    const esp_err_t touch_result = sticky_touch_init();
    if (touch_result != ESP_OK) {
        STICKY_LOGW(kTag,
                    "settings=touch_input result=%s",
                    esp_err_to_name(touch_result));
        return PrayerInputResult::SleepRequested;
    }
    if (wake_gesture_requests_settings(canvas)) {
        return PrayerInputResult::SettingsRequested;
    }
    const bool first_input_window = last_user_activity_ms == 0U;
    if (first_input_window) {
        sticky_touch_clear_interaction();
    }
    const bool imu_started = sticky_imu_start_monitoring() == ESP_OK;
    const uint32_t started_ms = monotonic_ms();
    if (first_input_window) {
        last_user_activity_ms = started_ms;
    }
    STICKY_LOGI(kTag,
                "input=active idle_timeout_ms=%u refresh_ms=%u touch=continuous imu=%s external_power=%d",
                static_cast<unsigned>(kPrayerInputIdleTimeoutMs),
                static_cast<unsigned>(refresh_interval_ms),
                imu_started ? "continuous" : "unavailable",
                board_charger_external_power_present());
    while (true) {
        const uint32_t touch_activity_ms =
            sticky_touch_last_activity_ms();
        uint32_t now_ms = monotonic_ms();
        if (static_cast<int32_t>(
                touch_activity_ms - last_user_activity_ms) > 0) {
            last_user_activity_ms = touch_activity_ms;
        }

        StickyTouchInteraction interaction = {};
        if (sticky_touch_take_interaction(interaction)) {
            now_ms = monotonic_ms();
            last_user_activity_ms = now_ms;
            const StickyVerticalSwipe swipe = classify_swipe(
                canvas,
                interaction.start_x, interaction.start_y,
                interaction.end_x, interaction.end_y,
                interaction.ended_at_ms - interaction.started_at_ms);
            if (swipe == StickyVerticalSwipe::UpFromBottom) {
                STICKY_LOGI(kTag,
                            "settings=swipe direction=up_from_bottom result=open");
                if (imu_started) {
                    sticky_imu_stop_monitoring();
                }
                return PrayerInputResult::SettingsRequested;
            }
            STICKY_LOGI(kTag,
                        "settings=swipe direction=unmatched start_x=%u start_y=%u end_x=%u end_y=%u duration_ms=%u rotation=%u result=ignored",
                        static_cast<unsigned>(interaction.start_x),
                        static_cast<unsigned>(interaction.start_y),
                        static_cast<unsigned>(interaction.end_x),
                        static_cast<unsigned>(interaction.end_y),
                        static_cast<unsigned>(
                            interaction.ended_at_ms -
                            interaction.started_at_ms),
                        static_cast<unsigned>(canvas.rotation()));
        }

        if (imu_started) {
            StickyImuState imu_state = {};
            if (sticky_imu_get_state(imu_state) == ESP_OK &&
                imu_state.valid) {
                if (imu_state.orientation ==
                    StickyImuOrientation::Unknown) {
                    vTaskDelay(pdMS_TO_TICKS(20));
                    continue;
                }
                const CanvasRotation next_rotation =
                    salah_rotation_from_orientation(
                        imu_state.orientation,
                        static_cast<CanvasRotation>(s_last_rotation));
                if (next_rotation !=
                    static_cast<CanvasRotation>(s_last_rotation)) {
                    s_last_rotation = static_cast<uint8_t>(next_rotation);
                    last_user_activity_ms = now_ms;
                    sticky_imu_stop_monitoring();
                    STICKY_LOGI(kTag,
                                "input=orientation state=changed rotation=%u result=refresh",
                                static_cast<unsigned>(s_last_rotation));
                    return PrayerInputResult::RefreshRequested;
                }
            }
        }

        now_ms = monotonic_ms();
        const uint32_t latest_touch_activity_ms =
            sticky_touch_last_activity_ms();
        if (sticky_display_transition_cleanup_pending() &&
            prayer_input_elapsed_ms(
                now_ms,
                std::max(started_ms, latest_touch_activity_ms)) >=
                kTransitionCleanupQuietMs) {
            sticky_display_refresh_transition_cleanup();
            continue;
        }
        const uint32_t refresh_elapsed_ms =
            prayer_input_elapsed_ms(now_ms, started_ms);
        const uint32_t idle_elapsed_ms =
            prayer_input_elapsed_ms(now_ms, last_user_activity_ms);
        const PrayerInputWaitDecision decision =
            prayer_input_wait_decision(
                refresh_elapsed_ms,
                idle_elapsed_ms,
                refresh_interval_ms);
        if (decision != PrayerInputWaitDecision::Continue) {
            if (imu_started) {
                sticky_imu_stop_monitoring();
            }
            if (decision == PrayerInputWaitDecision::Sleep) {
                STICKY_LOGI(kTag,
                            "input=inactive reason=idle_timeout idle_ms=%u result=sleep",
                            static_cast<unsigned>(idle_elapsed_ms));
                return PrayerInputResult::SleepRequested;
            }
            STICKY_LOGI(kTag,
                        "input=refresh reason=interval elapsed_ms=%u result=refresh",
                        static_cast<unsigned>(refresh_elapsed_ms));
            return PrayerInputResult::RefreshRequested;
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

bool run_settings(Canvas &canvas,
                  SalahSettings &settings,
                  bool first_time)
{
    const int16_t previous_utc_offset_minutes =
        settings.utc_offset_minutes;
    DeviceSettingsResult result = DeviceSettingsResult::Cancelled;
    do {
        result = device_settings_run(canvas, settings, first_time);
        if (result == DeviceSettingsResult::WifiRequested) {
            show_setup(canvas, &settings);
        }
    } while (result == DeviceSettingsResult::WifiRequested);
    if (result == DeviceSettingsResult::Failed) {
        // The settings flow already presents the specific hardware error.
        // 设置流程已经显示具体的硬件错误，此处直接等待自动重试。
        enter_deep_sleep(canvas, kErrorRetryUs);
    }
    const bool timezone_changed =
        result == DeviceSettingsResult::Saved &&
        previous_utc_offset_minutes != settings.utc_offset_minutes;
    STICKY_LOGI(kTag,
                "settings=closed result=%s mode=in_place timezone_changed=%d",
                result == DeviceSettingsResult::Saved ? "saved" : "cancelled",
                timezone_changed);
    if (result == DeviceSettingsResult::Saved ||
        result == DeviceSettingsResult::Cancelled) {
        sticky_display_prepare_app_transition_refresh();
    }
    return timezone_changed;
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
        if (run_settings(canvas, settings, true)) {
            s_last_time_sync_date = 0U;
        }
    }

    canvas.set_rotation(static_cast<CanvasRotation>(s_last_rotation));
    if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0 &&
        sticky_touch_init() == ESP_OK &&
        wake_gesture_requests_settings(canvas)) {
        if (run_settings(canvas, settings, false)) {
            s_last_time_sync_date = 0U;
        }
    }

    const bool woke_from_deep_sleep =
        esp_sleep_get_wakeup_cause() != ESP_SLEEP_WAKEUP_UNDEFINED;
    const bool woke_from_orientation =
        board_power_woke_from_orientation();
    bool first_cycle = true;
    uint32_t last_user_activity_ms = 0U;
    while (true) {
        const bool initial_cycle = first_cycle;
        StickyRtcDateTime now = {};
        const bool rtc_ready = current_time(now);
        const bool daily_sync_due = rtc_ready &&
                                    s_last_time_sync_date != date_key(now);

        bool online = false;
        if (!rtc_ready ||
            (first_cycle && !woke_from_deep_sleep) ||
            daily_sync_due) {
            refresh_network_time(settings, now, online);
        }
        first_cycle = false;

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
            const PrayerInputResult input_result = wait_for_prayer_input(
                canvas, kPoweredErrorRefreshMs, last_user_activity_ms);
            if (input_result == PrayerInputResult::SettingsRequested) {
                if (run_settings(canvas, settings, false)) {
                    s_last_time_sync_date = 0U;
                }
                last_user_activity_ms = monotonic_ms();
                continue;
            }
            if (input_result == PrayerInputResult::RefreshRequested) {
                continue;
            }
            enter_deep_sleep(canvas, kErrorRetryUs);
        }

        const CanvasRotation previous_rotation =
            static_cast<CanvasRotation>(s_last_rotation);
        const CanvasRotation detected_rotation = detect_rotation();
        if (initial_cycle && woke_from_orientation &&
            detected_rotation == previous_rotation) {
            const uint64_t retry_sleep_us = sleep_interval_us(next, now);
            STICKY_LOGI(kTag,
                        "input=orientation_wakeup state=unchanged rotation=%u result=resleep",
                        static_cast<unsigned>(detected_rotation));
            enter_deep_sleep(canvas, retry_sleep_us);
        }
        canvas.set_rotation(detected_rotation);
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
        const uint64_t sleep_us = sleep_interval_us(next, now);
        STICKY_LOGI(kTag,
                    "cycle=complete prayer=%s minutes_until=%u online=%d sleep_s=%llu",
                    prayer_id_name(next.prayer),
                    static_cast<unsigned>(next.minutes_until),
                    online,
                    static_cast<unsigned long long>(sleep_us / 1000000ULL));
        const PrayerInputResult input_result = wait_for_prayer_input(
            canvas,
            static_cast<uint32_t>(sleep_us / 1000ULL),
            last_user_activity_ms);
        if (input_result == PrayerInputResult::SettingsRequested) {
            if (run_settings(canvas, settings, false)) {
                s_last_time_sync_date = 0U;
            }
            last_user_activity_ms = monotonic_ms();
            continue;
        }
        if (input_result == PrayerInputResult::RefreshRequested) {
            continue;
        }
        enter_deep_sleep(canvas, sleep_us);
    }
}
