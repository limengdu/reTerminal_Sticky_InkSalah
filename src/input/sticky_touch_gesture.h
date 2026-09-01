#pragma once

#include <cstdint>

enum class StickyVerticalSwipe : uint8_t {
    None,
    UpFromBottom,
    DownFromTop,
};

struct StickyLogicalTouchPath {
    int start_x;
    int start_y;
    int end_x;
    int end_y;
    uint32_t duration_ms;
};

// Recognizes a deliberate vertical edge swipe in the current page axes.
// 在当前页面坐标系中识别明确的纵向边缘滑动。
StickyVerticalSwipe sticky_touch_classify_vertical_swipe(
    const StickyLogicalTouchPath &path,
    int page_width,
    int page_height);
