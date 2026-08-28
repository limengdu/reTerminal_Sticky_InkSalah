#pragma once

#include <cstddef>
#include <cstdint>

constexpr size_t kWifiScanMaximumNetworks = 20U;

struct WifiScanItem {
    char ssid[33] = {};
    int8_t rssi = -127;
    bool secure = false;
};

struct WifiScanList {
    WifiScanItem items[kWifiScanMaximumNetworks] = {};
    size_t count = 0U;
};

bool wifi_scan_list_add(WifiScanList &list,
                        const char *ssid,
                        int8_t rssi,
                        bool secure);
void wifi_scan_list_sort(WifiScanList &list);
