#include "wifi_scan_model.h"

#include <algorithm>
#include <cstring>

bool wifi_scan_list_add(WifiScanList &list,
                        const char *ssid,
                        int8_t rssi,
                        bool secure)
{
    if (ssid == nullptr || ssid[0] == '\0' || std::strlen(ssid) > 32U) {
        return false;
    }
    for (size_t index = 0U; index < list.count; ++index) {
        WifiScanItem &item = list.items[index];
        if (std::strcmp(item.ssid, ssid) == 0) {
            if (rssi > item.rssi) {
                item.rssi = rssi;
                item.secure = secure;
            }
            return true;
        }
    }
    if (list.count >= kWifiScanMaximumNetworks) {
        return false;
    }
    WifiScanItem &item = list.items[list.count++];
    std::strncpy(item.ssid, ssid, sizeof(item.ssid) - 1U);
    item.rssi = rssi;
    item.secure = secure;
    return true;
}

void wifi_scan_list_sort(WifiScanList &list)
{
    std::sort(list.items,
              list.items + list.count,
              [](const WifiScanItem &left, const WifiScanItem &right) {
                  return left.rssi > right.rssi;
              });
}
