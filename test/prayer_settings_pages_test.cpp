#include <cassert>
#include <cstring>
#include <vector>

#include "canvas.h"
#include "prayer_settings_pages.h"

namespace {

size_t black_pixels(const Canvas &canvas)
{
    size_t count = 0U;
    for (int y = 0; y < canvas.height(); ++y) {
        for (int x = 0; x < canvas.width(); ++x) {
            if (canvas.pixel_at(x, y) == GrayLevel::Black) ++count;
        }
    }
    return count;
}

}  // namespace

int main()
{
    assert(prayer_settings_action_at(DeviceSettingsPage::Main, 240, 150).type == DeviceSettingsActionType::OpenLocation);
    assert(prayer_settings_action_at(DeviceSettingsPage::Main, 240, 490).type == DeviceSettingsActionType::OpenWifi);
    assert(prayer_settings_action_at(DeviceSettingsPage::Location, 240, 260).type == DeviceSettingsActionType::EditLatitude);
    assert(prayer_settings_action_at(DeviceSettingsPage::LatitudeEditor, 90, 620).type == DeviceSettingsActionType::EditorClear);
    assert(prayer_settings_action_at(DeviceSettingsPage::LatitudeEditor, 220, 620).type == DeviceSettingsActionType::EditorBackspace);
    assert(prayer_settings_action_at(DeviceSettingsPage::LatitudeEditor, 370, 620).type == DeviceSettingsActionType::EditorConfirm);
    assert(prayer_settings_parent_page(DeviceSettingsPage::Main) == DeviceSettingsPage::Main);
    assert(prayer_settings_parent_page(DeviceSettingsPage::Location) == DeviceSettingsPage::Main);
    assert(prayer_settings_parent_page(DeviceSettingsPage::Calculation) == DeviceSettingsPage::Main);
    assert(prayer_settings_parent_page(DeviceSettingsPage::MethodList) == DeviceSettingsPage::Calculation);
    assert(prayer_settings_parent_page(DeviceSettingsPage::CustomMethod) == DeviceSettingsPage::MethodList);
    assert(prayer_settings_parent_page(DeviceSettingsPage::Adjustments) == DeviceSettingsPage::Main);
    assert(prayer_settings_parent_page(DeviceSettingsPage::LatitudeEditor) == DeviceSettingsPage::Location);
    assert(prayer_settings_parent_page(DeviceSettingsPage::LongitudeEditor) == DeviceSettingsPage::Location);
    assert(prayer_settings_parent_page(DeviceSettingsPage::CustomFajrEditor) == DeviceSettingsPage::CustomMethod);
    assert(prayer_settings_action_at(DeviceSettingsPage::Calculation, 100, 360).type == DeviceSettingsActionType::SelectStandard);
    const DeviceSettingsAction method = prayer_settings_action_at(
        DeviceSettingsPage::MethodList, 240, 420, 0U);
    assert(method.type == DeviceSettingsActionType::SelectMethod && method.value == 3);
    const DeviceSettingsAction custom_interval = prayer_settings_action_at(
        DeviceSettingsPage::MethodList, 240, 510, 4U);
    assert(custom_interval.type == DeviceSettingsActionType::SelectMethod &&
           custom_interval.value == kCalculationMethodCustomInterval);
    assert(prayer_settings_action_at(
               DeviceSettingsPage::MethodList, 100, 600, 1U).type ==
           DeviceSettingsActionType::PreviousMethodPage);
    const DeviceSettingsAction adjustment = prayer_settings_action_at(DeviceSettingsPage::Adjustments, 420, 350);
    assert(adjustment.type == DeviceSettingsActionType::AdjustmentPlus && adjustment.value == 3);
    const DeviceSettingsAction digit = prayer_settings_action_at(DeviceSettingsPage::LatitudeEditor, 100, 250);
    assert(digit.type == DeviceSettingsActionType::EditorDigit && digit.value == 1);
    assert(prayer_settings_action_at(DeviceSettingsPage::Main, -1, 100).type == DeviceSettingsActionType::None);

    std::vector<uint8_t> framebuffer(480U * 800U / 4U, 0xFFU);
    Canvas canvas(480U, 800U, framebuffer.data(), framebuffer.size());

    canvas.clear();
    canvas.draw_text(20, 20, "TITLE", 2, GrayLevel::Black,
                     TextWeight::Regular);
    const size_t regular_text_pixels = black_pixels(canvas);
    canvas.clear();
    canvas.draw_text(20, 20, "TITLE", 2, GrayLevel::Black,
                     TextWeight::Bold);
    assert(black_pixels(canvas) > regular_text_pixels);

    SalahSettings settings = {};
    settings_store_defaults(settings);
    std::strcpy(settings.wifi_ssid, "Home Wi-Fi");
    DeviceSettingsUiState state = {};
    prayer_settings_render(canvas, settings, state);
    assert(black_pixels(canvas) > 10000U);
    state.page = DeviceSettingsPage::Location;
    prayer_settings_render(canvas, settings, state);
    assert(black_pixels(canvas) > 10000U);
    state.page = DeviceSettingsPage::Adjustments;
    prayer_settings_render(canvas, settings, state);
    assert(black_pixels(canvas) > 10000U);
    state.page = DeviceSettingsPage::MethodList;
    state.method_page = 4U;
    prayer_settings_render(canvas, settings, state);
    assert(black_pixels(canvas) > 10000U);
    settings.calculation_method = kCalculationMethodCustomAngles;
    state.page = DeviceSettingsPage::CustomMethod;
    prayer_settings_render(canvas, settings, state);
    assert(black_pixels(canvas) > 10000U);
    state.page = DeviceSettingsPage::LatitudeEditor;
    std::strcpy(state.editor_value, "22.5431");
    prayer_settings_render(canvas, settings, state);
    assert(black_pixels(canvas) > 10000U);
    return 0;
}
