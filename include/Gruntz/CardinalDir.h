#ifndef GRUNTZ_GRUNTZ_CARDINALDIR_H
#define GRUNTZ_GRUNTZ_CARDINALDIR_H

#include <Enums.h>

// The four-way heading a level hazard travels, stored in the WWD object's
// Direction / Smarts field.
//
// This is NOT GruntDirection. That domain is the eight-way compass ring a Grunt
// faces, where EAST is 3 and WEST is 7; this one is four-way and consecutive, so
// EAST is 2 and WEST is 4. The two agree only on NORTH, which is exactly the
// trap: a value of 2 here is EAST, never NORTHEAST.
//
// Named by RETAIL'S OWN frame-set names. Three unrelated hazards each map the
// same four strings onto the same four values, once when reading the frame set
// back and once when applying it:
//
//   LEVEL_KITCHENSLIME_{NORTH,EAST,SOUTH,WEST}   CKitchenSlime
//   LEVEL_ROLLINGBALL_{NORTH,EAST,SOUTH,WEST}    CRollingBall
//   LEVEL_OBJECTDROPPER_{NORTH,EAST,SOUTH,WEST}  CDroppedObject
//
// The step vectors corroborate independently: NORTH is (0, -1), EAST is (1, 0),
// SOUTH is (0, +1), WEST is (-1, 0), in screen coordinates where +y is down. So
// does CRollingBall's tile-arrow switch, which turns a ball onto the heading its
// TILEKIND_ARROW_{UP,RIGHT,DOWN,LEFT} tile points at.
GZ_ENUM_BEGIN(CardinalDir)
    CARDINAL_NORTH = 1,
    CARDINAL_EAST = 2,
    CARDINAL_SOUTH = 3,
    CARDINAL_WEST = 4,
    // Clockwise from north, so the ring is these four in order. CKitchenSlime's
    // search walks it with `i <= 4`, trying each heading in turn.
    CARDINAL_FIRST = CARDINAL_NORTH,
    CARDINAL_LAST = CARDINAL_WEST,
    CARDINAL_COUNT = 4
GZ_ENUM_END(CardinalDir)

#endif // GRUNTZ_GRUNTZ_CARDINALDIR_H
