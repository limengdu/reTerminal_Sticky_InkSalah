#include <cassert>
#include <cstring>

#include "provisioning_form.h"

int main()
{
    int16_t offset = 0;
    assert(provisioning_parse_utc_offset("+07:00", offset));
    assert(offset == 420);
    assert(provisioning_parse_utc_offset("-04:30", offset));
    assert(offset == -270);
    assert(!provisioning_parse_utc_offset("+15:00", offset));
    assert(!provisioning_parse_utc_offset("7:00", offset));

    SalahSettings settings = {};
    char error[128] = {};
    const char *valid =
        "ssid=Home+WiFi&password=safe%21pass&city=Jakarta&country=Indonesia&utc_offset=%2B07%3A00&method=20&school=0&sound=on";
    assert(provisioning_form_parse(valid, settings, error, sizeof(error)));
    assert(std::strcmp(settings.wifi_ssid, "Home WiFi") == 0);
    assert(std::strcmp(settings.wifi_password, "safe!pass") == 0);
    assert(std::strcmp(settings.city, "Jakarta") == 0);
    assert(settings.utc_offset_minutes == 420);
    assert(settings.calculation_method == 20U);
    assert(settings.asr_school == 0U);
    assert(settings.sound_enabled);

    const char *missing_city =
        "ssid=Home&password=&country=Indonesia&utc_offset=%2B07%3A00&method=20&school=0";
    assert(!provisioning_form_parse(
        missing_city, settings, error, sizeof(error)));
    assert(error[0] != '\0');
    return 0;
}
