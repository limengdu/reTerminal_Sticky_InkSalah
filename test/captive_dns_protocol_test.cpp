#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>

#include "captive_dns_protocol.h"

int main()
{
    constexpr std::array<uint8_t, 33> query = {
        0x12, 0x34, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x07, 'c',  'o',  'n',
        'n',  'e',  'c',  't',  0x04, 't',  'e',  's',
        't',  0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00,
        0x00,
    };
    std::array<uint8_t, 64> response = {};
    const std::array<uint8_t, 4> portal_ip = {192, 168, 4, 1};
    const size_t length = captive_dns_build_response(
        query.data(), 30U, portal_ip.data(), response.data(), response.size());

    assert(length == 46U);
    assert(response[0] == 0x12 && response[1] == 0x34);
    assert(response[2] == 0x81 && response[3] == 0x80);
    assert(response[6] == 0x00 && response[7] == 0x01);
    assert(response[length - 4U] == 192U);
    assert(response[length - 3U] == 168U);
    assert(response[length - 2U] == 4U);
    assert(response[length - 1U] == 1U);

    const size_t padded_length = captive_dns_build_response(
        query.data(), query.size(), portal_ip.data(), response.data(), response.size());
    assert(padded_length == 46U);

    assert(captive_dns_build_response(
               query.data(), 8U, portal_ip.data(), response.data(), response.size()) == 0U);
    return 0;
}
