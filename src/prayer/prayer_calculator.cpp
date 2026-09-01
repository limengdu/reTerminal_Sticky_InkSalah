#include "prayer_calculator.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kMinutesPerDay = 1440.0;

struct CalculationProfile {
    double fajr_angle = 18.0;
    double isha_angle = 17.0;
    uint16_t isha_interval_minutes = 0U;
};

struct SolarParameters {
    double equation_of_time_minutes = 0.0;
    double declination_radians = 0.0;
};

double radians(double degrees)
{
    return degrees * kPi / 180.0;
}

double degrees(double radians_value)
{
    return radians_value * 180.0 / kPi;
}

double normalized_minutes(double value)
{
    value = std::fmod(value, kMinutesPerDay);
    return value < 0.0 ? value + kMinutesPerDay : value;
}

bool rounded_clock_minutes(double value, uint16_t &minutes)
{
    if (!std::isfinite(value)) {
        return false;
    }
    const long rounded = std::lround(normalized_minutes(value));
    minutes = static_cast<uint16_t>(rounded == 1440L ? 0L : rounded);
    return true;
}

uint16_t day_of_year(const PrayerDate &date)
{
    constexpr uint16_t kCumulativeDays[] = {
        0U, 31U, 59U, 90U, 120U, 151U,
        181U, 212U, 243U, 273U, 304U, 334U,
    };
    const bool leap = (date.year % 4U == 0U && date.year % 100U != 0U) ||
                      date.year % 400U == 0U;
    return static_cast<uint16_t>(
        kCumulativeDays[date.month - 1U] + date.day +
        (leap && date.month > 2U ? 1U : 0U));
}

SolarParameters solar_parameters(const PrayerDate &date)
{
    const double gamma = 2.0 * kPi / 365.0 *
                         (static_cast<double>(day_of_year(date)) - 1.0);
    SolarParameters value = {};
    value.equation_of_time_minutes = 229.18 *
        (0.000075 + 0.001868 * std::cos(gamma) -
         0.032077 * std::sin(gamma) -
         0.014615 * std::cos(2.0 * gamma) -
         0.040849 * std::sin(2.0 * gamma));
    value.declination_radians =
        0.006918 - 0.399912 * std::cos(gamma) +
        0.070257 * std::sin(gamma) -
        0.006758 * std::cos(2.0 * gamma) +
        0.000907 * std::sin(2.0 * gamma) -
        0.002697 * std::cos(3.0 * gamma) +
        0.00148 * std::sin(3.0 * gamma);
    return value;
}

double hour_angle_degrees(double latitude,
                          double declination,
                          double solar_elevation_degrees)
{
    const double latitude_radians = radians(latitude);
    const double numerator =
        std::sin(radians(solar_elevation_degrees)) -
        std::sin(latitude_radians) * std::sin(declination);
    const double denominator =
        std::cos(latitude_radians) * std::cos(declination);
    if (std::fabs(denominator) < 1.0e-12) {
        return NAN;
    }
    const double cosine = numerator / denominator;
    if (cosine < -1.0 || cosine > 1.0) {
        return NAN;
    }
    return degrees(std::acos(cosine));
}

CalculationProfile calculation_profile(uint8_t method)
{
    switch (method) {
    case 0U:
        return {16.0, 14.0, 0U};
    case 1U:
        return {18.0, 18.0, 0U};
    case 2U:
        return {15.0, 15.0, 0U};
    case 4U:
        return {18.5, 0.0, 90U};
    case 5U:
        return {19.5, 17.5, 0U};
    case 7U:
        return {17.7, 14.0, 0U};
    case 8U:
        return {19.5, 0.0, 90U};
    case 9U:
        return {18.0, 17.5, 0U};
    case 10U:
        return {18.0, 0.0, 90U};
    case 11U:
        return {20.0, 18.0, 0U};
    case 12U:
        return {12.0, 12.0, 0U};
    case 13U:
        return {18.0, 17.0, 0U};
    case 14U:
        return {16.0, 15.0, 0U};
    case 15U:
        return {18.0, 18.0, 0U};
    case 16U:
        return {18.2, 18.2, 0U};
    case 17U:
        return {20.0, 18.0, 0U};
    case 18U:
        return {18.0, 18.0, 0U};
    case 19U:
        return {18.0, 17.0, 0U};
    case 20U:
        return {20.0, 18.0, 0U};
    case 21U:
        return {19.0, 17.0, 0U};
    case 22U:
        return {18.0, 0.0, 77U};
    case 23U:
        return {18.0, 18.0, 0U};
    case 3U:
    default:
        return {18.0, 17.0, 0U};
    }
}

double high_latitude_fraction(double angle)
{
    return std::fmin(0.5, angle / 60.0);
}

bool calculate_six_times(const SalahSettings &settings,
                         const PrayerDate &date,
                         uint16_t output[6])
{
    const SolarParameters solar = solar_parameters(date);
    const CalculationProfile profile = calculation_profile(
        settings.calculation_method);
    const double noon = 720.0 - 4.0 * settings.longitude -
                        solar.equation_of_time_minutes +
                        settings.utc_offset_minutes;
    const double latitude = settings.latitude;
    const double sunrise_hour_angle = hour_angle_degrees(
        latitude, solar.declination_radians, -0.833);
    if (!std::isfinite(sunrise_hour_angle)) {
        return false;
    }
    const double sunrise = noon - 4.0 * sunrise_hour_angle;
    const double sunset = noon + 4.0 * sunrise_hour_angle;

    double fajr_hour_angle = hour_angle_degrees(
        latitude, solar.declination_radians, -profile.fajr_angle);
    double isha_hour_angle = profile.isha_interval_minutes == 0U
                                 ? hour_angle_degrees(
                                       latitude,
                                       solar.declination_radians,
                                       -profile.isha_angle)
                                 : 0.0;
    double fajr = std::isfinite(fajr_hour_angle)
                      ? noon - 4.0 * fajr_hour_angle
                      : NAN;
    double isha = profile.isha_interval_minutes != 0U
                      ? sunset + profile.isha_interval_minutes
                      : (std::isfinite(isha_hour_angle)
                             ? noon + 4.0 * isha_hour_angle
                             : NAN);

    // Caps twilight spans to an angle-based fraction of the night.
    // 将晨昏时间限制在按角度划分的夜长范围内。
    const double night_length = kMinutesPerDay - (sunset - sunrise);
    const double fajr_limit = high_latitude_fraction(profile.fajr_angle) *
                              night_length;
    if (!std::isfinite(fajr) || sunrise - fajr > fajr_limit) {
        fajr = sunrise - fajr_limit;
    }
    if (profile.isha_interval_minutes == 0U) {
        const double isha_limit = high_latitude_fraction(profile.isha_angle) *
                                  night_length;
        if (!std::isfinite(isha) || isha - sunset > isha_limit) {
            isha = sunset + isha_limit;
        }
    }

    const double latitude_radians = radians(latitude);
    const double shadow_factor = settings.asr_school == 1U ? 2.0 : 1.0;
    const double noon_shadow = std::tan(std::fabs(
        latitude_radians - solar.declination_radians));
    const double asr_elevation = degrees(std::atan(
        1.0 / (shadow_factor + noon_shadow)));
    const double asr_hour_angle = hour_angle_degrees(
        latitude, solar.declination_radians, asr_elevation);
    if (!std::isfinite(asr_hour_angle)) {
        return false;
    }

    const double values[] = {
        fajr,
        sunrise,
        noon,
        noon + 4.0 * asr_hour_angle,
        sunset,
        isha,
    };
    for (size_t index = 0U; index < 6U; ++index) {
        const int adjustment = settings.prayer_adjustments[index];
        if (!rounded_clock_minutes(values[index] + adjustment,
                                   output[index])) {
            return false;
        }
    }
    return true;
}

int64_t gregorian_julian_day(const PrayerDate &date)
{
    const int64_t a = (14 - date.month) / 12;
    const int64_t y = static_cast<int64_t>(date.year) + 4800 - a;
    const int64_t m = static_cast<int64_t>(date.month) + 12 * a - 3;
    return date.day + (153 * m + 2) / 5 + 365 * y + y / 4 -
           y / 100 + y / 400 - 32045;
}

void fill_calendar_labels(const PrayerDate &date, PrayerDay &day)
{
    const int64_t julian_day = gregorian_julian_day(date);
    int64_t value = julian_day - 1948440 + 10632;
    const int64_t cycle = (value - 1) / 10631;
    value = value - 10631 * cycle + 354;
    const int64_t correction =
        ((10985 - value) / 5316) * ((50 * value) / 17719) +
        (value / 5670) * ((43 * value) / 15238);
    value = value - ((30 - correction) / 15) *
                        ((17719 * correction) / 50) -
            (correction / 16) * ((15238 * correction) / 43) + 29;
    const int64_t month = (24 * value) / 709;
    const int64_t hijri_day = value - (709 * month) / 24;
    const int64_t hijri_year = 30 * cycle + correction - 30;
    day.hijri_year = static_cast<uint16_t>(hijri_year);
    day.hijri_month = static_cast<uint8_t>(month);
    day.hijri_day = static_cast<uint8_t>(hijri_day);
    std::strncpy(day.hijri_month_name,
                 prayer_hijri_month_name(day.hijri_month),
                 sizeof(day.hijri_month_name) - 1U);

    constexpr const char *kWeekdays[] = {
        "Monday", "Tuesday", "Wednesday", "Thursday",
        "Friday", "Saturday", "Sunday",
    };
    const size_t weekday = static_cast<size_t>(julian_day % 7);
    std::strncpy(day.weekday,
                 kWeekdays[weekday],
                 sizeof(day.weekday) - 1U);
}

}  // namespace

bool prayer_calculate_day(const SalahSettings &settings,
                          const PrayerDate &date,
                          PrayerDay &day)
{
    if (!settings_store_valid(settings) || !prayer_date_valid(date)) {
        return false;
    }
    day = {};
    day.date = date;
    uint16_t today[6] = {};
    uint16_t tomorrow[6] = {};
    if (!calculate_six_times(settings, date, today) ||
        !calculate_six_times(settings, prayer_date_next(date), tomorrow)) {
        return false;
    }
    for (size_t index = 0U; index < 6U; ++index) {
        day.minutes[index] = today[index];
    }

    const uint16_t maghrib = today[
        static_cast<size_t>(PrayerTimeSlot::Maghrib)];
    const uint16_t next_fajr = tomorrow[
        static_cast<size_t>(PrayerTimeSlot::Fajr)];
    const uint16_t night_length = static_cast<uint16_t>(
        24U * 60U + next_fajr - maghrib);
    const int qiyam_adjustment = settings.prayer_adjustments[
        static_cast<size_t>(PrayerAdjustment::Qiyam)];
    const int qiyam = static_cast<int>(maghrib) +
                      static_cast<int>((2U * night_length + 1U) / 3U) +
                      qiyam_adjustment;
    if (qiyam < 0 || qiyam >= 2 * 24 * 60) {
        return false;
    }
    day.minutes[static_cast<size_t>(PrayerTimeSlot::Qiyam)] =
        static_cast<uint16_t>(qiyam);
    fill_calendar_labels(date, day);
    return prayer_day_valid(day);
}
