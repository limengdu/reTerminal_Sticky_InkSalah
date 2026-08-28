#include <cassert>
#include <cstring>

#include "wifi_scan_model.h"

int main()
{
    WifiScanList list = {};
    assert(wifi_scan_list_add(list, "Home", -62, true));
    assert(wifi_scan_list_add(list, "Cafe", -81, false));
    assert(wifi_scan_list_add(list, "Home", -45, true));
    assert(!wifi_scan_list_add(list, "", -30, false));

    wifi_scan_list_sort(list);
    assert(list.count == 2U);
    assert(std::strcmp(list.items[0].ssid, "Home") == 0);
    assert(list.items[0].rssi == -45);
    assert(list.items[0].secure);
    assert(std::strcmp(list.items[1].ssid, "Cafe") == 0);
    return 0;
}
