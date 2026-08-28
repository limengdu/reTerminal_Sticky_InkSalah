#pragma once

#include <cstddef>
#include <cstdint>

struct LocationCountry {
    const char *code = nullptr;
    const char *api_name = nullptr;
    uint8_t recommended_method = 3U;
    const char *cities = nullptr;
};

size_t location_country_count();
const LocationCountry *location_country_at(size_t index);
const LocationCountry *location_country_by_code(const char *code);
bool location_country_has_city(const LocationCountry &country,
                               const char *city);
