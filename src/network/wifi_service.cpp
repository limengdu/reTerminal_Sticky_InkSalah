#include "wifi_service.h"

#include <atomic>
#include <cstdio>
#include <cstring>

#include "app_log.h"
#include "esp_check.h"
#include "esp_event.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "setup_network_readiness.h"

namespace {

constexpr char kTag[] = "wifi_service";
constexpr EventBits_t kConnectedBit = BIT0;
constexpr EventBits_t kFailedBit = BIT1;
constexpr EventBits_t kSetupApStartedBit = BIT2;
constexpr uint8_t kMaxConnectAttempts = 4U;
constexpr uint32_t kSetupStartTimeoutMs = 5000U;
char kSetupPortalUri[] = "http://192.168.4.1/";

EventGroupHandle_t s_events = nullptr;
esp_netif_t *s_station_netif = nullptr;
esp_netif_t *s_ap_netif = nullptr;
bool s_initialized = false;
std::atomic<uint8_t> s_connect_attempts{0U};
std::atomic<bool> s_connected{false};

void wifi_event_handler(void *,
                        esp_event_base_t base,
                        int32_t id,
                        void *event_data)
{
    if (base == WIFI_EVENT && id == WIFI_EVENT_AP_START) {
        if (s_events != nullptr) {
            xEventGroupSetBits(s_events, kSetupApStartedBit);
        }
        STICKY_LOGI(kTag, "wifi=setup_ap state=started result=ok");
        return;
    }
    if (base == WIFI_EVENT && id == WIFI_EVENT_AP_STACONNECTED) {
        const auto *event = static_cast<wifi_event_ap_staconnected_t *>(
            event_data);
        STICKY_LOGI(kTag,
                    "wifi=setup_client state=connected mac=" MACSTR,
                    MAC2STR(event->mac));
        return;
    }
    if (base == WIFI_EVENT && id == WIFI_EVENT_AP_STADISCONNECTED) {
        const auto *event = static_cast<wifi_event_ap_stadisconnected_t *>(
            event_data);
        STICKY_LOGI(kTag,
                    "wifi=setup_client state=disconnected mac=" MACSTR
                    " reason=%u",
                    MAC2STR(event->mac),
                    static_cast<unsigned>(event->reason));
        return;
    }
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        s_connect_attempts.store(1U, std::memory_order_release);
        esp_wifi_connect();
        return;
    }
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        s_connected.store(false, std::memory_order_release);
        const uint8_t attempt = s_connect_attempts.fetch_add(
            1U, std::memory_order_acq_rel);
        if (attempt < kMaxConnectAttempts) {
            esp_wifi_connect();
        } else if (s_events != nullptr) {
            xEventGroupSetBits(s_events, kFailedBit);
        }
    }
}

void ip_event_handler(void *,
                      esp_event_base_t base,
                      int32_t id,
                      void *event_data)
{
    if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        s_connected.store(true, std::memory_order_release);
        if (s_events != nullptr) {
            xEventGroupSetBits(s_events, kConnectedBit);
        }
        return;
    }
    if (base == IP_EVENT && id == IP_EVENT_AP_STAIPASSIGNED) {
        const auto *event = static_cast<ip_event_ap_staipassigned_t *>(
            event_data);
        STICKY_LOGI(kTag,
                    "wifi=setup_client state=address_assigned ip=" IPSTR
                    " mac=" MACSTR,
                    IP2STR(&event->ip),
                    MAC2STR(event->mac));
    }
}

esp_err_t stop_wifi_if_running()
{
    const esp_err_t result = esp_wifi_stop();
    if (result == ESP_OK || result == ESP_ERR_WIFI_NOT_STARTED) {
        return ESP_OK;
    }
    return result;
}

}  // namespace

esp_err_t wifi_service_init()
{
    app_log_register_tag(kTag);
    if (s_initialized) {
        return ESP_OK;
    }
    esp_err_t result = esp_netif_init();
    if (result != ESP_OK && result != ESP_ERR_INVALID_STATE) {
        return result;
    }
    result = esp_event_loop_create_default();
    if (result != ESP_OK && result != ESP_ERR_INVALID_STATE) {
        return result;
    }
    s_events = xEventGroupCreate();
    if (s_events == nullptr) {
        return ESP_ERR_NO_MEM;
    }
    // Creates both default interfaces before Wi-Fi initialization, matching
    // the lifecycle used by ESP-IDF's official SoftAP examples.
    // 在Wi-Fi初始化前创建两个默认网络接口，与ESP-IDF官方SoftAP示例的生命周期保持一致。
    s_station_netif = esp_netif_create_default_wifi_sta();
    s_ap_netif = esp_netif_create_default_wifi_ap();
    if (s_station_netif == nullptr || s_ap_netif == nullptr) {
        return ESP_ERR_NO_MEM;
    }
    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(esp_wifi_init(&config), kTag, "initialize driver");
    ESP_RETURN_ON_ERROR(
        esp_event_handler_register(
            WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, nullptr),
        kTag,
        "register wifi events");
    ESP_RETURN_ON_ERROR(
        esp_event_handler_register(
            IP_EVENT, ESP_EVENT_ANY_ID, ip_event_handler, nullptr),
        kTag,
        "register ip events");
    ESP_RETURN_ON_ERROR(esp_wifi_set_storage(WIFI_STORAGE_RAM),
                        kTag,
                        "set storage");
    s_initialized = true;
    STICKY_LOGI(kTag, "wifi=init result=ok");
    return ESP_OK;
}

esp_err_t wifi_service_connect(const SalahSettings &settings,
                               uint32_t timeout_ms)
{
    if (!settings_store_valid(settings)) {
        return ESP_ERR_INVALID_ARG;
    }
    ESP_RETURN_ON_ERROR(wifi_service_init(), kTag, "initialize wifi");
    ESP_RETURN_ON_ERROR(stop_wifi_if_running(), kTag, "stop previous mode");
    xEventGroupClearBits(s_events, kConnectedBit | kFailedBit);
    s_connect_attempts.store(0U, std::memory_order_release);
    s_connected.store(false, std::memory_order_release);

    wifi_config_t config = {};
    std::strncpy(reinterpret_cast<char *>(config.sta.ssid),
                 settings.wifi_ssid,
                 sizeof(config.sta.ssid) - 1U);
    std::strncpy(reinterpret_cast<char *>(config.sta.password),
                 settings.wifi_password,
                 sizeof(config.sta.password) - 1U);
    config.sta.threshold.authmode = settings.wifi_password[0] == '\0'
                                        ? WIFI_AUTH_OPEN
                                        : WIFI_AUTH_WPA2_PSK;
    config.sta.pmf_cfg.capable = true;
    config.sta.pmf_cfg.required = false;

    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_STA),
                        kTag,
                        "set station mode");
    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_STA, &config),
                        kTag,
                        "set station config");
    ESP_RETURN_ON_ERROR(esp_wifi_start(), kTag, "start station");

    const EventBits_t bits = xEventGroupWaitBits(
        s_events,
        kConnectedBit | kFailedBit,
        pdTRUE,
        pdFALSE,
        pdMS_TO_TICKS(timeout_ms));
    if ((bits & kConnectedBit) != 0U) {
        STICKY_LOGI(kTag, "wifi=connect result=ok");
        return ESP_OK;
    }
    STICKY_LOGW(kTag,
                "wifi=connect result=unavailable attempts=%u",
                static_cast<unsigned>(s_connect_attempts.load()));
    return ESP_ERR_TIMEOUT;
}

esp_err_t wifi_service_start_setup_ap(char *ssid,
                                      size_t ssid_capacity)
{
    if (ssid == nullptr || ssid_capacity < 20U) {
        return ESP_ERR_INVALID_ARG;
    }
    ESP_RETURN_ON_ERROR(wifi_service_init(), kTag, "initialize wifi");
    ESP_RETURN_ON_ERROR(stop_wifi_if_running(), kTag, "stop previous mode");
    xEventGroupClearBits(s_events, kSetupApStartedBit);

    uint8_t mac[6] = {};
    ESP_RETURN_ON_ERROR(esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP),
                        kTag,
                        "read access point mac");
    std::snprintf(ssid,
                  ssid_capacity,
                  "Sticky-Salah-%02X%02X",
                  mac[4],
                  mac[5]);

    wifi_config_t config = {};
    std::strncpy(reinterpret_cast<char *>(config.ap.ssid),
                 ssid,
                 sizeof(config.ap.ssid) - 1U);
    config.ap.ssid_len = std::strlen(ssid);
    std::strncpy(reinterpret_cast<char *>(config.ap.password),
                 kStickySalahSetupPassword,
                 sizeof(config.ap.password) - 1U);
    config.ap.channel = 1U;
    config.ap.max_connection = 4U;
    config.ap.authmode = WIFI_AUTH_WPA2_PSK;

    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_AP),
                        kTag,
                        "set access point mode");
    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_AP, &config),
                        kTag,
                        "set access point config");
    ESP_RETURN_ON_ERROR(esp_wifi_start(), kTag, "start access point");

    const EventBits_t bits = xEventGroupWaitBits(
        s_events,
        kSetupApStartedBit,
        pdFALSE,
        pdTRUE,
        pdMS_TO_TICKS(kSetupStartTimeoutMs));
    if ((bits & kSetupApStartedBit) == 0U) {
        STICKY_LOGW(kTag, "wifi=setup_ap state=start_timeout");
        return ESP_ERR_TIMEOUT;
    }

    esp_netif_dhcp_status_t dhcp_status = ESP_NETIF_DHCP_INIT;
    ESP_RETURN_ON_ERROR(
        esp_netif_dhcps_get_status(s_ap_netif, &dhcp_status),
        kTag,
        "read DHCP status");
    if (dhcp_status == ESP_NETIF_DHCP_STARTED) {
        ESP_RETURN_ON_ERROR(esp_netif_dhcps_stop(s_ap_netif),
                            kTag,
                            "pause DHCP server");
    }
    ESP_RETURN_ON_ERROR(
        esp_netif_dhcps_option(s_ap_netif,
                               ESP_NETIF_OP_SET,
                               ESP_NETIF_CAPTIVEPORTAL_URI,
                               kSetupPortalUri,
                               std::strlen(kSetupPortalUri)),
        kTag,
        "set captive portal URI");
    ESP_RETURN_ON_ERROR(esp_netif_dhcps_start(s_ap_netif),
                        kTag,
                        "start DHCP server");
    ESP_RETURN_ON_ERROR(
        esp_netif_dhcps_get_status(s_ap_netif, &dhcp_status),
        kTag,
        "verify DHCP status");

    esp_netif_ip_info_t ip_info = {};
    ESP_RETURN_ON_ERROR(esp_netif_get_ip_info(s_ap_netif, &ip_info),
                        kTag,
                        "read access point address");
    SetupNetworkReadiness readiness = {};
    readiness.ap_started = true;
    readiness.dhcp_server_running =
        dhcp_status == ESP_NETIF_DHCP_STARTED;
    readiness.expected_ip_configured =
        ip_info.ip.addr == ESP_IP4TOADDR(192, 168, 4, 1);
    if (!setup_network_ready(readiness)) {
        STICKY_LOGW(kTag,
                    "wifi=setup_ap state=not_ready dhcp=%d expected_ip=%d",
                    readiness.dhcp_server_running,
                    readiness.expected_ip_configured);
        return ESP_ERR_INVALID_STATE;
    }
    STICKY_LOGI(kTag,
                "wifi=setup_ap ssid=%s address=192.168.4.1 dhcp=started portal_uri=%s result=ok",
                ssid,
                kSetupPortalUri);
    return ESP_OK;
}

esp_err_t wifi_service_stop()
{
    if (!s_initialized) {
        return ESP_OK;
    }
    s_connected.store(false, std::memory_order_release);
    return stop_wifi_if_running();
}

bool wifi_service_is_connected()
{
    return s_connected.load(std::memory_order_acquire);
}
