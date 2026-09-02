#include "prayer_settings_pages.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "islamic_background_asset.h"

namespace {

constexpr size_t kMethodsPerPage = 5U;

constexpr const char *kAdjustmentNames[kPrayerAdjustmentCount] = {
    "FAJR", "SUNRISE", "DHUHR", "ASR", "MAGHRIB", "ISHA", "QIYAM",
};

int text_width(const char *text,
               uint8_t scale,
               TextWeight weight = TextWeight::Regular)
{
    if (text == nullptr) {
        return 0;
    }
    const int width = static_cast<int>(std::strlen(text)) * 6 * scale;
    return width + (weight == TextWeight::Bold ? 1 : 0);
}

void centered(Canvas &canvas,
              int center_x,
              int y,
              const char *text,
              uint8_t scale,
              GrayLevel color = GrayLevel::Black,
              TextWeight weight = TextWeight::Regular)
{
    canvas.draw_text(center_x - text_width(text, scale, weight) / 2,
                     y, text, scale, color, weight);
}

void ellipsize(const char *text,
               size_t maximum_characters,
               char *output,
               size_t capacity)
{
    if (output == nullptr || capacity == 0U) {
        return;
    }
    output[0] = '\0';
    if (text == nullptr) {
        return;
    }
    const size_t length = std::strlen(text);
    if (length <= maximum_characters) {
        std::snprintf(output, capacity, "%s", text);
        return;
    }
    const size_t kept = maximum_characters > 3U
                            ? maximum_characters - 3U
                            : 0U;
    std::snprintf(output,
                  capacity,
                  "%.*s...",
                  static_cast<int>(kept),
                  text);
}

void draw_asset(Canvas &canvas, const IslamicBackgroundAsset &asset)
{
    if (asset.data == nullptr || asset.width != canvas.width() ||
        asset.height != canvas.height()) {
        return;
    }
    for (uint16_t y = 0U; y < asset.height; ++y) {
        for (uint16_t x = 0U; x < asset.width; ++x) {
            const uint8_t value = asset.data[
                static_cast<size_t>(y) * asset.stride + x / 8U];
            if ((value & static_cast<uint8_t>(1U << (7U - x % 8U))) != 0U) {
                canvas.draw_pixel(x, y, GrayLevel::Black);
            }
        }
    }
}

void page_base(Canvas &canvas, const char *title, const char *subtitle)
{
    canvas.clear(GrayLevel::White);
    draw_asset(canvas, islamic_setup_background_asset());
    canvas.fill_rect(22, 42, 436, 730, GrayLevel::White);
    centered(canvas, 240, 58, title, 3,
             GrayLevel::Black, TextWeight::Bold);
    centered(canvas, 240, 88, subtitle, 2);
}

void button(Canvas &canvas,
            int x,
            int y,
            int width,
            int height,
            const char *label,
            bool selected = false,
            uint8_t scale = 2U,
            TextWeight weight = TextWeight::Regular)
{
    canvas.fill_rect(x, y, width, height,
                     selected ? GrayLevel::Black : GrayLevel::White);
    canvas.draw_rect(x, y, width, height, GrayLevel::Black);
    centered(canvas, x + width / 2, y + (height - 7 * scale) / 2,
             label, scale,
             selected ? GrayLevel::White : GrayLevel::Black,
             weight);
}

void row(Canvas &canvas,
         int y,
         const char *title,
         const char *value,
         bool selected = false)
{
    canvas.fill_rect(34, y, 412, 76,
                     selected ? GrayLevel::Black : GrayLevel::White);
    canvas.draw_rect(34, y, 412, 76, GrayLevel::Black);
    canvas.draw_text(50, y + 14, title, 2,
                     selected ? GrayLevel::White : GrayLevel::Black,
                     TextWeight::Bold);
    if (value != nullptr && value[0] != '\0') {
        char fitted[32] = {};
        ellipsize(value, 27U, fitted, sizeof(fitted));
        canvas.draw_text(50, y + 43, fitted, 2,
                         selected ? GrayLevel::White : GrayLevel::Black);
    }
    canvas.draw_text(420, y + 28, ">", 2,
                     selected ? GrayLevel::White : GrayLevel::Black);
}

void main_row(Canvas &canvas,
              int y,
              const char *title,
              const char *value)
{
    constexpr int kHeight = 64;
    canvas.fill_rect(34, y, 412, kHeight, GrayLevel::White);
    canvas.draw_rect(34, y, 412, kHeight, GrayLevel::Black);
    canvas.draw_text(50, y + 9, title, 2,
                     GrayLevel::Black, TextWeight::Bold);
    if (value != nullptr && value[0] != '\0') {
        char fitted[32] = {};
        ellipsize(value, 27U, fitted, sizeof(fitted));
        canvas.draw_text(50, y + 36, fitted, 2, GrayLevel::Black);
    }
    canvas.draw_text(420, y + 21, ">", 2, GrayLevel::Black);
}

void footer_status(Canvas &canvas, const DeviceSettingsUiState &state)
{
    if (state.status[0] == '\0') {
        return;
    }
    char fitted[36] = {};
    ellipsize(state.status, 33U, fitted, sizeof(fitted));
    canvas.fill_rect(35, 738, 410, 30, GrayLevel::White);
    centered(canvas, 240, 746, fitted, 2);
}

void render_main(Canvas &canvas,
                 const SalahSettings &settings,
                 const DeviceSettingsUiState &state)
{
    page_base(canvas, "SALAH SETTINGS", "TAP A SECTION TO CHANGE");
    char location[64] = {};
    std::snprintf(location, sizeof(location), "%.28s, %.28s",
                  settings.city, settings.country);
    main_row(canvas, 110, "LOCATION", location);

    char calculation[64] = {};
    std::snprintf(calculation, sizeof(calculation), "%s / %s",
                  settings_method_short_name(settings.calculation_method),
                  settings_asr_school_name(settings.asr_school));
    main_row(canvas, 178, "CALCULATION", calculation);
    main_row(canvas, 246, "MINUTE ADJUSTMENTS", "SEVEN TIME OFFSETS");
    main_row(canvas, 314, "PRAYER REMINDER",
             settings.sound_enabled ? "ON" : "OFF");
    main_row(canvas, 382, "QIYAM DISPLAY",
             settings.qiyam_visible ? "ON" : "OFF");
    main_row(canvas, 450, "WI-FI SETUP", settings.wifi_ssid);
    button(canvas, 34, 540, 412, 72,
           state.first_time ? "SAVE & START" : "SAVE & EXIT",
           true, 2, TextWeight::Bold);
    if (!state.first_time) {
        button(canvas, 34, 625, 412, 60, "DISCARD & EXIT", false, 2);
    } else {
        centered(canvas, 240, 648, "REQUIRED TO CONTINUE", 2);
    }
    footer_status(canvas, state);
}

void coordinate_row(Canvas &canvas,
                    int y,
                    const char *name,
                    const char *value)
{
    button(canvas, 34, y, 66, 88, "-", false, 3);
    button(canvas, 108, y, 264, 88, value, false, 2);
    button(canvas, 380, y, 66, 88, "+", false, 3);
    canvas.fill_rect(105, y - 10, 270, 22, GrayLevel::White);
    centered(canvas, 240, y - 6, name, 2,
             GrayLevel::Black, TextWeight::Bold);
}

void render_location(Canvas &canvas,
                     const SalahSettings &settings,
                     const DeviceSettingsUiState &state)
{
    page_base(canvas, "LOCATION", "NETWORK LOCATION OR COORDINATES");
    char location[64] = {};
    std::snprintf(location, sizeof(location), "%.28s, %.28s",
                  settings.city, settings.country);
    row(canvas, 115, "AUTO-DETECT FROM WI-FI", location);
    char value[24] = {};
    std::snprintf(value, sizeof(value), "%.4f", settings.latitude);
    coordinate_row(canvas, 225, "LATITUDE", value);
    std::snprintf(value, sizeof(value), "%.4f", settings.longitude);
    coordinate_row(canvas, 350, "LONGITUDE", value);

    char offset[24] = {};
    const int total = settings.utc_offset_minutes;
    std::snprintf(offset, sizeof(offset), "UTC%c%02d:%02d",
                  total < 0 ? '-' : '+', std::abs(total) / 60,
                  std::abs(total) % 60);
    coordinate_row(canvas, 475, "TIME ZONE / 15 MIN", offset);
    button(canvas, 34, 580, 198, 56, "- 1 HOUR", false, 2);
    button(canvas, 248, 580, 198, 56, "+ 1 HOUR", false, 2);
    button(canvas, 34, 670, 412, 58, "BACK", false, 2);
    footer_status(canvas, state);
}

void render_calculation(Canvas &canvas,
                        const SalahSettings &settings,
                        const DeviceSettingsUiState &state)
{
    page_base(canvas, "CALCULATION", "CHOOSE THE STANDARD YOU FOLLOW");
    row(canvas, 150, "CALCULATION METHOD",
        settings_method_short_name(settings.calculation_method));
    const CalculationMethodDefinition *definition =
        settings_calculation_method_by_id(settings.calculation_method);
    char rule[40] = {};
    if (settings.calculation_method == kCalculationMethodCustomAngles) {
        std::snprintf(rule, sizeof(rule), "FAJR %.1f / ISHA %.1f DEG",
                      settings.custom_fajr_angle_tenths / 10.0,
                      settings.custom_isha_angle_tenths / 10.0);
    } else if (settings.calculation_method ==
               kCalculationMethodCustomInterval) {
        std::snprintf(rule, sizeof(rule), "FAJR %.1f / ISHA +%u MIN",
                      settings.custom_fajr_angle_tenths / 10.0,
                      static_cast<unsigned>(
                          settings.custom_isha_interval_minutes));
    } else if (definition != nullptr &&
               definition->isha_interval_minutes > 0U) {
        std::snprintf(rule, sizeof(rule), "FAJR %.1f / ISHA +%u MIN",
                      definition->fajr_angle_tenths / 10.0,
                      static_cast<unsigned>(
                          definition->isha_interval_minutes));
    } else if (definition != nullptr) {
        std::snprintf(rule, sizeof(rule), "FAJR %.1f / ISHA %.1f DEG",
                      definition->fajr_angle_tenths / 10.0,
                      definition->isha_angle_tenths / 10.0);
    }
    centered(canvas, 240, 244, rule, 2);
    centered(canvas, 240, 296, "ASR JURISTIC METHOD", 2,
             GrayLevel::Black, TextWeight::Bold);
    button(canvas, 34, 338, 198, 100, "STANDARD",
           settings.asr_school == 0U, 2);
    button(canvas, 248, 338, 198, 100, "HANAFI",
           settings.asr_school == 1U, 2);
    centered(canvas, 240, 466, "ASR ONLY; OTHER TIMES STAY THE SAME", 2);
    button(canvas, 34, 670, 412, 58, "BACK", false, 2);
    footer_status(canvas, state);
}

void render_method_list(Canvas &canvas,
                        const SalahSettings &settings,
                        const DeviceSettingsUiState &state)
{
    const size_t method_count = settings_calculation_method_count();
    const size_t page_count =
        (method_count + kMethodsPerPage - 1U) / kMethodsPerPage;
    const size_t page = std::min<size_t>(state.method_page,
                                        page_count - 1U);
    char subtitle[48] = {};
    std::snprintf(subtitle, sizeof(subtitle),
                  "PAGE %u OF %u / SELECT A STANDARD",
                  static_cast<unsigned>(page + 1U),
                  static_cast<unsigned>(page_count));
    page_base(canvas, "METHOD", subtitle);
    const size_t first = page * kMethodsPerPage;
    for (size_t slot = 0U; slot < kMethodsPerPage; ++slot) {
        const CalculationMethodDefinition *definition =
            settings_calculation_method_at(first + slot);
        if (definition == nullptr) {
            break;
        }
        const int y = 118 + static_cast<int>(slot) * 88;
        button(canvas, 34, y, 412, 72, definition->short_name,
               settings.calculation_method == definition->id, 2,
               TextWeight::Bold);
    }
    button(canvas, 34, 580, 198, 58, "PREVIOUS",
           false, 2);
    button(canvas, 248, 580, 198, 58, "NEXT", false, 2);
    button(canvas, 34, 670, 412, 58, "BACK", false, 2);
    footer_status(canvas, state);
}

void render_custom_method(Canvas &canvas,
                          const SalahSettings &settings,
                          const DeviceSettingsUiState &state)
{
    const bool interval = settings.calculation_method ==
                          kCalculationMethodCustomInterval;
    page_base(canvas, interval ? "CUSTOM INTERVAL" : "CUSTOM ANGLES",
              "TAP A VALUE TO ENTER IT");
    char value[24] = {};
    std::snprintf(value, sizeof(value), "%.1f DEG",
                  settings.custom_fajr_angle_tenths / 10.0);
    row(canvas, 150, "FAJR ANGLE", value);
    if (interval) {
        std::snprintf(value, sizeof(value), "%u MIN",
                      static_cast<unsigned>(
                          settings.custom_isha_interval_minutes));
        row(canvas, 260, "ISHA AFTER MAGHRIB", value);
    } else {
        std::snprintf(value, sizeof(value), "%.1f DEG",
                      settings.custom_isha_angle_tenths / 10.0);
        row(canvas, 260, "ISHA ANGLE", value);
    }
    centered(canvas, 240, 382,
             interval ? "ISHA USES MINUTES AFTER MAGHRIB"
                      : "ANGLES DESCRIBE TWILIGHT",
             2);
    button(canvas, 34, 560, 412, 76, "USE THIS METHOD",
           true, 2, TextWeight::Bold);
    button(canvas, 34, 670, 412, 58, "BACK", false, 2);
    footer_status(canvas, state);
}

void render_adjustments(Canvas &canvas,
                        const SalahSettings &settings,
                        const DeviceSettingsUiState &state)
{
    page_base(canvas, "MINUTE ADJUSTMENTS", "MATCH YOUR LOCAL MOSQUE SCHEDULE");
    for (size_t index = 0U; index < kPrayerAdjustmentCount; ++index) {
        const int y = 112 + static_cast<int>(index) * 73;
        char value[16] = {};
        std::snprintf(value, sizeof(value), "%+d MIN",
                      static_cast<int>(settings.prayer_adjustments[index]));
        button(canvas, 34, y, 62, 58, "-", false, 3);
        canvas.draw_text(113, y + 10, kAdjustmentNames[index], 2,
                         GrayLevel::Black, TextWeight::Bold);
        canvas.draw_text(270, y + 14, value, 2);
        button(canvas, 384, y, 62, 58, "+", false, 3);
    }
    button(canvas, 34, 670, 412, 58, "BACK", false, 2);
    footer_status(canvas, state);
}

void render_editor(Canvas &canvas,
                   const DeviceSettingsUiState &state)
{
    const bool latitude = state.page == DeviceSettingsPage::LatitudeEditor;
    const bool longitude = state.page == DeviceSettingsPage::LongitudeEditor;
    const bool fajr = state.page == DeviceSettingsPage::CustomFajrEditor;
    const bool custom_isha = state.page == DeviceSettingsPage::CustomIshaEditor;
    const char *title = latitude ? "EDIT LATITUDE"
                        : longitude ? "EDIT LONGITUDE"
                        : fajr ? "EDIT FAJR ANGLE"
                               : "EDIT ISHA VALUE";
    const char *subtitle = latitude ? "VALID RANGE -90 TO 90"
                           : longitude ? "VALID RANGE -180 TO 180"
                           : custom_isha ? "ANGLE 1-30 / INTERVAL 1-240"
                                         : "VALID ANGLE 1.0 TO 30.0";
    page_base(canvas, title, subtitle);
    button(canvas, 34, 120, 412, 72,
           state.editor_value[0] == '\0' ? "0" : state.editor_value,
           false, 3);
    const bool coordinate_editor = latitude || longitude;
    const char *keys[] = {
        "1", "2", "3", "4", "5", "6", "7", "8", "9",
        coordinate_editor ? "+/-" : "00", "0", ".",
    };
    for (size_t index = 0U; index < 12U; ++index) {
        const int column = static_cast<int>(index % 3U);
        const int row_index = static_cast<int>(index / 3U);
        button(canvas, 34 + column * 138, 220 + row_index * 88,
               124, 72, keys[index], false, 2);
    }
    button(canvas, 34, 590, 124, 72, "DELETE", false, 2);
    button(canvas, 172, 590, 124, 72, "BACK", false, 2);
    button(canvas, 310, 590, 136, 72, "OK", true, 2,
           TextWeight::Bold);
    footer_status(canvas, state);
}

bool inside(int x, int y, int left, int top, int right, int bottom)
{
    return x >= left && x <= right && y >= top && y <= bottom;
}

}  // namespace

DeviceSettingsAction prayer_settings_action_at(DeviceSettingsPage page,
                                                int x,
                                                int y,
                                                uint8_t method_page)
{
    if (x < 0 || x >= 480 || y < 0 || y >= 800) {
        return {};
    }
    if (page == DeviceSettingsPage::Main) {
        if (inside(x, y, 34, 110, 446, 174)) return {DeviceSettingsActionType::OpenLocation};
        if (inside(x, y, 34, 178, 446, 242)) return {DeviceSettingsActionType::OpenCalculation};
        if (inside(x, y, 34, 246, 446, 310)) return {DeviceSettingsActionType::OpenAdjustments};
        if (inside(x, y, 34, 314, 446, 378)) return {DeviceSettingsActionType::ToggleReminder};
        if (inside(x, y, 34, 382, 446, 446)) return {DeviceSettingsActionType::ToggleQiyamDisplay};
        if (inside(x, y, 34, 450, 446, 514)) return {DeviceSettingsActionType::OpenWifi};
        if (inside(x, y, 34, 540, 446, 612)) return {DeviceSettingsActionType::Save};
        if (inside(x, y, 34, 625, 446, 685)) return {DeviceSettingsActionType::Cancel};
    } else if (page == DeviceSettingsPage::Location) {
        if (inside(x, y, 34, 115, 446, 187)) return {DeviceSettingsActionType::DetectLocation};
        if (inside(x, y, 34, 225, 100, 313)) return {DeviceSettingsActionType::LatitudeMinus};
        if (inside(x, y, 108, 225, 372, 313)) return {DeviceSettingsActionType::EditLatitude};
        if (inside(x, y, 380, 225, 446, 313)) return {DeviceSettingsActionType::LatitudePlus};
        if (inside(x, y, 34, 350, 100, 438)) return {DeviceSettingsActionType::LongitudeMinus};
        if (inside(x, y, 108, 350, 372, 438)) return {DeviceSettingsActionType::EditLongitude};
        if (inside(x, y, 380, 350, 446, 438)) return {DeviceSettingsActionType::LongitudePlus};
        if (inside(x, y, 34, 475, 100, 563)) return {DeviceSettingsActionType::OffsetMinus15};
        if (inside(x, y, 380, 475, 446, 563)) return {DeviceSettingsActionType::OffsetPlus15};
        if (inside(x, y, 34, 580, 232, 636)) return {DeviceSettingsActionType::OffsetMinusHour};
        if (inside(x, y, 248, 580, 446, 636)) return {DeviceSettingsActionType::OffsetPlusHour};
        if (inside(x, y, 34, 670, 446, 728)) return {DeviceSettingsActionType::Back};
    } else if (page == DeviceSettingsPage::Calculation) {
        if (inside(x, y, 34, 150, 446, 226)) return {DeviceSettingsActionType::SelectMethod, -1};
        if (inside(x, y, 34, 338, 232, 438)) return {DeviceSettingsActionType::SelectStandard};
        if (inside(x, y, 248, 338, 446, 438)) return {DeviceSettingsActionType::SelectHanafi};
        if (inside(x, y, 34, 670, 446, 728)) return {DeviceSettingsActionType::Back};
    } else if (page == DeviceSettingsPage::MethodList) {
        const size_t first = static_cast<size_t>(method_page) *
                             kMethodsPerPage;
        for (size_t slot = 0U; slot < kMethodsPerPage; ++slot) {
            const CalculationMethodDefinition *definition =
                settings_calculation_method_at(first + slot);
            const int top = 118 + static_cast<int>(slot) * 88;
            if (definition != nullptr &&
                inside(x, y, 34, top, 446, top + 72)) {
                return {DeviceSettingsActionType::SelectMethod,
                        definition->id};
            }
        }
        if (inside(x, y, 34, 580, 232, 638)) return {DeviceSettingsActionType::PreviousMethodPage};
        if (inside(x, y, 248, 580, 446, 638)) return {DeviceSettingsActionType::NextMethodPage};
        if (inside(x, y, 34, 670, 446, 728)) return {DeviceSettingsActionType::Back};
    } else if (page == DeviceSettingsPage::CustomMethod) {
        if (inside(x, y, 34, 150, 446, 226)) return {DeviceSettingsActionType::EditCustomFajr};
        if (inside(x, y, 34, 260, 446, 336)) return {DeviceSettingsActionType::EditCustomIsha};
        if (inside(x, y, 34, 560, 446, 636)) return {DeviceSettingsActionType::FinishCustomMethod};
        if (inside(x, y, 34, 670, 446, 728)) return {DeviceSettingsActionType::Back};
    } else if (page == DeviceSettingsPage::Adjustments) {
        for (int index = 0; index < static_cast<int>(kPrayerAdjustmentCount); ++index) {
            const int top = 112 + index * 73;
            if (inside(x, y, 34, top, 96, top + 58)) return {DeviceSettingsActionType::AdjustmentMinus, index};
            if (inside(x, y, 384, top, 446, top + 58)) return {DeviceSettingsActionType::AdjustmentPlus, index};
        }
        if (inside(x, y, 34, 670, 446, 728)) return {DeviceSettingsActionType::Back};
    } else {
        for (int index = 0; index < 12; ++index) {
            const int column = index % 3;
            const int row_index = index / 3;
            if (inside(x, y, 34 + column * 138, 220 + row_index * 88,
                       158 + column * 138, 292 + row_index * 88)) {
                if (index == 9) {
                    const bool coordinate_editor =
                        page == DeviceSettingsPage::LatitudeEditor ||
                        page == DeviceSettingsPage::LongitudeEditor;
                    return {coordinate_editor
                                ? DeviceSettingsActionType::EditorSign
                                : DeviceSettingsActionType::EditorDoubleZero};
                }
                if (index == 11) return {DeviceSettingsActionType::EditorDot};
                return {DeviceSettingsActionType::EditorDigit,
                        index == 10 ? 0 : index + 1};
            }
        }
        if (inside(x, y, 34, 590, 158, 662)) return {DeviceSettingsActionType::EditorClear};
        if (inside(x, y, 172, 590, 296, 662)) return {DeviceSettingsActionType::EditorBackspace};
        if (inside(x, y, 310, 590, 446, 662)) return {DeviceSettingsActionType::EditorConfirm};
    }
    return {};
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
    page_base(canvas, title, "STICKY SALAH");
    centered(canvas, 240, 330, message, 2);
    centered(canvas, 240, 390, "PLEASE WAIT", 2);
}
