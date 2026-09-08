#include "prayer_settings_pages.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "ui_style.h"

namespace {

constexpr size_t kMethodsPerPage = 5U;
constexpr const char *kAdjustmentNames[] = {"Fajr", "Sunrise", "Dhuhr", "Asr", "Maghrib", "Isha", "Qiyam"};

struct Box {
    int x, y, width, height;
    bool contains(int px, int py) const
    {
        return px >= x && px < x + width && py >= y && py < y + height;
    }
};

// These bounds drive both rendering and touch actions.
// 这些区域同时用于绘制和触摸操作映射。
constexpr Box kMainRows[] = {
    {46, 220, 399, 77}, {46, 314, 399, 77}, {46, 408, 399, 77},
    {46, 502, 399, 77}, {46, 596, 399, 77},
};
constexpr Box kSave{46, 698, 195, 66};
constexpr Box kDiscard{250, 698, 195, 66};
constexpr Box kMethod{41, 268, 399, 77};
constexpr Box kStandard{41, 446, 187, 78};
constexpr Box kHanafi{260, 446, 187, 78};
constexpr Box kCalculationBack{143, 686, 194, 64};
constexpr Box kBack{143, 704, 194, 56};
constexpr Box kLocationDetect{41, 220, 399, 72};
constexpr Box kQiyam{41, 636, 399, 46};

Box method_box(size_t slot) { return {41, 220 + static_cast<int>(slot) * 81, 399, 66}; }
Box coordinate_box(int row, int column)
{
    return {column == 0 ? 41 : column == 1 ? 113 : 374,
            318 + row * 110, column == 1 ? 248 : 66, 72};
}
Box adjustment_box(int index, bool plus)
{
    return {plus ? 392 : 41, 220 + index * 58, 48, 48};
}
Box key_box(int index)
{
    return {41 + index % 3 * 137, 306 + index / 3 * 79, 125, 66};
}

void button(Canvas &canvas, Box box, const char *text, bool selected = false,
             uint8_t size = 24U, bool round = true)
{
    if (round) {
        ui::pill(canvas, box.x, box.y, box.width, box.height, text, selected, size);
    } else {
        canvas.fill_round_rect(box.x, box.y, box.width, box.height, 12,
                               selected ? GrayLevel::Black : GrayLevel::White);
        canvas.draw_round_rect(box.x, box.y, box.width, box.height, 12);
        ui::fitted_centered(canvas, box.x + box.width / 2,
                             box.y + (box.height - size) / 2,
                             text, box.width - 20, size, TextWeight::Bold,
                             selected ? GrayLevel::White : GrayLevel::Black);
    }
}

void page_base(Canvas &canvas, const char *title, const char *subtitle)
{
    canvas.clear();
    ui::bitmap(canvas, 0, 0, inksalah_lantern_asset());
    const bool main = std::strcmp(title, "INKSALAH Settings") == 0;
    const int width = main ? 340 : 368;
    canvas.fill_round_rect(240 - width / 2, 136, width, 67, 33, GrayLevel::Black);
    if (main) {
        ui::bitmap(canvas, 89, 151, inksalah_book_asset(), GrayLevel::White);
    }
    ui::fitted_centered(canvas, main ? 267 : 240, 157, title,
                         main ? 270 : 330, 24, TextWeight::Bold, GrayLevel::White);
    if (subtitle != nullptr && subtitle[0] != '\0')
        ui::fitted_centered(canvas, 240, 775, subtitle, 430, 20,
                             TextWeight::Regular, GrayLevel::DarkGray);
}

void card(Canvas &canvas, Box box, const char *title, const char *value)
{
    canvas.draw_round_rect(box.x, box.y, box.width, box.height, 12);
    canvas.draw_text_sized(box.x + 17, box.y + 10, title, 20, GrayLevel::DarkGray);
    char fitted[128] = {};
    ui::fit(value, fitted, sizeof(fitted), box.width - 68, 24, TextWeight::Bold);
    canvas.draw_text_sized(box.x + 17, box.y + 41, fitted, 24,
                           GrayLevel::Black, TextWeight::Bold);
    canvas.draw_text_sized(box.x + box.width - 34, box.y + 28, ">", 28, GrayLevel::DarkGray);
}

void footer_status(Canvas &canvas, const DeviceSettingsUiState &state)
{
    if (state.status[0] == '\0') return;
    canvas.fill_rect(0, 771, 480, 29, GrayLevel::White);
    ui::fitted_centered(canvas, 240, 775, state.status, 436, 20);
}

void render_main(Canvas &canvas, const SalahSettings &settings, const DeviceSettingsUiState &state)
{
    page_base(canvas, "INKSALAH Settings", "");
    char location[128] = {};
    std::snprintf(location, sizeof(location), "%.56s, %.56s", settings.city, settings.country);
    card(canvas, kMainRows[0], "Location", location);
    card(canvas, kMainRows[1], "Calculation Method",
          settings.calculation_method == 13U ? "D\u0130B, Turkey" :
          settings_method_short_name(settings.calculation_method));
    card(canvas, kMainRows[2], "Minute Adjustments",
          settings.qiyam_visible ? "Seven Displayed Times" : "Six Displayed Times");
    card(canvas, kMainRows[3], "Prayer Reminder", settings.sound_enabled ? "ON" : "OFF");
    card(canvas, kMainRows[4], "Wi-Fi Setup",
          settings.wifi_ssid[0] == '\0' ? "Not connected" : settings.wifi_ssid);
    if (state.first_time) {
        button(canvas, {46, 698, 399, 66}, "Save & Start", true);
    } else {
        button(canvas, kSave, "Save & Exit", true);
        button(canvas, kDiscard, "Discard & Exit");
    }
    footer_status(canvas, state);
}

void render_location(Canvas &canvas, const SalahSettings &settings, const DeviceSettingsUiState &state)
{
    page_base(canvas, "Location", "");
    char value[128] = {};
    std::snprintf(value, sizeof(value), "%.56s, %.56s", settings.city, settings.country);
    card(canvas, kLocationDetect, "Detect from connected Wi-Fi", value);
    for (int row = 0; row < 3; ++row) {
        if (row == 0) std::snprintf(value, sizeof(value), "%.4f", settings.latitude);
        else if (row == 1) std::snprintf(value, sizeof(value), "%.4f", settings.longitude);
        else {
            const int offset = settings.utc_offset_minutes;
            std::snprintf(value, sizeof(value), "UTC%c%02d:%02d", offset < 0 ? '-' : '+',
                          std::abs(offset) / 60, std::abs(offset) % 60);
        }
        button(canvas, coordinate_box(row, 0), "-", false, 28, false);
        button(canvas, coordinate_box(row, 1), value, false, 24, false);
        button(canvas, coordinate_box(row, 2), "+", false, 28, false);
        canvas.fill_rect(117, 303 + row * 110, 238, 25, GrayLevel::White);
        ui::centered(canvas, 237, 303 + row * 110,
                      row == 0 ? "Latitude" : row == 1 ? "Longitude" : "Time zone / 15 min", 20);
    }
    button(canvas, {41, 628, 190, 48}, "- 1 hour", false, 22);
    button(canvas, {250, 628, 190, 48}, "+ 1 hour", false, 22);
    button(canvas, kBack, "Back");
    footer_status(canvas, state);
}

void render_calculation(Canvas &canvas, const SalahSettings &settings, const DeviceSettingsUiState &state)
{
    page_base(canvas, "Calculation Method", "");
    canvas.draw_text_sized(53, 238, "Calculation Method", 22, GrayLevel::Black, TextWeight::Bold);
    canvas.draw_round_rect(kMethod.x, kMethod.y, kMethod.width, kMethod.height, 12);
    canvas.draw_text_sized(60, 293, settings_method_short_name(settings.calculation_method),
                           24, GrayLevel::Black, TextWeight::Bold);
    canvas.draw_text_sized(407, 293, ">", 28, GrayLevel::DarkGray);
    const CalculationMethodDefinition *definition = settings_calculation_method_by_id(settings.calculation_method);
    int fajr = settings.custom_fajr_angle_tenths;
    int isha = settings.custom_isha_angle_tenths;
    int interval = settings.calculation_method == kCalculationMethodCustomInterval
                       ? settings.custom_isha_interval_minutes : 0;
    if (definition != nullptr && settings.calculation_method != kCalculationMethodCustomAngles &&
        settings.calculation_method != kCalculationMethodCustomInterval) {
        fajr = definition->fajr_angle_tenths;
        isha = definition->isha_angle_tenths;
        interval = definition->isha_interval_minutes;
    }
    char rule[80] = {};
    if (interval > 0) std::snprintf(rule, sizeof(rule), "* Fajr %.1f / Isha +%d min", fajr / 10.0, interval);
    else std::snprintf(rule, sizeof(rule), "* Fajr %.1f / Isha %.1f deg", fajr / 10.0, isha / 10.0);
    ui::centered(canvas, 240, 362, rule, 20, TextWeight::Regular, GrayLevel::DarkGray);
    canvas.draw_text_sized(47, 415, "Asr Juristic Method", 22, GrayLevel::Black, TextWeight::Bold);
    button(canvas, kStandard, "Standard", settings.asr_school == 0U, 24, false);
    button(canvas, kHanafi, "Hanafi", settings.asr_school == 1U, 24, false);
    ui::centered(canvas, 240, 543, "* Asr only; other times stay the same.", 20,
                  TextWeight::Regular, GrayLevel::DarkGray);
    button(canvas, kCalculationBack, "Back");
    footer_status(canvas, state);
}

void render_method_list(Canvas &canvas, const SalahSettings &settings, const DeviceSettingsUiState &state)
{
    const size_t pages = (settings_calculation_method_count() + kMethodsPerPage - 1U) / kMethodsPerPage;
    const size_t page = std::min<size_t>(state.method_page, pages - 1U);
    char subtitle[36] = {};
    std::snprintf(subtitle, sizeof(subtitle), "Page %u of %u",
                  static_cast<unsigned>(page + 1U), static_cast<unsigned>(pages));
    page_base(canvas, "Calculation Method", subtitle);
    for (size_t slot = 0; slot < kMethodsPerPage; ++slot) {
        const auto *method = settings_calculation_method_at(page * kMethodsPerPage + slot);
        if (method == nullptr) break;
        button(canvas, method_box(slot), method->short_name,
                settings.calculation_method == method->id, 24, false);
    }
    button(canvas, {41, 640, 190, 48}, "Previous", false, 22);
    button(canvas, {250, 640, 190, 48}, "Next", false, 22);
    button(canvas, kBack, "Back");
    footer_status(canvas, state);
}

void render_custom_method(Canvas &canvas, const SalahSettings &settings, const DeviceSettingsUiState &state)
{
    const bool interval = settings.calculation_method == kCalculationMethodCustomInterval;
    page_base(canvas, interval ? "Custom Interval" : "Custom Angles", "");
    char value[36] = {};
    std::snprintf(value, sizeof(value), "%.1f degrees", settings.custom_fajr_angle_tenths / 10.0);
    card(canvas, {41, 240, 399, 77}, "Fajr Angle", value);
    if (interval) std::snprintf(value, sizeof(value), "%u minutes", settings.custom_isha_interval_minutes);
    else std::snprintf(value, sizeof(value), "%.1f degrees", settings.custom_isha_angle_tenths / 10.0);
    card(canvas, {41, 352, 399, 77}, interval ? "Isha after Maghrib" : "Isha Angle", value);
    ui::centered(canvas, 240, 468, interval ? "Isha: minutes after Maghrib" : "Angles define twilight", 20);
    button(canvas, {41, 574, 399, 64}, "Use This Method", true);
    button(canvas, kBack, "Back");
    footer_status(canvas, state);
}

void render_adjustments(Canvas &canvas, const SalahSettings &settings, const DeviceSettingsUiState &state)
{
    page_base(canvas, "Minute Adjustments", "");
    for (int index = 0; index < static_cast<int>(kPrayerAdjustmentCount); ++index) {
        const Box box = adjustment_box(index, false);
        char value[24] = {};
        std::snprintf(value, sizeof(value), "%+d min", settings.prayer_adjustments[index]);
        button(canvas, box, "-", false, 28, false);
        button(canvas, adjustment_box(index, true), "+", false, 28, false);
        canvas.draw_text_sized(106, box.y + 13, kAdjustmentNames[index], 22, GrayLevel::Black, TextWeight::Bold);
        ui::centered(canvas, 310, box.y + 14, value, 22);
    }
    button(canvas, kQiyam, settings.qiyam_visible ? "Show Qiyam: ON" : "Show Qiyam: OFF",
            settings.qiyam_visible, 22);
    button(canvas, kBack, "Back");
    footer_status(canvas, state);
}

void render_editor(Canvas &canvas, const DeviceSettingsUiState &state)
{
    const bool latitude = state.page == DeviceSettingsPage::LatitudeEditor;
    const bool longitude = state.page == DeviceSettingsPage::LongitudeEditor;
    const bool fajr = state.page == DeviceSettingsPage::CustomFajrEditor;
    page_base(canvas, latitude ? "Latitude" : longitude ? "Longitude" : fajr ? "Fajr Angle" : "Isha Value",
               latitude ? "Range: -90 to 90" : longitude ? "Range: -180 to 180"
               : fajr ? "Range: 1.0 to 30.0 degrees" : "Angle: 1-30 / interval: 1-240");
    button(canvas, {41, 224, 399, 62}, state.editor_value[0] == '\0' ? "0" : state.editor_value, false, 28, false);
    const char *keys[] = {"1", "2", "3", "4", "5", "6", "7", "8", "9",
                          latitude || longitude ? "+/-" : "00", "0", "."};
    for (int index = 0; index < 12; ++index) button(canvas, key_box(index), keys[index], false, 28, false);
    button(canvas, {41, 644, 125, 62}, "Delete", false, 22, false);
    button(canvas, {178, 644, 125, 62}, "Back", false, 22, false);
    button(canvas, {315, 644, 125, 62}, "OK", true, 22, false);
    footer_status(canvas, state);
}

}  // namespace

DeviceSettingsAction prayer_settings_action_at(DeviceSettingsPage page, int x, int y,
                                                uint8_t method_page, bool first_time)
{
    if (x < 0 || x >= 480 || y < 0 || y >= 800) return {};
    using Action = DeviceSettingsActionType;
    if (page == DeviceSettingsPage::Main) {
        constexpr Action actions[] = {Action::OpenLocation, Action::OpenCalculation,
            Action::OpenAdjustments, Action::ToggleReminder, Action::OpenWifi};
        for (size_t i = 0; i < 5U; ++i)
            if (kMainRows[i].contains(x, y)) return {actions[i]};
        if (first_time && Box{46, 698, 399, 66}.contains(x, y)) return {Action::Save};
        if (kSave.contains(x, y)) return {Action::Save};
        if (!first_time && kDiscard.contains(x, y)) return {Action::Cancel};
    } else if (page == DeviceSettingsPage::Location) {
        if (kLocationDetect.contains(x, y)) return {Action::DetectLocation};
        constexpr Action actions[3][3] = {
            {Action::LatitudeMinus, Action::EditLatitude, Action::LatitudePlus},
            {Action::LongitudeMinus, Action::EditLongitude, Action::LongitudePlus},
            {Action::OffsetMinus15, Action::None, Action::OffsetPlus15},
        };
        for (int row = 0; row < 3; ++row)
            for (int col = 0; col < 3; ++col)
                if (coordinate_box(row, col).contains(x, y)) return {actions[row][col]};
        if (Box{41, 628, 190, 48}.contains(x, y)) return {Action::OffsetMinusHour};
        if (Box{250, 628, 190, 48}.contains(x, y)) return {Action::OffsetPlusHour};
        if (kBack.contains(x, y)) return {Action::Back};
    } else if (page == DeviceSettingsPage::Calculation) {
        if (kMethod.contains(x, y)) return {Action::SelectMethod, -1};
        if (kStandard.contains(x, y)) return {Action::SelectStandard};
        if (kHanafi.contains(x, y)) return {Action::SelectHanafi};
        if (kCalculationBack.contains(x, y)) return {Action::Back};
    } else if (page == DeviceSettingsPage::MethodList) {
        const size_t pages = (settings_calculation_method_count() + kMethodsPerPage - 1U) / kMethodsPerPage;
        const size_t first = std::min<size_t>(method_page, pages - 1U) * kMethodsPerPage;
        for (size_t slot = 0; slot < kMethodsPerPage; ++slot) {
            const auto *method = settings_calculation_method_at(first + slot);
            if (method != nullptr && method_box(slot).contains(x, y)) return {Action::SelectMethod, method->id};
        }
        if (Box{41, 640, 190, 48}.contains(x, y)) return {Action::PreviousMethodPage};
        if (Box{250, 640, 190, 48}.contains(x, y)) return {Action::NextMethodPage};
        if (kBack.contains(x, y)) return {Action::Back};
    } else if (page == DeviceSettingsPage::CustomMethod) {
        if (Box{41, 240, 399, 77}.contains(x, y)) return {Action::EditCustomFajr};
        if (Box{41, 352, 399, 77}.contains(x, y)) return {Action::EditCustomIsha};
        if (Box{41, 574, 399, 64}.contains(x, y)) return {Action::FinishCustomMethod};
        if (kBack.contains(x, y)) return {Action::Back};
    } else if (page == DeviceSettingsPage::Adjustments) {
        for (int index = 0; index < static_cast<int>(kPrayerAdjustmentCount); ++index) {
            if (adjustment_box(index, false).contains(x, y)) return {Action::AdjustmentMinus, index};
            if (adjustment_box(index, true).contains(x, y)) return {Action::AdjustmentPlus, index};
        }
        if (kQiyam.contains(x, y)) return {Action::ToggleQiyamDisplay};
        if (kBack.contains(x, y)) return {Action::Back};
    } else {
        for (int index = 0; index < 12; ++index) {
            if (!key_box(index).contains(x, y)) continue;
            if (index == 9) return {page == DeviceSettingsPage::LatitudeEditor ||
                                      page == DeviceSettingsPage::LongitudeEditor
                                      ? Action::EditorSign : Action::EditorDoubleZero};
            if (index == 11) return {Action::EditorDot};
            return {Action::EditorDigit, index == 10 ? 0 : index + 1};
        }
        if (Box{41, 644, 125, 62}.contains(x, y)) return {Action::EditorClear};
        if (Box{178, 644, 125, 62}.contains(x, y)) return {Action::EditorBackspace};
        if (Box{315, 644, 125, 62}.contains(x, y)) return {Action::EditorConfirm};
    }
    return {};
}

DeviceSettingsPage prayer_settings_entry_page(bool first_time, DeviceSettingsPage requested)
{
    return !first_time && requested == DeviceSettingsPage::Location
               ? DeviceSettingsPage::Location : DeviceSettingsPage::Main;
}

DeviceSettingsPage prayer_settings_parent_page(DeviceSettingsPage page)
{
    switch (page) {
    case DeviceSettingsPage::MethodList:
        return DeviceSettingsPage::Calculation;
    case DeviceSettingsPage::CustomMethod:
        return DeviceSettingsPage::MethodList;
    case DeviceSettingsPage::LatitudeEditor:
    case DeviceSettingsPage::LongitudeEditor:
        return DeviceSettingsPage::Location;
    case DeviceSettingsPage::CustomFajrEditor:
    case DeviceSettingsPage::CustomIshaEditor:
        return DeviceSettingsPage::CustomMethod;
    case DeviceSettingsPage::Location:
    case DeviceSettingsPage::Calculation:
    case DeviceSettingsPage::Adjustments:
        return DeviceSettingsPage::Main;
    case DeviceSettingsPage::Main:
    default:
        return DeviceSettingsPage::Main;
    }
}

void prayer_settings_render(Canvas &canvas,
                            const SalahSettings &settings,
                            const DeviceSettingsUiState &state)
{
    switch (state.page) {
    case DeviceSettingsPage::Location:
        render_location(canvas, settings, state);
        break;
    case DeviceSettingsPage::Calculation:
        render_calculation(canvas, settings, state);
        break;
    case DeviceSettingsPage::MethodList:
        render_method_list(canvas, settings, state);
        break;
    case DeviceSettingsPage::CustomMethod:
        render_custom_method(canvas, settings, state);
        break;
    case DeviceSettingsPage::Adjustments:
        render_adjustments(canvas, settings, state);
        break;
    case DeviceSettingsPage::LatitudeEditor:
    case DeviceSettingsPage::LongitudeEditor:
    case DeviceSettingsPage::CustomFajrEditor:
    case DeviceSettingsPage::CustomIshaEditor:
        render_editor(canvas, state);
        break;
    case DeviceSettingsPage::Main:
    default:
        render_main(canvas, settings, state);
        break;
    }
}

void prayer_settings_render_loading(Canvas &canvas,
                                    const char *title,
                                    const char *message)
{
    page_base(canvas, title, "INKSALAH");
    ui::fitted_centered(canvas, 240, 330, message, 416, 24);
    ui::centered(canvas, 240, 390, "Please wait", 20);
}
