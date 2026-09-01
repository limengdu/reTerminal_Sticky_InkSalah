#include "provisioning_form.h"

#include <cctype>
#include <cstdio>
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
    if (value >= '0' && value <= '9') return value - '0';
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
            if (high < 0 || low < 0) return false;
            value = static_cast<char>((high << 4) | low);
            input += 2U;
        }
        if (output + 1U >= capacity) return false;
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
    std::snprintf(destination, capacity, "%s", decoded);
    return true;
}

}  // namespace

bool provisioning_wifi_form_parse(const char *body,
                                  SalahSettings &settings,
                                  char *error,
                                  size_t error_capacity)
{
    if (body == nullptr || std::strlen(body) > kMaxFormLength) {
        set_error(error, error_capacity, "The setup form is too large.");
        return false;
    }
    settings_store_defaults(settings);
    bool has_ssid = false;
    char buffer[kMaxFormLength + 1U] = {};
    std::snprintf(buffer, sizeof(buffer), "%s", body);
    char *save = nullptr;
    for (char *pair = strtok_r(buffer, "&", &save);
         pair != nullptr;
         pair = strtok_r(nullptr, "&", &save)) {
        char *equals = std::strchr(pair, '=');
        if (equals == nullptr) continue;
        *equals = '\0';
        const char *value = equals + 1;
        if (std::strcmp(pair, "ssid") == 0) {
            has_ssid = copy_field(value, settings.wifi_ssid,
                                  sizeof(settings.wifi_ssid));
        } else if (std::strcmp(pair, "password") == 0 &&
                   !copy_field(value, settings.wifi_password,
                               sizeof(settings.wifi_password))) {
            set_error(error, error_capacity,
                      "The Wi-Fi password is too long.");
            return false;
        }
    }
    if (!has_ssid || !settings_wifi_valid(settings)) {
        set_error(error, error_capacity,
                  "Choose or enter a valid Wi-Fi network name.");
        return false;
    }
    set_error(error, error_capacity, "");
    return true;
}
