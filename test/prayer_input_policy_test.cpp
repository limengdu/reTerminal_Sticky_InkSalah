#include <cassert>

#include "prayer_input_policy.h"

int main()
{
    assert(prayer_input_elapsed_ms(14258U, 15258U) == 0U);
    assert(prayer_input_elapsed_ms(15258U, 14258U) == 1000U);
    assert(prayer_input_elapsed_ms(25U, UINT32_MAX - 24U) == 50U);

    assert(prayer_input_wait_decision(0U, 0U, 60000U) ==
           PrayerInputWaitDecision::Continue);
    assert(prayer_input_wait_decision(60000U, 90000U, 60000U) ==
           PrayerInputWaitDecision::Refresh);
    assert(prayer_input_wait_decision(1000U, 179999U, 60000U) ==
           PrayerInputWaitDecision::Continue);
    assert(prayer_input_wait_decision(1000U, 180000U, 60000U) ==
           PrayerInputWaitDecision::Sleep);
    assert(prayer_input_wait_decision(180000U, 180000U, 60000U) ==
           PrayerInputWaitDecision::Sleep);
    return 0;
}
