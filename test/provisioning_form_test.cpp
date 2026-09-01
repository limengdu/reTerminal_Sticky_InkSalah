#include <cassert>
#include <cstring>

#include "provisioning_form.h"

int main()
{
    SalahSettings settings = {};
    char error[128] = {};
    assert(provisioning_wifi_form_parse(
        "ssid=Home+WiFi&password=safe%21pass",
        settings, error, sizeof(error)));
    assert(std::strcmp(settings.wifi_ssid, "Home WiFi") == 0);
    assert(std::strcmp(settings.wifi_password, "safe!pass") == 0);
    assert(!settings.configured);
    assert(!settings.location_confirmed);

    SalahSettings hidden = {};
    assert(provisioning_wifi_form_parse(
        "ssid=HiddenNet&password=", hidden, error, sizeof(error)));
    assert(std::strcmp(hidden.wifi_ssid, "HiddenNet") == 0);
    assert(hidden.wifi_password[0] == '\0');

    assert(!provisioning_wifi_form_parse(
        "ssid=&password=test", settings, error, sizeof(error)));
    assert(error[0] != '\0');
    assert(!provisioning_wifi_form_parse(
        "ssid=Bad%ZZ&password=test", settings, error, sizeof(error)));
    return 0;
}
