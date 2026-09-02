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

    assert(settings_calculation_method_count() == 25U);
    assert(std::strcmp(settings_method_short_name(2U), "ISNA") == 0);
    assert(std::strcmp(settings_method_short_name(11U), "MUIS") == 0);
    assert(std::strcmp(settings_method_short_name(12U), "UOIF") == 0);
    assert(std::strcmp(settings_method_short_name(13U), "DIYANET") == 0);
    assert(std::strcmp(settings_method_short_name(20U), "KEMENAG") == 0);

    SalahSettings isna = settings;
    isna.calculation_method = 2U;
    PrayerDay isna_day = {};
    assert(prayer_calculate_day(isna, {2026U, 8U, 28U}, isna_day));
    SalahSettings custom_angles = settings;
    custom_angles.calculation_method = kCalculationMethodCustomAngles;
    custom_angles.custom_fajr_angle_tenths = 150U;
    custom_angles.custom_isha_angle_tenths = 150U;
    PrayerDay custom_angles_day = {};
    assert(prayer_calculate_day(custom_angles,
                                {2026U, 8U, 28U},
                                custom_angles_day));
    assert(custom_angles_day.minutes[
               static_cast<size_t>(PrayerTimeSlot::Fajr)] ==
           isna_day.minutes[static_cast<size_t>(PrayerTimeSlot::Fajr)]);
    assert(custom_angles_day.minutes[
               static_cast<size_t>(PrayerTimeSlot::Isha)] ==
           isna_day.minutes[static_cast<size_t>(PrayerTimeSlot::Isha)]);

    SalahSettings qatar = settings;
    qatar.calculation_method = 10U;
    PrayerDay qatar_day = {};
    assert(prayer_calculate_day(qatar, {2026U, 8U, 28U}, qatar_day));
    SalahSettings custom_interval = settings;
    custom_interval.calculation_method = kCalculationMethodCustomInterval;
    custom_interval.custom_fajr_angle_tenths = 180U;
    custom_interval.custom_isha_interval_minutes = 90U;
    PrayerDay custom_interval_day = {};
    assert(prayer_calculate_day(custom_interval,
                                {2026U, 8U, 28U},
                                custom_interval_day));
    assert(custom_interval_day.minutes[
               static_cast<size_t>(PrayerTimeSlot::Fajr)] ==
           qatar_day.minutes[static_cast<size_t>(PrayerTimeSlot::Fajr)]);
    assert(custom_interval_day.minutes[
               static_cast<size_t>(PrayerTimeSlot::Isha)] ==
           qatar_day.minutes[static_cast<size_t>(PrayerTimeSlot::Isha)]);

    SalahSettings moonsighting = settings;
    moonsighting.latitude = 59.9139;
    moonsighting.longitude = 10.7522;
    moonsighting.utc_offset_minutes = 120;
    moonsighting.calculation_method = 15U;
    PrayerDay moonsighting_day = {};
    assert(prayer_calculate_day(moonsighting,
                                {2026U, 6U, 15U},
                                moonsighting_day));
    assert(moonsighting_day.minutes[
               static_cast<size_t>(PrayerTimeSlot::Fajr)] <
           moonsighting_day.minutes[
               static_cast<size_t>(PrayerTimeSlot::Sunrise)]);
    assert(moonsighting_day.minutes[
               static_cast<size_t>(PrayerTimeSlot::Maghrib)] <
           moonsighting_day.minutes[
               static_cast<size_t>(PrayerTimeSlot::Isha)]);

    SalahSettings invalid_custom = settings;
    invalid_custom.calculation_method = kCalculationMethodCustomAngles;
    invalid_custom.custom_fajr_angle_tenths = 0U;
    assert(!settings_store_valid(invalid_custom));
    return 0;
}
