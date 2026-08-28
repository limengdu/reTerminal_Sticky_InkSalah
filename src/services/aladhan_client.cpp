#include "aladhan_client.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "app_log.h"
#include "cJSON.h"
#include "esp_crt_bundle.h"
#include "esp_heap_caps.h"
#include "esp_http_client.h"

namespace {

constexpr char kTag[] = "aladhan_client";
constexpr size_t kResponseCapacity = 128U * 1024U;
constexpr size_t kValidationResponseCapacity = 16U * 1024U;

struct ResponseBuffer {
    char *data = nullptr;
    size_t length = 0U;
    size_t capacity = 0U;
    bool overflow = false;
};

esp_err_t http_event_handler(esp_http_client_event_t *event)
{
    auto *buffer = static_cast<ResponseBuffer *>(event->user_data);
    if (event->event_id != HTTP_EVENT_ON_DATA || buffer == nullptr ||
        event->data == nullptr || event->data_len <= 0) {
        return ESP_OK;
    }
    const size_t incoming = static_cast<size_t>(event->data_len);
    if (buffer->length + incoming + 1U > buffer->capacity) {
        buffer->overflow = true;
        return ESP_ERR_NO_MEM;
    }
    std::memcpy(buffer->data + buffer->length, event->data, incoming);
    buffer->length += incoming;
    buffer->data[buffer->length] = '\0';
    return ESP_OK;
}

bool unreserved(unsigned char value)
{
    return std::isalnum(value) || value == '-' || value == '_' ||
           value == '.' || value == '~';
}

bool url_encode(const char *source, char *destination, size_t capacity)
{
    constexpr char kHex[] = "0123456789ABCDEF";
    if (source == nullptr || destination == nullptr || capacity == 0U) {
        return false;
    }
    size_t output = 0U;
    for (size_t input = 0U; source[input] != '\0'; ++input) {
        const unsigned char value = static_cast<unsigned char>(source[input]);
        const size_t needed = unreserved(value) ? 1U : 3U;
        if (output + needed + 1U > capacity) {
            return false;
        }
        if (needed == 1U) {
            destination[output++] = static_cast<char>(value);
        } else {
            destination[output++] = '%';
            destination[output++] = kHex[value >> 4U];
            destination[output++] = kHex[value & 0x0FU];
        }
    }
    destination[output] = '\0';
    return true;
}

const cJSON *object_item(const cJSON *object, const char *name)
{
    return object == nullptr ? nullptr
                             : cJSON_GetObjectItemCaseSensitive(object, name);
}

bool json_uint(const cJSON *item, uint16_t &value)
{
    if (cJSON_IsNumber(item) && item->valuedouble >= 0.0 &&
        item->valuedouble <= 65535.0) {
        value = static_cast<uint16_t>(item->valueint);
        return true;
    }
    if (cJSON_IsString(item) && item->valuestring != nullptr) {
        char *end = nullptr;
        const unsigned long parsed = std::strtoul(item->valuestring, &end, 10);
        if (item->valuestring != end && end != nullptr && *end == '\0' &&
            parsed <= 65535UL) {
            value = static_cast<uint16_t>(parsed);
            return true;
        }
    }
    return false;
}

bool json_u8(const cJSON *item, uint8_t &value)
{
    uint16_t parsed = 0U;
    if (!json_uint(item, parsed) || parsed > 255U) {
        return false;
    }
    value = static_cast<uint8_t>(parsed);
    return true;
}

bool copy_json_string(const cJSON *item, char *destination, size_t capacity)
{
    if (!cJSON_IsString(item) || item->valuestring == nullptr ||
        std::strlen(item->valuestring) >= capacity) {
        return false;
    }
    std::strncpy(destination, item->valuestring, capacity - 1U);
    destination[capacity - 1U] = '\0';
    return true;
}

bool parse_timing(const cJSON *timings,
                  const char *name,
                  uint16_t &minutes)
{
    const cJSON *value = object_item(timings, name);
    return cJSON_IsString(value) && value->valuestring != nullptr &&
           prayer_parse_hhmm(value->valuestring, minutes);
}

bool parse_day(const cJSON *item, PrayerDay &day)
{
    const cJSON *timings = object_item(item, "timings");
    const cJSON *date = object_item(item, "date");
    const cJSON *gregorian = object_item(date, "gregorian");
    const cJSON *hijri = object_item(date, "hijri");
    const cJSON *gregorian_month = object_item(gregorian, "month");
    const cJSON *hijri_month = object_item(hijri, "month");
    const cJSON *weekday = object_item(gregorian, "weekday");
    if (!cJSON_IsObject(timings) || !cJSON_IsObject(gregorian) ||
        !cJSON_IsObject(hijri)) {
        return false;
    }

    day = {};
    if (!parse_timing(timings,
                      "Fajr",
                      day.minutes[static_cast<size_t>(PrayerTimeSlot::Fajr)]) ||
        !parse_timing(timings,
                      "Sunrise",
                      day.minutes[static_cast<size_t>(PrayerTimeSlot::Sunrise)]) ||
        !parse_timing(timings,
                      "Dhuhr",
                      day.minutes[static_cast<size_t>(PrayerTimeSlot::Dhuhr)]) ||
        !parse_timing(timings,
                      "Asr",
                      day.minutes[static_cast<size_t>(PrayerTimeSlot::Asr)]) ||
        !parse_timing(timings,
                      "Maghrib",
                      day.minutes[static_cast<size_t>(PrayerTimeSlot::Maghrib)]) ||
        !parse_timing(timings,
                      "Isha",
                      day.minutes[static_cast<size_t>(PrayerTimeSlot::Isha)])) {
        return false;
    }

    if (!json_uint(object_item(gregorian, "year"), day.date.year) ||
        !json_u8(object_item(gregorian, "day"), day.date.day) ||
        !json_u8(object_item(gregorian_month, "number"), day.date.month) ||
        !json_uint(object_item(hijri, "year"), day.hijri_year) ||
        !json_u8(object_item(hijri, "day"), day.hijri_day) ||
        !json_u8(object_item(hijri_month, "number"), day.hijri_month) ||
        !copy_json_string(object_item(hijri_month, "en"),
                          day.hijri_month_name,
                          sizeof(day.hijri_month_name)) ||
        !copy_json_string(object_item(weekday, "en"),
                          day.weekday,
                          sizeof(day.weekday))) {
        return false;
    }
    return prayer_day_valid(day);
}

esp_err_t parse_calendar(const char *json, PrayerSchedule &schedule)
{
    cJSON *root = cJSON_Parse(json);
    if (root == nullptr) {
        return ESP_ERR_INVALID_RESPONSE;
    }
    const cJSON *code = object_item(root, "code");
    const cJSON *data = object_item(root, "data");
    if (!cJSON_IsNumber(code) || code->valueint != 200 || !cJSON_IsArray(data)) {
        cJSON_Delete(root);
        return ESP_ERR_INVALID_RESPONSE;
    }

    esp_err_t result = ESP_OK;
    const cJSON *item = nullptr;
    cJSON_ArrayForEach(item, data) {
        PrayerDay day = {};
        if (!parse_day(item, day) || !prayer_schedule_add(schedule, day)) {
            result = ESP_ERR_INVALID_RESPONSE;
            break;
        }
    }
    cJSON_Delete(root);
    return result;
}

esp_err_t parse_validation_response(const char *json)
{
    cJSON *root = cJSON_Parse(json);
    if (root == nullptr) {
        return ESP_ERR_INVALID_RESPONSE;
    }
    const cJSON *code = object_item(root, "code");
    const cJSON *data = object_item(root, "data");
    const cJSON *timings = object_item(data, "timings");
    const cJSON *meta = object_item(data, "meta");
    const cJSON *timezone = object_item(meta, "timezone");
    const cJSON *latitude = object_item(meta, "latitude");
    const cJSON *longitude = object_item(meta, "longitude");
    uint16_t minutes = 0U;
    const bool valid = cJSON_IsNumber(code) && code->valueint == 200 &&
                       cJSON_IsObject(timings) &&
                       cJSON_IsString(timezone) &&
                       timezone->valuestring != nullptr &&
                       timezone->valuestring[0] != '\0' &&
                       cJSON_IsNumber(latitude) &&
                       cJSON_IsNumber(longitude) &&
                       parse_timing(timings, "Fajr", minutes) &&
                       parse_timing(timings, "Dhuhr", minutes) &&
                       parse_timing(timings, "Asr", minutes) &&
                       parse_timing(timings, "Maghrib", minutes) &&
                       parse_timing(timings, "Isha", minutes);
    cJSON_Delete(root);
    return valid ? ESP_OK : ESP_ERR_INVALID_RESPONSE;
}

esp_err_t fetch_month(const SalahSettings &settings,
                      uint16_t year,
                      uint8_t month,
                      PrayerSchedule &schedule)
{
    char city[160] = {};
    char country[160] = {};
    if (!url_encode(settings.city, city, sizeof(city)) ||
        !url_encode(settings.country, country, sizeof(country))) {
        return ESP_ERR_INVALID_ARG;
    }
    char url[512] = {};
    const int url_length = std::snprintf(
        url,
        sizeof(url),
        "https://api.aladhan.com/v1/calendarByCity/%u/%u?city=%s&country=%s&method=%u&school=%u",
        static_cast<unsigned>(year),
        static_cast<unsigned>(month),
        city,
        country,
        static_cast<unsigned>(settings.calculation_method),
        static_cast<unsigned>(settings.asr_school));
    if (url_length <= 0 || static_cast<size_t>(url_length) >= sizeof(url)) {
        return ESP_ERR_INVALID_SIZE;
    }

    ResponseBuffer response = {};
    response.data = static_cast<char *>(heap_caps_malloc(
        kResponseCapacity, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (response.data == nullptr) {
        return ESP_ERR_NO_MEM;
    }
    response.capacity = kResponseCapacity;
    response.data[0] = '\0';

    esp_http_client_config_t config = {};
    config.url = url;
    config.event_handler = http_event_handler;
    config.user_data = &response;
    config.crt_bundle_attach = esp_crt_bundle_attach;
    config.timeout_ms = 20000;
    config.buffer_size = 2048;
    config.buffer_size_tx = 1024;
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == nullptr) {
        heap_caps_free(response.data);
        return ESP_ERR_NO_MEM;
    }

    esp_err_t result = esp_http_client_perform(client);
    const int status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);
    if (result == ESP_OK && status == 200 && !response.overflow) {
        result = parse_calendar(response.data, schedule);
    } else if (result == ESP_OK) {
        result = response.overflow ? ESP_ERR_NO_MEM : ESP_ERR_HTTP_FETCH_HEADER;
    }
    heap_caps_free(response.data);
    STICKY_LOGI(kTag,
                "calendar=fetch year=%u month=%u status=%d days=%u result=%s",
                static_cast<unsigned>(year),
                static_cast<unsigned>(month),
                status,
                static_cast<unsigned>(schedule.count),
                esp_err_to_name(result));
    return result;
}

}  // namespace

esp_err_t aladhan_validate_location(const SalahSettings &settings)
{
    app_log_register_tag(kTag);
    if (!settings_store_valid(settings)) {
        return ESP_ERR_INVALID_ARG;
    }
    char city[160] = {};
    char country[160] = {};
    if (!url_encode(settings.city, city, sizeof(city)) ||
        !url_encode(settings.country, country, sizeof(country))) {
        return ESP_ERR_INVALID_ARG;
    }
    char url[512] = {};
    const int url_length = std::snprintf(
        url,
        sizeof(url),
        "https://api.aladhan.com/v1/timingsByCity?city=%s&country=%s&method=%u&school=%u",
        city,
        country,
        static_cast<unsigned>(settings.calculation_method),
        static_cast<unsigned>(settings.asr_school));
    if (url_length <= 0 || static_cast<size_t>(url_length) >= sizeof(url)) {
        return ESP_ERR_INVALID_SIZE;
    }

    ResponseBuffer response = {};
    response.data = static_cast<char *>(heap_caps_malloc(
        kValidationResponseCapacity, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (response.data == nullptr) {
        return ESP_ERR_NO_MEM;
    }
    response.capacity = kValidationResponseCapacity;
    response.data[0] = '\0';
    esp_http_client_config_t config = {};
    config.url = url;
    config.event_handler = http_event_handler;
    config.user_data = &response;
    config.crt_bundle_attach = esp_crt_bundle_attach;
    config.timeout_ms = 15000;
    config.buffer_size = 2048;
    config.buffer_size_tx = 1024;
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == nullptr) {
        heap_caps_free(response.data);
        return ESP_ERR_NO_MEM;
    }
    esp_err_t result = esp_http_client_perform(client);
    const int status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);
    if (result == ESP_OK && status == 200 && !response.overflow) {
        result = parse_validation_response(response.data);
    } else if (result == ESP_OK) {
        result = status >= 400 && status < 500
                     ? ESP_ERR_INVALID_RESPONSE
                     : ESP_ERR_HTTP_FETCH_HEADER;
    }
    heap_caps_free(response.data);
    STICKY_LOGI(kTag,
                "location=validate status=%d result=%s",
                status,
                esp_err_to_name(result));
    return result;
}

esp_err_t aladhan_fetch_two_month_schedule(const SalahSettings &settings,
                                           const PrayerDate &current_date,
                                           PrayerSchedule &schedule)
{
    app_log_register_tag(kTag);
    if (!settings_store_valid(settings) || !prayer_date_valid(current_date)) {
        return ESP_ERR_INVALID_ARG;
    }
    prayer_schedule_clear(schedule);
    esp_err_t result = fetch_month(settings,
                                   current_date.year,
                                   current_date.month,
                                   schedule);
    if (result != ESP_OK) {
        return result;
    }
    uint16_t next_year = 0U;
    uint8_t next_month = 0U;
    prayer_next_month(current_date.year,
                      current_date.month,
                      next_year,
                      next_month);
    result = fetch_month(settings, next_year, next_month, schedule);
    return result;
}
