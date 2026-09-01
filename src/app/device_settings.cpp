#include "device_settings.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "app_log.h"
#include "display_orientation.h"
#include "location_catalog.h"
#include "network_location.h"
#include "prayer_calculator.h"
#include "prayer_settings_pages.h"
#include "settings_store.h"
#include "sticky_display.h"
#include "sticky_rtc.h"
#include "sticky_touch.h"
#include "sticky_touch_gesture.h"
#include "time_sync.h"
#include "wifi_service.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace {

constexpr char kTag[] = "device_settings";
constexpr uint32_t kConnectTimeoutMs = 16000U;
constexpr uint32_t kTimeSyncTimeoutMs = 12000U;

void set_status(DeviceSettingsUiState &state, const char *message)
{
    std::snprintf(state.status, sizeof(state.status), "%s",
                  message == nullptr ? "" : message);
}

void refresh(Canvas &canvas,
             const SalahSettings &settings,
             const DeviceSettingsUiState &state,
             bool full = false)
{
    prayer_settings_render(canvas, settings, state);
    if (!full) {
        sticky_display_prepare_app_transition_refresh();
    }
    sticky_display_refresh_monochrome();
}

void copy_text(char *destination, size_t capacity, const char *source)
{
    if (destination == nullptr || capacity == 0U) {
        return;
    }
    std::snprintf(destination, capacity, "%s", source == nullptr ? "" : source);
}

bool detect_location(SalahSettings &settings,
                     DeviceSettingsUiState &state,
                     Canvas &canvas)
{
    prayer_settings_render_loading(canvas, "DETECTING LOCATION",
                                   "USING YOUR WI-FI CONNECTION");
    sticky_display_prepare_app_transition_refresh();
    sticky_display_refresh_monochrome();

    if (wifi_service_connect(settings, kConnectTimeoutMs) != ESP_OK) {
        wifi_service_stop();
        set_status(state, "WI-FI CONNECTION FAILED");
        return false;
    }
    NetworkLocationEstimate estimate = {};
    const esp_err_t location_result = network_location_estimate(estimate);
    if (location_result != ESP_OK || !estimate.valid) {
        wifi_service_stop();
        set_status(state, "LOCATION LOOKUP FAILED; ENTER IT MANUALLY");
        return false;
    }

    copy_text(settings.city, sizeof(settings.city), estimate.city);
    copy_text(settings.country, sizeof(settings.country), estimate.country);
    settings.latitude = estimate.latitude;
    settings.longitude = estimate.longitude;
    settings.utc_offset_minutes = estimate.utc_offset_minutes;
    settings.location_confirmed = true;
    const LocationCountry *country = location_country_by_name(estimate.country);
    if (country != nullptr) {
        settings.calculation_method = country->recommended_method;
        settings.asr_school = country->recommended_asr_school;
    }

    StickyRtcDateTime local_time = {};
    const esp_err_t time_result = time_sync_from_network(
        settings.utc_offset_minutes, local_time, kTimeSyncTimeoutMs);
    wifi_service_stop();
    set_status(state, time_result == ESP_OK
                          ? "LOCATION AND TIME UPDATED"
                          : "LOCATION UPDATED; TIME SYNC WILL RETRY");
    STICKY_LOGI(kTag,
                "location=detected city=%s country=%s latitude=%.4f longitude=%.4f offset=%d result=ok",
                settings.city, settings.country, settings.latitude,
                settings.longitude,
                static_cast<int>(settings.utc_offset_minutes));
    return true;
}

void begin_editor(DeviceSettingsUiState &state,
                  DeviceSettingsPage page,
                  double value)
{
    state.page = page;
    std::snprintf(state.editor_value, sizeof(state.editor_value), "%.4f", value);
    set_status(state, "");
}

bool append_editor(DeviceSettingsUiState &state, char value)
{
    const size_t length = std::strlen(state.editor_value);
    if (length + 1U >= sizeof(state.editor_value)) {
        return false;
    }
    state.editor_value[length] = value;
    state.editor_value[length + 1U] = '\0';
    return true;
}

bool confirm_editor(SalahSettings &settings, DeviceSettingsUiState &state)
{
    char *end = nullptr;
    const double value = std::strtod(state.editor_value, &end);
    const bool latitude = state.page == DeviceSettingsPage::LatitudeEditor;
    const double minimum = latitude ? -90.0 : -180.0;
    const double maximum = latitude ? 90.0 : 180.0;
    if (state.editor_value == end || end == nullptr || *end != '\0' ||
        !std::isfinite(value) || value < minimum || value > maximum) {
        set_status(state, "VALUE IS OUTSIDE THE VALID RANGE");
        return false;
    }
    if (latitude) {
        settings.latitude = value;
    } else {
        settings.longitude = value;
    }
    copy_text(settings.city, sizeof(settings.city), "Manual");
    copy_text(settings.country, sizeof(settings.country), "Coordinates");
    settings.location_confirmed = true;
    state.page = DeviceSettingsPage::Location;
    state.editor_value[0] = '\0';
    set_status(state, "MANUAL COORDINATES APPLIED");
    return true;
}

bool validate_and_save(SalahSettings &settings,
                       DeviceSettingsUiState &state)
{
    if (!settings.location_confirmed) {
        set_status(state, "DETECT OR ENTER YOUR LOCATION FIRST");
        return false;
    }
    SalahSettings candidate = settings;
    candidate.configured = true;
    if (!settings_store_valid(candidate)) {
        set_status(state, "ONE OR MORE SETTINGS ARE INVALID");
        return false;
    }
    StickyRtcDateTime now = {};
    if (sticky_rtc_read(now) != ESP_OK &&
        sticky_rtc_seed_from_build_time(now) != ESP_OK) {
        set_status(state, "CLOCK IS NOT READY");
        return false;
    }
    PrayerDay day = {};
    if (!prayer_calculate_day(candidate,
                              {now.year, now.month, now.day}, day)) {
        set_status(state, "PRAYER TIMES CANNOT BE CALCULATED HERE");
        return false;
    }
    if (settings_store_save(candidate) != ESP_OK) {
        set_status(state, "SETTINGS COULD NOT BE SAVED");
        return false;
    }
    settings = candidate;
    return true;
}

bool clamp_offset(SalahSettings &settings, int delta)
{
    const int next = static_cast<int>(settings.utc_offset_minutes) + delta;
    const int clamped = std::max(-12 * 60, std::min(14 * 60, next));
    if (clamped == settings.utc_offset_minutes) {
        return false;
    }
    settings.utc_offset_minutes = static_cast<int16_t>(clamped);
    return true;
}

void mark_manual_location(SalahSettings &settings)
{
    copy_text(settings.city, sizeof(settings.city), "Manual");
    copy_text(settings.country, sizeof(settings.country), "Coordinates");
    settings.location_confirmed = true;
}

StickyVerticalSwipe classify_swipe(Canvas &canvas,
                                   const StickyTouchInteraction &interaction)
{
    int logical_start_x = 0;
    int logical_start_y = 0;
    int logical_end_x = 0;
    int logical_end_y = 0;
    canvas.physical_to_logical(interaction.start_x,
                               interaction.start_y,
                               logical_start_x,
                               logical_start_y);
    canvas.physical_to_logical(interaction.end_x,
                               interaction.end_y,
                               logical_end_x,
                               logical_end_y);
    return sticky_touch_classify_vertical_swipe(
        {logical_start_x, logical_start_y,
         logical_end_x, logical_end_y,
         interaction.ended_at_ms - interaction.started_at_ms},
        canvas.width(), canvas.height());
}

}  // namespace

DeviceSettingsResult device_settings_run(Canvas &canvas,
                                         SalahSettings &settings,
                                         bool first_time)
{
    app_log_register_tag(kTag);
    // Restores the persisted snapshot when the user cancels or opens Wi-Fi.
    // 用户取消或转入Wi-Fi配网时，恢复进入菜单前的持久化快照。
    const SalahSettings original = settings;
    canvas.set_rotation(salah_default_rotation());
    sticky_display_set_battery_overlay_enabled(false);
    DeviceSettingsUiState state = {};
    state.first_time = first_time;

    if (sticky_touch_init() != ESP_OK) {
        prayer_settings_render_loading(canvas, "TOUCH UNAVAILABLE",
                                       "RESTART THE DEVICE");
        sticky_display_refresh_monochrome();
        return DeviceSettingsResult::Failed;
    }
    sticky_touch_clear_press();
    sticky_touch_clear_interaction();
    if (first_time && !settings.location_confirmed) {
        // First-time setup proposes an IP-based location for touch review.
        // 首次设置先给出公网IP估算地点，再交给用户在触摸屏上核对。
        detect_location(settings, state, canvas);
    }
    sticky_touch_clear_press();
    state.page = DeviceSettingsPage::Main;
    refresh(canvas, settings, state, true);

    // Keeps the native menu open until the user chooses a terminal action.
    // 原生设置菜单保持打开，直到用户主动选择保存、取消或配网。
    while (true) {
        StickyTouchInteraction interaction = {};
        if (sticky_touch_take_interaction(interaction) &&
            classify_swipe(canvas, interaction) ==
                StickyVerticalSwipe::DownFromTop) {
            if (validate_and_save(settings, state)) {
                STICKY_LOGI(kTag,
                            "settings=swipe direction=down_from_top result=save_and_close");
                return DeviceSettingsResult::Saved;
            }
            refresh(canvas, settings, state);
            continue;
        }
        StickyTouchPress press = {};
        if (!sticky_touch_take_press(press)) {
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }
        int x = 0;
        int y = 0;
        canvas.physical_to_logical(press.x, press.y, x, y);
        const DeviceSettingsAction action = prayer_settings_action_at(
            state.page, x, y);
        if (action.type == DeviceSettingsActionType::None) {
            continue;
        }
        set_status(state, "");
        switch (action.type) {
        case DeviceSettingsActionType::OpenLocation:
            state.page = DeviceSettingsPage::Location;
            break;
        case DeviceSettingsActionType::OpenCalculation:
            state.page = DeviceSettingsPage::Calculation;
            break;
        case DeviceSettingsActionType::OpenAdjustments:
            state.page = DeviceSettingsPage::Adjustments;
            break;
        case DeviceSettingsActionType::ToggleReminder:
            settings.sound_enabled = !settings.sound_enabled;
            break;
        case DeviceSettingsActionType::OpenWifi:
            settings = original;
            sticky_touch_stop();
            return DeviceSettingsResult::WifiRequested;
        case DeviceSettingsActionType::Save:
            if (validate_and_save(settings, state)) {
                return DeviceSettingsResult::Saved;
            }
            break;
        case DeviceSettingsActionType::Cancel:
            if (!first_time) {
                settings = original;
                return DeviceSettingsResult::Cancelled;
            }
            break;
        case DeviceSettingsActionType::Back:
            if (state.page == DeviceSettingsPage::MethodList) {
                state.page = DeviceSettingsPage::Calculation;
            } else {
                state.page = DeviceSettingsPage::Main;
            }
            break;
        case DeviceSettingsActionType::DetectLocation:
            detect_location(settings, state, canvas);
            state.page = DeviceSettingsPage::Location;
            break;
        case DeviceSettingsActionType::LatitudeMinus:
            settings.latitude = std::max(-90.0, settings.latitude - 0.1);
            mark_manual_location(settings);
            break;
        case DeviceSettingsActionType::LatitudePlus:
            settings.latitude = std::min(90.0, settings.latitude + 0.1);
            mark_manual_location(settings);
            break;
        case DeviceSettingsActionType::LongitudeMinus:
            settings.longitude = std::max(-180.0, settings.longitude - 0.1);
            mark_manual_location(settings);
            break;
        case DeviceSettingsActionType::LongitudePlus:
            settings.longitude = std::min(180.0, settings.longitude + 0.1);
            mark_manual_location(settings);
            break;
        case DeviceSettingsActionType::OffsetMinus15:
            clamp_offset(settings, -15);
            break;
        case DeviceSettingsActionType::OffsetPlus15:
            clamp_offset(settings, 15);
            break;
        case DeviceSettingsActionType::OffsetMinusHour:
            clamp_offset(settings, -60);
            break;
        case DeviceSettingsActionType::OffsetPlusHour:
            clamp_offset(settings, 60);
            break;
        case DeviceSettingsActionType::EditLatitude:
            begin_editor(state, DeviceSettingsPage::LatitudeEditor,
                         settings.latitude);
            break;
        case DeviceSettingsActionType::EditLongitude:
            begin_editor(state, DeviceSettingsPage::LongitudeEditor,
                         settings.longitude);
            break;
        case DeviceSettingsActionType::SelectStandard:
            settings.asr_school = 0U;
            break;
        case DeviceSettingsActionType::SelectHanafi:
            settings.asr_school = 1U;
            break;
        case DeviceSettingsActionType::SelectMethod:
            if (action.value < 0) {
                state.page = DeviceSettingsPage::MethodList;
            } else {
                settings.calculation_method = static_cast<uint8_t>(action.value);
                state.page = DeviceSettingsPage::Calculation;
            }
            break;
        case DeviceSettingsActionType::AdjustmentMinus:
        case DeviceSettingsActionType::AdjustmentPlus: {
            const size_t index = static_cast<size_t>(action.value);
            const int delta = action.type == DeviceSettingsActionType::AdjustmentPlus ? 1 : -1;
            const int value = settings.prayer_adjustments[index] + delta;
            settings.prayer_adjustments[index] = static_cast<int8_t>(
                std::max(-30, std::min(30, value)));
            break;
        }
        case DeviceSettingsActionType::EditorDigit:
            append_editor(state, static_cast<char>('0' + action.value));
            break;
        case DeviceSettingsActionType::EditorSign:
            if (state.editor_value[0] == '-') {
                std::memmove(state.editor_value, state.editor_value + 1,
                             std::strlen(state.editor_value));
            } else if (std::strlen(state.editor_value) + 1U < sizeof(state.editor_value)) {
                std::memmove(state.editor_value + 1, state.editor_value,
                             std::strlen(state.editor_value) + 1U);
                state.editor_value[0] = '-';
            }
            break;
        case DeviceSettingsActionType::EditorDot:
            if (std::strchr(state.editor_value, '.') == nullptr) {
                append_editor(state, '.');
            }
            break;
        case DeviceSettingsActionType::EditorBackspace: {
            const size_t length = std::strlen(state.editor_value);
            if (length > 0U) state.editor_value[length - 1U] = '\0';
            break;
        }
        case DeviceSettingsActionType::EditorCancel:
            state.page = DeviceSettingsPage::Location;
            state.editor_value[0] = '\0';
            break;
        case DeviceSettingsActionType::EditorConfirm:
            confirm_editor(settings, state);
            break;
        case DeviceSettingsActionType::None:
        default:
            break;
        }
        refresh(canvas, settings, state);
    }
}
