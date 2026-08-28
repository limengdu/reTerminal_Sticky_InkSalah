#include <cassert>

#include "display_orientation.h"

int main()
{
    const CanvasRotation portrait = salah_default_rotation();
    assert(portrait == CanvasRotation::Deg90CounterClockwise);
    assert(salah_rotation_from_orientation(
               StickyImuOrientation::Landscape0, portrait) ==
           CanvasRotation::Deg90CounterClockwise);
    assert(salah_rotation_from_orientation(
               StickyImuOrientation::Landscape180, portrait) ==
           CanvasRotation::Deg90Clockwise);
    assert(salah_rotation_from_orientation(
               StickyImuOrientation::Portrait0, portrait) ==
           CanvasRotation::Deg180);
    assert(salah_rotation_from_orientation(
               StickyImuOrientation::Portrait180, portrait) ==
           CanvasRotation::Deg0);
    assert(salah_rotation_from_orientation(
               StickyImuOrientation::Unknown, portrait) == portrait);
    assert(salah_rotation_from_orientation(
               StickyImuOrientation::FaceUp, portrait) == portrait);
    return 0;
}
