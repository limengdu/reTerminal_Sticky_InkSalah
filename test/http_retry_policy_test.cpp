#include <cassert>

#include "http_retry_policy.h"

int main()
{
    assert(kHttpRequestAttemptLimit == 3U);
    assert(http_retry_should_continue(
        HttpRequestOutcome::TransientFailure, 1U));
    assert(http_retry_should_continue(
        HttpRequestOutcome::TransientFailure, 2U));
    assert(!http_retry_should_continue(
        HttpRequestOutcome::TransientFailure, 3U));
    assert(!http_retry_should_continue(HttpRequestOutcome::Success, 1U));
    assert(!http_retry_should_continue(
        HttpRequestOutcome::PermanentFailure, 1U));
    assert(http_retry_delay_ms(1U) == 500U);
    assert(http_retry_delay_ms(2U) == 1000U);
    return 0;
}
