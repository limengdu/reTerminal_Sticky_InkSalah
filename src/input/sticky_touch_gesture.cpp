#include "sticky_touch_gesture.h"

#include <algorithm>
#include <cstdlib>

StickyVerticalSwipe sticky_touch_classify_vertical_swipe(
    const StickyLogicalTouchPath &path,
    int page_width,
    int page_height)
{
    if (page_width <= 0 || page_height <= 0 ||
        path.duration_ms > 1800U) {
        return StickyVerticalSwipe::None;
    }

    const int edge_depth = std::max(72, page_height / 4);
    const int minimum_travel = std::max(90, page_height / 5);
    const int delta_x = path.end_x - path.start_x;
    const int delta_y = path.end_y - path.start_y;
    const int horizontal_travel = std::abs(delta_x);
    const int vertical_travel = std::abs(delta_y);
    if (vertical_travel < minimum_travel ||
        vertical_travel < horizontal_travel * 2) {
        return StickyVerticalSwipe::None;
    }

    if (path.start_y >= page_height - edge_depth &&
        delta_y < 0) {
        return StickyVerticalSwipe::UpFromBottom;
    }
    if (path.start_y <= edge_depth && delta_y > 0) {
        return StickyVerticalSwipe::DownFromTop;
    }
    return StickyVerticalSwipe::None;
}
