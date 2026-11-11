#include <cassert>
#include "sticky_touch_recovery_policy.h"

int main()
{
    assert(!sticky_touch_startup_retry_required(true, true, 480, 800, 1));
    assert(sticky_touch_startup_retry_required(true, true, 2048, 2048, 1));
    assert(sticky_touch_startup_retry_required(true, true, 2048, 2048, 2));
    assert(!sticky_touch_startup_retry_required(true, true, 2048, 2048, 3));
    assert(sticky_touch_startup_retry_required(true, false, 0, 0, 1));
    assert(sticky_touch_startup_retry_required(false, false, 0, 0, 2));
    assert(!sticky_touch_startup_retry_required(false, false, 0, 0, 3));
    const auto fallback = sticky_touch_select_sensor_resolution(true, 2048, 2048);
    assert(fallback.used_fallback && fallback.width == 480 && fallback.height == 800);
    assert(sticky_touch_recovery_required(kStickyTouchReadFailureThreshold));
    assert(!sticky_touch_recovery_required(kStickyTouchReadFailureThreshold - 1));
    assert(sticky_touch_contact_stuck(100, 100 + kStickyTouchMaximumContactMs));
    assert(!sticky_touch_contact_stuck(100, 100 + kStickyTouchMaximumContactMs - 1));
}
