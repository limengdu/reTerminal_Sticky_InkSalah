#include "provisioning_form.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {

constexpr size_t kMaxFormLength = 1024U;

void set_error(char *error, size_t capacity, const char *message)
{
    if (error != nullptr && capacity > 0U) {
        std::snprintf(error, capacity, "%s", message);
    }
}

int hex_value(char value)
{
    if (value >= '0' && value <= '9') {
        return value - '0';
    }
    value = static_cast<char>(std::tolower(static_cast<unsigned char>(value)));
    return value >= 'a' && value <= 'f' ? value - 'a' + 10 : -1;
}

bool url_decode(const char *source, char *destination, size_t capacity)
{
    if (source == nullptr || destination == nullptr || capacity == 0U) {
        return false;
    }
    size_t output = 0U;
    for (size_t input = 0U; source[input] != '\0'; ++input) {
        char value = source[input];
        if (value == '+') {
            value = ' ';
        } else if (value == '%') {
            const int high = hex_value(source[input + 1U]);
            const int low = high >= 0 ? hex_value(source[input + 2U]) : -1;
            if (high < 0 || low < 0) {
                return false;
            }
            value = static_cast<char>((high << 4) | low);
            input += 2U;
        }
        if (output + 1U >= capacity) {
            return false;
        }
        destination[output++] = value;
    }
    destination[output] = '\0';
    return true;
}

bool copy_field(const char *value, char *destination, size_t capacity)
{
    char decoded[128] = {};
    if (!url_decode(value, decoded, sizeof(decoded)) ||
        std::strlen(decoded) >= capacity) {
        return false;
    }
    std::strncpy(destination, decoded, capacity - 1U);
    destination[capacity - 1U] = '\0';
    return true;
}

bool parse_number(const char *text, long minimum, long maximum, uint8_t &value)
{
    char *end = nullptr;
    const long parsed = std::strtol(text, &end, 10);
    if (text == end || end == nullptr || *end != '\0' ||
        parsed < minimum || parsed > maximum) {
        return false;
    }
    value = static_cast<uint8_t>(parsed);
    return true;
}

}  // namespace

bool provisioning_parse_utc_offset(const char *text,
                                   int16_t &offset_minutes)
{
    if (text == nullptr || std::strlen(text) != 6U ||
        (text[0] != '+' && text[0] != '-') || text[3] != ':' ||
        !std::isdigit(static_cast<unsigned char>(text[1])) ||
        !std::isdigit(static_cast<unsigned char>(text[2])) ||
        !std::isdigit(static_cast<unsigned char>(text[4])) ||
        !std::isdigit(static_cast<unsigned char>(text[5]))) {
        return false;
    }
    const int hours = (text[1] - '0') * 10 + text[2] - '0';
    const int minutes = (text[4] - '0') * 10 + text[5] - '0';
    int total = hours * 60 + minutes;
    if (minutes > 59 || total > 14 * 60) {
        return false;
    }
    if (text[0] == '-') {
        total = -total;
    }
    if (total < -12 * 60) {
        return false;
    }
    offset_minutes = static_cast<int16_t>(total);
    return true;
}

bool provisioning_form_parse(const char *body,
                             SalahSettings &settings,
                             char *error,
                             size_t error_capacity)
{
    if (body == nullptr || std::strlen(body) > kMaxFormLength) {
        set_error(error, error_capacity, "The setup form is too large.");
        return false;
    }
    settings_store_defaults(settings);
    settings.configured = true;
    settings.sound_enabled = false;

    bool has_ssid = false;
    bool has_city = false;
    bool has_country = false;
    bool has_offset = false;
    bool has_method = false;
    bool has_school = false;
    char buffer[kMaxFormLength + 1U] = {};
    std::strncpy(buffer, body, sizeof(buffer) - 1U);

    char *save = nullptr;
    for (char *pair = strtok_r(buffer, "&", &save);
         pair != nullptr;
         pair = strtok_r(nullptr, "&", &save)) {
        char *equals = std::strchr(pair, '=');
        if (equals == nullptr) {
            continue;
        }
        *equals = '\0';
        const char *key = pair;
        const char *value = equals + 1;
        if (std::strcmp(key, "ssid") == 0) {
            has_ssid = copy_field(value,
                                  settings.wifi_ssid,
                                  sizeof(settings.wifi_ssid));
        } else if (std::strcmp(key, "password") == 0) {
            if (!copy_field(value,
                            settings.wifi_password,
                            sizeof(settings.wifi_password))) {
                set_error(error, error_capacity, "The Wi-Fi password is too long.");
                return false;
            }
        } else if (std::strcmp(key, "city") == 0) {
            has_city = copy_field(value, settings.city, sizeof(settings.city));
        } else if (std::strcmp(key, "country") == 0) {
            has_country = copy_field(value,
                                     settings.country,
                                     sizeof(settings.country));
        } else if (std::strcmp(key, "utc_offset") == 0) {
            char decoded[16] = {};
            has_offset = url_decode(value, decoded, sizeof(decoded)) &&
                         provisioning_parse_utc_offset(
                             decoded, settings.utc_offset_minutes);
        } else if (std::strcmp(key, "method") == 0) {
            has_method = parse_number(value,
                                      0,
                                      99,
                                      settings.calculation_method);
        } else if (std::strcmp(key, "school") == 0) {
            has_school = parse_number(value, 0, 1, settings.asr_school);
        } else if (std::strcmp(key, "sound") == 0) {
            settings.sound_enabled = std::strcmp(value, "on") == 0;
        }
    }

    if (!has_ssid || !has_city || !has_country || !has_offset ||
        !has_method || !has_school || !settings_store_valid(settings)) {
        set_error(error,
                  error_capacity,
                  "Please complete every required field with valid values.");
        return false;
    }
    set_error(error, error_capacity, "");
    return true;
}
