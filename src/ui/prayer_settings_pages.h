#pragma once

#include <cstdint>

#include "canvas.h"
#include "settings_model.h"

enum class DeviceSettingsPage : uint8_t {
    Main,
    Location,
    Calculation,
    MethodList,
    CustomMethod,
    Adjustments,
    LatitudeEditor,
    LongitudeEditor,
    CustomFajrEditor,
    CustomIshaEditor,
};

enum class DeviceSettingsActionType : uint8_t {
    None,
    OpenLocation,
    OpenCalculation,
    OpenAdjustments,
    ToggleReminder,
    OpenWifi,
    Save,
    Cancel,
    Back,
    DetectLocation,
    LatitudeMinus,
    LatitudePlus,
    LongitudeMinus,
    LongitudePlus,
    OffsetMinus15,
    OffsetPlus15,
    OffsetMinusHour,
    OffsetPlusHour,
    EditLatitude,
    EditLongitude,
    SelectStandard,
    SelectHanafi,
    SelectMethod,
    PreviousMethodPage,
    NextMethodPage,
    EditCustomFajr,
    EditCustomIsha,
    FinishCustomMethod,
    AdjustmentMinus,
    AdjustmentPlus,
    EditorDigit,
    EditorDoubleZero,
    EditorSign,
    EditorDot,
    EditorClear,
    EditorBackspace,
    EditorConfirm,
};

struct DeviceSettingsAction {
    DeviceSettingsActionType type = DeviceSettingsActionType::None;
    int value = 0;
};

struct DeviceSettingsUiState {
    DeviceSettingsPage page = DeviceSettingsPage::Main;
    bool first_time = false;
    uint8_t method_page = 0U;
    char editor_value[20] = {};
    char status[72] = {};
};

// Maps one portrait touch coordinate to a settings action.
// 将一个竖屏触摸坐标映射为设置操作。
DeviceSettingsAction prayer_settings_action_at(DeviceSettingsPage page,
                                                int x,
                                                int y,
                                                uint8_t method_page = 0U);

// Returns the parent page for one in-settings back gesture.
// 返回设置内一次返回操作对应的上一级页面。
DeviceSettingsPage prayer_settings_parent_page(DeviceSettingsPage page);

// Draws one complete native settings page into the caller-owned canvas.
// 将一张完整的设备原生设置页绘制到调用方画布。
void prayer_settings_render(Canvas &canvas,
                            const SalahSettings &settings,
                            const DeviceSettingsUiState &state);

void prayer_settings_render_loading(Canvas &canvas,
                                    const char *title,
                                    const char *message);
