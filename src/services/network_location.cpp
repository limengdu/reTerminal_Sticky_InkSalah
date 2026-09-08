#include "network_location.h"

#include <cmath>
#include <cstring>

#include "app_log.h"
#include "cJSON.h"
#include "esp_crt_bundle.h"
#include "esp_http_client.h"

namespace {

constexpr char kTag[] = "network_location";
constexpr char kUrl[] =
    "https://ipwho.is/?fields=success,city,country,latitude,longitude,timezone";
constexpr size_t kResponseCapacity = 4096U;

struct ResponseBuffer {
    char data[kResponseCapacity] = {};
    size_t length = 0U;
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
    if (buffer->length + incoming + 1U > sizeof(buffer->data)) {
        buffer->overflow = true;
        return ESP_ERR_NO_MEM;
    }
    std::memcpy(buffer->data + buffer->length, event->data, incoming);
    buffer->length += incoming;
    buffer->data[buffer->length] = '\0';
    return ESP_OK;
}

bool copy_string(const cJSON *object,
                 const char *name,
                 char *output,
                 size_t capacity)
{
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(object, name);
    if (!cJSON_IsString(item) || item->valuestring == nullptr ||
        std::strlen(item->valuestring) >= capacity) {
        return false;
    }
    std::strncpy(output, item->valuestring, capacity - 1U);
    output[capacity - 1U] = '\0';
    return true;
}

}  // namespace

esp_err_t network_location_estimate(NetworkLocationEstimate &estimate)
{
    app_log_register_tag(kTag);
    estimate = {};
    ResponseBuffer response = {};
    esp_http_client_config_t config = {};
    config.url = kUrl;
    config.user_agent = "Sticky-Salah/0.1.1";
    config.event_handler = http_event_handler;
    config.user_data = &response;
    config.crt_bundle_attach = esp_crt_bundle_attach;
    config.timeout_ms = 12000;
    config.buffer_size = 2048;
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == nullptr) {
        return ESP_ERR_NO_MEM;
    }
    const esp_err_t request_result = esp_http_client_perform(client);
    const int status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);
    if (request_result != ESP_OK || status != 200 || response.overflow) {
        STICKY_LOGW(kTag,
                    "location=estimate status=%d result=%s",
                    status,
                    esp_err_to_name(request_result));
        return request_result == ESP_OK ? ESP_ERR_INVALID_RESPONSE
                                        : request_result;
    }

    cJSON *root = cJSON_Parse(response.data);
    if (!cJSON_IsObject(root)) {
        cJSON_Delete(root);
        return ESP_ERR_INVALID_RESPONSE;
    }
    const cJSON *success = cJSON_GetObjectItemCaseSensitive(root, "success");
    const cJSON *latitude = cJSON_GetObjectItemCaseSensitive(root, "latitude");
    const cJSON *longitude = cJSON_GetObjectItemCaseSensitive(root, "longitude");
    const cJSON *timezone = cJSON_GetObjectItemCaseSensitive(root, "timezone");
    const cJSON *offset = cJSON_IsObject(timezone)
                              ? cJSON_GetObjectItemCaseSensitive(
                                    timezone, "offset")
                              : nullptr;
    const bool valid = cJSON_IsTrue(success) && cJSON_IsNumber(latitude) &&
                       cJSON_IsNumber(longitude) && cJSON_IsNumber(offset) &&
                       std::isfinite(latitude->valuedouble) &&
                       std::isfinite(longitude->valuedouble) &&
                       latitude->valuedouble >= -90.0 &&
                       latitude->valuedouble <= 90.0 &&
                       longitude->valuedouble >= -180.0 &&
                       longitude->valuedouble <= 180.0 &&
                       offset->valueint >= -12 * 60 * 60 &&
                       offset->valueint <= 14 * 60 * 60 &&
                       copy_string(root,
                                   "city",
                                   estimate.city,
                                   sizeof(estimate.city)) &&
                       copy_string(root,
                                   "country",
                                   estimate.country,
                                   sizeof(estimate.country));
    if (valid) {
        estimate.latitude = latitude->valuedouble;
        estimate.longitude = longitude->valuedouble;
        estimate.utc_offset_minutes = static_cast<int16_t>(
            offset->valueint / 60);
        estimate.valid = true;
    }
    cJSON_Delete(root);
    STICKY_LOGI(kTag,
                "location=estimate city=%s country=%s result=%s",
                estimate.city,
                estimate.country,
                valid ? "ok" : "invalid");
    return valid ? ESP_OK : ESP_ERR_INVALID_RESPONSE;
}
