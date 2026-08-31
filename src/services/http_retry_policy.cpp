#include "http_retry_policy.h"

bool http_retry_should_continue(HttpRequestOutcome outcome,
                                uint8_t completed_attempts)
{
    return outcome == HttpRequestOutcome::TransientFailure &&
           completed_attempts < kHttpRequestAttemptLimit;
}

uint32_t http_retry_delay_ms(uint8_t completed_attempts)
{
    return completed_attempts <= 1U ? 500U : 1000U;
}
