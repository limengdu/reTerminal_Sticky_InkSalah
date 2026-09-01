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
    assert(prayer_settings_action_at(DeviceSettingsPage::Calculation, 100, 360).type == DeviceSettingsActionType::SelectStandard);
    const DeviceSettingsAction method = prayer_settings_action_at(DeviceSettingsPage::MethodList, 240, 255);
    assert(method.type == DeviceSettingsActionType::SelectMethod && method.value == 3);
    const DeviceSettingsAction adjustment = prayer_settings_action_at(DeviceSettingsPage::Adjustments, 420, 350);
    assert(adjustment.type == DeviceSettingsActionType::AdjustmentPlus && adjustment.value == 3);
    const DeviceSettingsAction digit = prayer_settings_action_at(DeviceSettingsPage::LatitudeEditor, 100, 250);
    assert(digit.type == DeviceSettingsActionType::EditorDigit && digit.value == 1);
    assert(prayer_settings_action_at(DeviceSettingsPage::Main, -1, 100).type == DeviceSettingsActionType::None);

    std::vector<uint8_t> framebuffer(480U * 800U / 4U, 0xFFU);
    Canvas canvas(480U, 800U, framebuffer.data(), framebuffer.size());
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
    state.page = DeviceSettingsPage::LatitudeEditor;
    std::strcpy(state.editor_value, "22.5431");
    prayer_settings_render(canvas, settings, state);
    assert(black_pixels(canvas) > 10000U);
    return 0;
}
