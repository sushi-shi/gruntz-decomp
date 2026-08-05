#ifndef GRUNTZ_WWD_MOVEMODE_H
#define GRUNTZ_WWD_MOVEMODE_H

#include <Enums.h>

// How CGameLevel::DispatchMove propels an object this step, as carried by
// CWwdGameObjectFamily::m_moveMode.
//
// NOT the same field as CGrunt::m_moveMode (Grunt.h), which shares the name but
// holds a PickupType - see the comment there.
//
// Each name is the mover its own arm calls, and the two transitions the arms
// test pin the pair 1/4 independently: MoveRising sets 4 when it tops out, and
// MoveFalling sets 1 when it lands.
GZ_ENUM_BEGIN(MoveMode)
    MOVE_NONE = 0,
    MOVE_GROUNDED = 1,
    // These distinct serialized values share the grounded mover. No other
    // writer or discriminator survives, so the suffix records the evidence
    // without inventing a finer semantic distinction.
    MOVE_GROUNDED_2 = 2,
    MOVE_GROUNDED_LAST = MOVE_GROUNDED_2,
    MOVE_RISING = 3,
    MOVE_FALLING = 4,
    MOVE_GROUNDED_5 = 5,
    MOVE_CLIMBING = 6,
    // The arm writes m_screenX/m_screenY straight through with no collision step,
    // so the position is authoritative: what CProjectile and CGrunt's teleport set.
    MOVE_DIRECT = 7,
    // Picks rising or falling by comparing destY to the current Y, then re-arms
    // itself to 8 so the choice is made afresh next step.
    MOVE_AUTO_VERTICAL = 8
GZ_ENUM_END(MoveMode)

#endif // GRUNTZ_WWD_MOVEMODE_H
