#pragma once

#include <cstdint>

enum class HttpRequestOutcome : uint8_t {
    Success,
    TransientFailure,
    PermanentFailure,
};

constexpr uint8_t kHttpRequestAttemptLimit = 3U;

// Return whether another HTTP attempt is allowed for this outcome.
// 返回当前请求结果是否允许继续尝试。
bool http_retry_should_continue(HttpRequestOutcome outcome,
                                uint8_t completed_attempts);

// Return the delay before the next HTTP attempt.
// 返回下一次 HTTP 尝试前的等待时间。
uint32_t http_retry_delay_ms(uint8_t completed_attempts);
