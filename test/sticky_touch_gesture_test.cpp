#include <cassert>

#include "sticky_touch_gesture.h"

int main()
{
    assert(sticky_touch_classify_vertical_swipe(
               {240, 760, 238, 430, 620}, 480, 800) ==
           StickyVerticalSwipe::UpFromBottom);
    assert(sticky_touch_classify_vertical_swipe(
               {680, 455, 676, 260, 540}, 800, 480) ==
           StickyVerticalSwipe::UpFromBottom);
    assert(sticky_touch_classify_vertical_swipe(
               {220, 42, 224, 310, 510}, 480, 800) ==
           StickyVerticalSwipe::DownFromTop);
    assert(sticky_touch_classify_vertical_swipe(
               {420, 38, 418, 230, 480}, 800, 480) ==
           StickyVerticalSwipe::DownFromTop);

    assert(sticky_touch_classify_vertical_swipe(
               {230, 520, 228, 250, 500}, 480, 800) ==
           StickyVerticalSwipe::None);
    assert(sticky_touch_classify_vertical_swipe(
               {240, 760, 420, 690, 500}, 480, 800) ==
           StickyVerticalSwipe::None);
    assert(sticky_touch_classify_vertical_swipe(
               {240, 760, 240, 430, 2200}, 480, 800) ==
           StickyVerticalSwipe::None);

    assert(sticky_touch_classify_horizontal_swipe(
               {28, 420, 230, 418, 520}, 480, 800) ==
           StickyHorizontalSwipe::RightFromLeft);
    assert(sticky_touch_classify_horizontal_swipe(
               {42, 240, 370, 238, 610}, 800, 480) ==
           StickyHorizontalSwipe::RightFromLeft);
    assert(sticky_touch_classify_horizontal_swipe(
               {180, 420, 350, 418, 520}, 480, 800) ==
           StickyHorizontalSwipe::None);
    assert(sticky_touch_classify_horizontal_swipe(
               {28, 420, 150, 418, 520}, 480, 800) ==
           StickyHorizontalSwipe::None);
    assert(sticky_touch_classify_horizontal_swipe(
               {28, 420, 250, 650, 520}, 480, 800) ==
           StickyHorizontalSwipe::None);
    assert(sticky_touch_classify_horizontal_swipe(
               {28, 420, 230, 418, 2200}, 480, 800) ==
           StickyHorizontalSwipe::None);
    return 0;
}
