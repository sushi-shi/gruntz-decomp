#ifndef WAP32_COORDUNSET_H
#define WAP32_COORDUNSET_H

#include <Enums.h>
#include <Wap32/CoordUnset.h>

// INT_MIN as "this coordinate or rectangle has never been set".
//
// ONE sentinel, one name. It had grown four: COORD_UNSET (GameLevel.h),
// COORD_UNSET (AmbientSound.h) and TWO verbatim-duplicated file-local
// AXIS_UNSETs (GameLevel.cpp, GameLevelMove.cpp), plus raw sites in
// ResolveNode, LevelPlane and WorldSoundSet.
//
// Every one of them is the same thing on the same kind of field: the `left` of
// a RECT, or a screen coordinate, poked to INT_MIN so that a later read can
// tell "unset" from a legitimate zero or negative. LevelPlane's reset writes it
// into m_area, m_extent, m_clip and m_switchRect in four consecutive lines, and
// the guards that read those fields are all `!= COORD_UNSET`.
//
// NOT the same as GetAsyncKeyState's 0x80000000, which is a key-down bit rather
// than a coordinate - those stay as they are.
GZ_ENUM_CONST_BEGIN(CoordSentinel)
    COORD_UNSET = 0x80000000,
    // The OTHER "never written" marker, and a different one: 0xEE repeated is
    // the fill pattern, used where the field is a tile-grid cell or a menu
    // item's cached hit box rather than a screen coordinate. It had two names
    // too - UNINIT_FILL in GameLevel.h and nothing at all at the dozen sites
    // that spelled it raw - and every one of them tests it the same way, as
    // `x == UNINIT_FILL || x == -1`.
    UNINIT_FILL = 0xeeeeeeee
GZ_ENUM_CONST_END(CoordSentinel)

#endif // WAP32_COORDUNSET_H
