#include "location_catalog.h"

#include <cstring>

namespace {

#define C(code, name) {code, name, 3U, 0U, ""}
#define M(code, name, method, cities) {code, name, method, 0U, cities}
#define H(code, name, method, cities) {code, name, method, 1U, cities}

// Country names follow ISO 3166. City suggestions are a compact offline list
// used only to make location entry faster.
// 国家名称采用ISO 3166，城市建议是用于加快地点输入的离线精简列表。
constexpr LocationCountry kCountries[] = {
    C("AD", "Andorra"),
    M("AE", "United Arab Emirates", 16U, "Abu Dhabi|Ajman|Dubai|Ras Al Khaimah|Sharjah|Umm Al Quwain"),
    M("AF", "Afghanistan", 1U, "Kabul"),
    C("AG", "Antigua and Barbuda"),
    C("AI", "Anguilla"),
    M("AL", "Albania", 3U, "Tirana"),
    M("AM", "Armenia", 3U, "Yerevan"),
    M("AO", "Angola", 3U, "Luanda"),
    C("AQ", "Antarctica"),
    M("AR", "Argentina", 3U, "Buenos Aires"),
    C("AS", "American Samoa"),
    M("AT", "Austria", 3U, "Vienna"),
    M("AU", "Australia", 3U, "Adelaide|Brisbane|Canberra|Darwin|Perth|Sydney"),
    C("AW", "Aruba"),
    C("AX", "Aland Islands"),
    M("AZ", "Azerbaijan", 3U, "Baku"),
    M("BA", "Bosnia and Herzegovina", 3U, "Sarajevo"),
    M("BB", "Barbados", 3U, "Bridgetown"),
    M("BD", "Bangladesh", 1U, "Dhaka"),
    M("BE", "Belgium", 3U, "Brussels"),
    M("BF", "Burkina Faso", 3U, "Ouagadougou"),
    M("BG", "Bulgaria", 3U, "Sofia"),
    M("BH", "Bahrain", 8U, "Manama"),
    M("BI", "Burundi", 3U, "Bujumbura|Gitega"),
    M("BJ", "Benin", 3U, "Cotonou|Porto-Novo"),
    C("BL", "Saint Barthelemy"),
    C("BM", "Bermuda"),
    M("BN", "Brunei", 3U, "Bandar Seri Begawan"),
    M("BO", "Bolivia", 3U, "La Paz|Sucre"),
    C("BQ", "Caribbean Netherlands"),
    M("BR", "Brazil", 3U, "Brasilia|Sao Paulo"),
    M("BS", "Bahamas", 3U, "Nassau"),
    M("BT", "Bhutan", 3U, "Thimphu"),
    C("BV", "Bouvet Island"),
    M("BW", "Botswana", 3U, "Gaborone"),
    M("BY", "Belarus", 14U, "Minsk"),
    C("BZ", "Belize"),
    M("CA", "Canada", 2U, "Edmonton|Halifax|Montreal|Ottawa|Toronto|Vancouver"),
    C("CC", "Cocos Islands"),
    M("CD", "Democratic Republic of the Congo", 3U, "Kinshasa"),
    M("CF", "Central African Republic", 3U, "Bangui"),
    M("CG", "Republic of the Congo", 3U, "Brazzaville"),
    M("CH", "Switzerland", 3U, "Bern|Geneva|Zurich"),
    M("CI", "Cote d'Ivoire", 3U, "Abidjan|Yamoussoukro"),
    C("CK", "Cook Islands"),
    M("CL", "Chile", 3U, "Santiago"),
    M("CM", "Cameroon", 3U, "Yaounde"),
    H("CN", "China", 3U, "Beijing|Guangzhou|Shanghai|Shenzhen"),
    M("CO", "Colombia", 3U, "Bogota"),
    M("CR", "Costa Rica", 3U, "San Jose"),
    M("CU", "Cuba", 3U, "Havana"),
    M("CV", "Cape Verde", 3U, "Praia"),
    M("CW", "Curacao", 3U, "Willemstad"),
    C("CX", "Christmas Island"),
    M("CY", "Cyprus", 3U, "Nicosia"),
    M("CZ", "Czech Republic", 3U, "Prague"),
    M("DE", "Germany", 3U, "Berlin|Frankfurt|Hamburg|Munich"),
    M("DJ", "Djibouti", 3U, "Djibouti"),
    M("DK", "Denmark", 3U, "Copenhagen"),
    C("DM", "Dominica"),
    M("DO", "Dominican Republic", 3U, "Santo Domingo"),
    M("DZ", "Algeria", 19U, "Algiers|Oran"),
    M("EC", "Ecuador", 3U, "Quito"),
    M("EE", "Estonia", 3U, "Tallinn"),
    M("EG", "Egypt", 5U, "Alexandria|Cairo"),
    C("EH", "Western Sahara"),
    M("ER", "Eritrea", 3U, "Asmara"),
    M("ES", "Spain", 3U, "Barcelona|Cordoba|Madrid|Valencia"),
    M("ET", "Ethiopia", 3U, "Addis Ababa"),
    M("FI", "Finland", 3U, "Helsinki"),
    M("FJ", "Fiji", 3U, "Suva"),
    C("FK", "Falkland Islands"),
    C("FM", "Micronesia"),
    C("FO", "Faroe Islands"),
    M("FR", "France", 12U, "Lyon|Marseille|Paris"),
    M("GA", "Gabon", 3U, "Libreville"),
    M("GB", "United Kingdom", 3U, "Belfast|Birmingham|Edinburgh|Glasgow|Leeds|London|Manchester"),
    C("GD", "Grenada"),
    M("GE", "Georgia", 3U, "Tbilisi"),
    C("GF", "French Guiana"),
    C("GG", "Guernsey"),
    M("GH", "Ghana", 3U, "Accra"),
    C("GI", "Gibraltar"),
    C("GL", "Greenland"),
    M("GM", "Gambia", 3U, "Banjul"),
    M("GN", "Guinea", 3U, "Conakry"),
    C("GP", "Guadeloupe"),
    M("GQ", "Equatorial Guinea", 3U, "Malabo"),
    M("GR", "Greece", 3U, "Athens"),
    C("GS", "South Georgia and the South Sandwich Islands"),
    M("GT", "Guatemala", 3U, "Guatemala City"),
    C("GU", "Guam"),
    M("GW", "Guinea-Bissau", 3U, "Bissau"),
    M("GY", "Guyana", 3U, "Georgetown"),
    M("HK", "Hong Kong", 3U, "Hong Kong"),
    C("HM", "Heard Island and McDonald Islands"),
    M("HN", "Honduras", 3U, "Tegucigalpa"),
    M("HR", "Croatia", 3U, "Zagreb"),
    M("HT", "Haiti", 3U, "Port-au-Prince"),
    M("HU", "Hungary", 3U, "Budapest"),
    M("ID", "Indonesia", 20U, "Bandung|Jakarta|Medan|Surabaya|Yogyakarta"),
    M("IE", "Ireland", 3U, "Dublin"),
    M("IL", "Israel", 3U, "Jerusalem|Tel Aviv"),
    C("IM", "Isle of Man"),
    M("IN", "India", 1U, "Ahmedabad|Bangalore|Chennai|Delhi|Hyderabad|Kolkata|Mumbai|New Delhi"),
    C("IO", "British Indian Ocean Territory"),
    M("IQ", "Iraq", 1U, "Baghdad|Mosul"),
    M("IR", "Iran", 7U, "Isfahan|Tehran"),
    M("IS", "Iceland", 3U, "Reykjavik"),
    M("IT", "Italy", 3U, "Milan|Naples|Rome"),
    C("JE", "Jersey"),
    M("JM", "Jamaica", 3U, "Kingston"),
    M("JO", "Jordan", 23U, "Amman"),
    M("JP", "Japan", 3U, "Osaka|Tokyo"),
    M("KE", "Kenya", 3U, "Mombasa|Nairobi"),
    M("KG", "Kyrgyzstan", 14U, "Bishkek"),
    M("KH", "Cambodia", 3U, "Phnom Penh"),
    C("KI", "Kiribati"),
    M("KM", "Comoros", 3U, "Moroni"),
    C("KN", "Saint Kitts and Nevis"),
    M("KP", "North Korea", 3U, "Pyongyang"),
    M("KR", "South Korea", 3U, "Busan|Seoul"),
    M("KW", "Kuwait", 9U, "Kuwait City"),
    C("KY", "Cayman Islands"),
    M("KZ", "Kazakhstan", 14U, "Almaty|Astana|Turkestan"),
    M("LA", "Laos", 3U, "Vientiane"),
    M("LB", "Lebanon", 3U, "Beirut"),
    C("LC", "Saint Lucia"),
    C("LI", "Liechtenstein"),
    M("LK", "Sri Lanka", 3U, "Colombo|Sri Jayewardenepura Kotte"),
    M("LR", "Liberia", 3U, "Monrovia"),
    M("LS", "Lesotho", 3U, "Maseru"),
    M("LT", "Lithuania", 3U, "Vilnius"),
    M("LU", "Luxembourg", 3U, "Luxembourg"),
    M("LV", "Latvia", 3U, "Riga"),
    M("LY", "Libya", 3U, "Tripoli"),
    M("MA", "Morocco", 21U, "Casablanca|Fez|Marrakech|Rabat"),
    C("MC", "Monaco"),
    M("MD", "Moldova", 3U, "Chisinau"),
    M("ME", "Montenegro", 3U, "Podgorica"),
    C("MF", "Saint Martin"),
    M("MG", "Madagascar", 3U, "Antananarivo"),
    C("MH", "Marshall Islands"),
    M("MK", "North Macedonia", 3U, "Skopje"),
    M("ML", "Mali", 3U, "Bamako"),
    M("MM", "Myanmar", 3U, "Nay Pyi Taw|Yangon"),
    M("MN", "Mongolia", 3U, "Ulaanbaatar"),
    M("MO", "Macau", 3U, "Macau"),
    C("MP", "Northern Mariana Islands"),
    C("MQ", "Martinique"),
    M("MR", "Mauritania", 3U, "Nouakchott"),
    C("MS", "Montserrat"),
    M("MT", "Malta", 3U, "Valletta"),
    M("MU", "Mauritius", 3U, "Port Louis"),
    M("MV", "Maldives", 3U, "Male"),
    M("MW", "Malawi", 3U, "Lilongwe"),
    M("MX", "Mexico", 3U, "Mexico City"),
    M("MY", "Malaysia", 17U, "Johor Bahru|Kuala Lumpur|Kuching|Penang"),
    M("MZ", "Mozambique", 3U, "Maputo"),
    M("NA", "Namibia", 3U, "Windhoek"),
    M("NC", "New Caledonia", 3U, "Noumea"),
    M("NE", "Niger", 3U, "Niamey"),
    C("NF", "Norfolk Island"),
    M("NG", "Nigeria", 3U, "Abuja|Kano|Lagos"),
    M("NI", "Nicaragua", 3U, "Managua"),
    M("NL", "Netherlands", 3U, "Amsterdam|Rotterdam|The Hague"),
    M("NO", "Norway", 3U, "Oslo"),
    M("NP", "Nepal", 3U, "Kathmandu"),
    C("NR", "Nauru"),
    C("NU", "Niue"),
    M("NZ", "New Zealand", 3U, "Auckland|Wellington"),
    M("OM", "Oman", 8U, "Muscat"),
    M("PA", "Panama", 3U, "Panama City"),
    M("PE", "Peru", 3U, "Lima"),
    M("PF", "French Polynesia", 3U, "Papeete"),
    M("PG", "Papua New Guinea", 3U, "Port Moresby"),
    M("PH", "Philippines", 3U, "Manila"),
    M("PK", "Pakistan", 1U, "Islamabad|Karachi|Lahore"),
    M("PL", "Poland", 3U, "Krakow|Warsaw"),
    C("PM", "Saint Pierre and Miquelon"),
    C("PN", "Pitcairn"),
    M("PR", "Puerto Rico", 2U, "San Juan"),
    M("PS", "Palestine", 3U, "Gaza|Jerusalem|Ramallah"),
    M("PT", "Portugal", 22U, "Lisbon|Porto"),
    C("PW", "Palau"),
    M("PY", "Paraguay", 3U, "Asuncion"),
    M("QA", "Qatar", 10U, "Doha"),
    M("RE", "Reunion", 3U, "Saint-Denis"),
    M("RO", "Romania", 3U, "Bucharest"),
    M("RS", "Serbia", 3U, "Belgrade"),
    M("RU", "Russia", 14U, "Kazan|Makhachkala|Moscow|Saint Petersburg"),
    M("RW", "Rwanda", 3U, "Kigali"),
    M("SA", "Saudi Arabia", 4U, "Jeddah|Madinah|Makkah|Riyadh"),
    C("SB", "Solomon Islands"),
    C("SC", "Seychelles"),
    M("SD", "Sudan", 3U, "Khartoum"),
    M("SE", "Sweden", 3U, "Stockholm"),
    M("SG", "Singapore", 11U, "Singapore"),
    C("SH", "Saint Helena"),
    M("SI", "Slovenia", 3U, "Ljubljana"),
    C("SJ", "Svalbard and Jan Mayen"),
    M("SK", "Slovakia", 3U, "Bratislava"),
    M("SL", "Sierra Leone", 3U, "Freetown"),
    C("SM", "San Marino"),
    M("SN", "Senegal", 3U, "Dakar"),
    M("SO", "Somalia", 3U, "Mogadishu"),
    M("SR", "Suriname", 3U, "Paramaribo"),
    M("SS", "South Sudan", 3U, "Juba"),
    M("ST", "Sao Tome and Principe", 3U, "Sao Tome"),
    M("SV", "El Salvador", 3U, "San Salvador"),
    C("SX", "Sint Maarten"),
    M("SY", "Syria", 3U, "Aleppo|Damascus"),
    M("SZ", "Eswatini", 3U, "Mbabane"),
    C("TC", "Turks and Caicos Islands"),
    M("TD", "Chad", 3U, "N'Djamena"),
    C("TF", "French Southern Territories"),
    M("TG", "Togo", 3U, "Lome"),
    M("TH", "Thailand", 3U, "Bangkok"),
    M("TJ", "Tajikistan", 14U, "Dushanbe"),
    C("TK", "Tokelau"),
    M("TL", "Timor-Leste", 3U, "Dili"),
    M("TM", "Turkmenistan", 14U, "Ashgabat"),
    M("TN", "Tunisia", 18U, "Tunis"),
    C("TO", "Tonga"),
    M("TR", "Turkey", 13U, "Ankara|Istanbul|Kayseri|Konya"),
    C("TT", "Trinidad and Tobago"),
    C("TV", "Tuvalu"),
    M("TW", "Taiwan", 3U, "Taipei"),
    M("TZ", "Tanzania", 3U, "Dar es Salaam|Dodoma"),
    M("UA", "Ukraine", 3U, "Kyiv"),
    M("UG", "Uganda", 3U, "Kampala"),
    C("UM", "United States Minor Outlying Islands"),
    M("US", "United States", 2U, "Boston|Chicago|Dallas|Denver|Houston|Los Angeles|Miami|New York|Philadelphia|San Francisco|Seattle|Washington"),
    M("UY", "Uruguay", 3U, "Montevideo"),
    M("UZ", "Uzbekistan", 14U, "Samarkand|Tashkent"),
    C("VA", "Vatican City"),
    C("VC", "Saint Vincent and the Grenadines"),
    M("VE", "Venezuela", 3U, "Caracas"),
    C("VG", "British Virgin Islands"),
    C("VI", "United States Virgin Islands"),
    M("VN", "Vietnam", 3U, "Hanoi|Ho Chi Minh City"),
    C("VU", "Vanuatu"),
    C("WF", "Wallis and Futuna"),
    C("WS", "Samoa"),
    M("YE", "Yemen", 3U, "Aden|Sanaa"),
    C("YT", "Mayotte"),
    M("ZA", "South Africa", 3U, "Cape Town|Durban|Johannesburg|Pretoria"),
    M("ZM", "Zambia", 3U, "Lusaka"),
    M("ZW", "Zimbabwe", 3U, "Harare"),
};

#undef C
#undef M
#undef H

}  // namespace

size_t location_country_count()
{
    return sizeof(kCountries) / sizeof(kCountries[0]);
}

const LocationCountry *location_country_at(size_t index)
{
    return index < location_country_count() ? &kCountries[index] : nullptr;
}

const LocationCountry *location_country_by_code(const char *code)
{
    if (code == nullptr) {
        return nullptr;
    }
    for (const LocationCountry &country : kCountries) {
        if (std::strcmp(country.code, code) == 0) {
            return &country;
        }
    }
    return nullptr;
}

const LocationCountry *location_country_by_name(const char *name)
{
    if (name == nullptr) {
        return nullptr;
    }
    for (const LocationCountry &country : kCountries) {
        if (std::strcmp(country.api_name, name) == 0) {
            return &country;
        }
    }
    return nullptr;
}

bool location_country_has_city(const LocationCountry &country,
                               const char *city)
{
    if (country.cities == nullptr || city == nullptr || city[0] == '\0') {
        return false;
    }
    const size_t city_length = std::strlen(city);
    const char *cursor = country.cities;
    while (*cursor != '\0') {
        const char *separator = std::strchr(cursor, '|');
        const size_t length = separator == nullptr
                                  ? std::strlen(cursor)
                                  : static_cast<size_t>(separator - cursor);
        if (length == city_length && std::strncmp(cursor, city, length) == 0) {
            return true;
        }
        if (separator == nullptr) {
            break;
        }
        cursor = separator + 1;
    }
    return false;
}
