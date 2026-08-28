#include "captive_dns_protocol.h"

#include <cstring>

namespace {

constexpr size_t kDnsHeaderLength = 12U;
constexpr size_t kDnsAnswerLength = 16U;

size_t question_length(const uint8_t *query, size_t length)
{
    if (query == nullptr || length < kDnsHeaderLength + 5U ||
        query[4] != 0U || query[5] != 1U) {
        return 0U;
    }
    size_t cursor = kDnsHeaderLength;
    while (cursor < length && query[cursor] != 0U) {
        const size_t label_length = query[cursor];
        if (label_length > 63U || cursor + label_length + 1U >= length) {
            return 0U;
        }
        cursor += label_length + 1U;
    }
    const bool valid = cursor + 5U <= length && query[cursor + 1U] == 0U &&
                       query[cursor + 2U] == 1U &&
                       query[cursor + 3U] == 0U &&
                       query[cursor + 4U] == 1U;
    return valid ? cursor + 5U : 0U;
}

}  // namespace

size_t captive_dns_build_response(const uint8_t *query,
                                  size_t query_length,
                                  const uint8_t portal_ip[4],
                                  uint8_t *response,
                                  size_t response_capacity)
{
    const size_t request_length = question_length(query, query_length);
    if (request_length == 0U || portal_ip == nullptr || response == nullptr ||
        request_length + kDnsAnswerLength > response_capacity) {
        return 0U;
    }
    std::memcpy(response, query, request_length);
    response[2] = 0x81U;
    response[3] = 0x80U;
    response[6] = 0x00U;
    response[7] = 0x01U;
    response[8] = 0x00U;
    response[9] = 0x00U;
    response[10] = 0x00U;
    response[11] = 0x00U;

    uint8_t *answer = response + request_length;
    answer[0] = 0xC0U;
    answer[1] = 0x0CU;
    answer[2] = 0x00U;
    answer[3] = 0x01U;
    answer[4] = 0x00U;
    answer[5] = 0x01U;
    answer[6] = 0x00U;
    answer[7] = 0x00U;
    answer[8] = 0x00U;
    answer[9] = 0x3CU;
    answer[10] = 0x00U;
    answer[11] = 0x04U;
    std::memcpy(answer + 12U, portal_ip, 4U);
    return request_length + kDnsAnswerLength;
}
