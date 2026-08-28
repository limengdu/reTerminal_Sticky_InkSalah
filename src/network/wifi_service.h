#pragma once

#include <cstddef>
#include <cstdint>

#include "esp_err.h"
#include "settings_model.h"
#include "wifi_scan_model.h"

esp_err_t wifi_service_init();
esp_err_t wifi_service_connect(const SalahSettings &settings,
                               uint32_t timeout_ms);
esp_err_t wifi_service_start_setup_ap(char *ssid,
                                      size_t ssid_capacity);
esp_err_t wifi_service_scan_networks(WifiScanList &list);
esp_err_t wifi_service_validate_setup_connection(
    const SalahSettings &settings,
    uint32_t timeout_ms);
esp_err_t wifi_service_stop();
bool wifi_service_is_connected();

constexpr char kStickySalahSetupPassword[] = "sticky-salah";
