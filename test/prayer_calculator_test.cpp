#include <cassert>
#include <cmath>
#include <cstring>

#include "prayer_calculator.h"

int main()
{
    SalahSettings settings = {};
    settings_store_defaults(settings);
    settings.configured = true;
    settings.location_confirmed = true;
    std::strcpy(settings.wifi_ssid, "Home");

    PrayerDay day = {};
    assert(prayer_calculate_day(settings, {2026U, 8U, 28U}, day));
    assert(prayer_day_valid(day));

    const uint16_t fajr = day.minutes[
        static_cast<size_t>(PrayerTimeSlot::Fajr)];
    const uint16_t sunrise = day.minutes[
        static_cast<size_t>(PrayerTimeSlot::Sunrise)];
    const uint16_t dhuhr = day.minutes[
        static_cast<size_t>(PrayerTimeSlot::Dhuhr)];
    const uint16_t asr = day.minutes[
        static_cast<size_t>(PrayerTimeSlot::Asr)];
    const uint16_t maghrib = day.minutes[
        static_cast<size_t>(PrayerTimeSlot::Maghrib)];
    const uint16_t isha = day.minutes[
        static_cast<size_t>(PrayerTimeSlot::Isha)];
    const uint16_t qiyam = day.minutes[
        static_cast<size_t>(PrayerTimeSlot::Qiyam)];

    assert(fajr >= 4U * 60U && fajr <= 6U * 60U);
    assert(sunrise > fajr && sunrise < 7U * 60U);
    assert(dhuhr >= 11U * 60U && dhuhr <= 14U * 60U);
    assert(asr > dhuhr && asr < maghrib);
    assert(maghrib >= 17U * 60U && maghrib <= 20U * 60U);
    assert(isha > maghrib);
    assert(qiyam > isha && qiyam < 2U * 24U * 60U);
    assert(std::strcmp(day.weekday, "Friday") == 0);

    PrayerDay next_day = {};
    assert(prayer_calculate_day(settings, {2026U, 8U, 29U}, next_day));
    const uint16_t next_fajr = next_day.minutes[
        static_cast<size_t>(PrayerTimeSlot::Fajr)];
    const uint16_t expected_qiyam = static_cast<uint16_t>(
        maghrib + (2U * (24U * 60U + next_fajr - maghrib) + 1U) / 3U);
    assert(qiyam == expected_qiyam);

    SalahSettings standard = settings;
    standard.asr_school = 0U;
    PrayerDay standard_day = {};
    assert(prayer_calculate_day(standard, {2026U, 8U, 28U}, standard_day));
    assert(day.minutes[static_cast<size_t>(PrayerTimeSlot::Asr)] >
           standard_day.minutes[static_cast<size_t>(PrayerTimeSlot::Asr)]);

    SalahSettings adjusted = settings;
    adjusted.prayer_adjustments[
        static_cast<size_t>(PrayerAdjustment::Fajr)] = 3;
    adjusted.prayer_adjustments[
        static_cast<size_t>(PrayerAdjustment::Qiyam)] = -2;
    PrayerDay adjusted_day = {};
    assert(prayer_calculate_day(adjusted, {2026U, 8U, 28U}, adjusted_day));
    assert(adjusted_day.minutes[static_cast<size_t>(PrayerTimeSlot::Fajr)] ==
           fajr + 3U);
    assert(adjusted_day.minutes[static_cast<size_t>(PrayerTimeSlot::Qiyam)] ==
           qiyam);

    SalahSettings invalid = settings;
    invalid.latitude = 95.0;
    assert(!prayer_calculate_day(invalid, {2026U, 8U, 28U}, day));
    return 0;
}
