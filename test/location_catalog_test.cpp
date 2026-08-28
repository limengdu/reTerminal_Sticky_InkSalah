#include <cassert>
#include <cstring>

#include "location_catalog.h"

int main()
{
    assert(location_country_count() >= 240U);

    const LocationCountry *indonesia = location_country_by_code("ID");
    assert(indonesia != nullptr);
    assert(std::strcmp(indonesia->api_name, "Indonesia") == 0);
    assert(indonesia->recommended_method == 20U);
    assert(location_country_has_city(*indonesia, "Jakarta"));

    const LocationCountry *malaysia = location_country_by_code("MY");
    assert(malaysia != nullptr);
    assert(malaysia->recommended_method == 17U);
    assert(location_country_has_city(*malaysia, "Kuala Lumpur"));

    const LocationCountry *united_states = location_country_by_code("US");
    assert(united_states != nullptr);
    assert(united_states->recommended_method == 2U);
    assert(location_country_has_city(*united_states, "New York"));

    assert(location_country_by_code("XX") == nullptr);
    return 0;
}
