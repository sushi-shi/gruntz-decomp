#ifndef GRUNTZ_GRUNTZ_WARPLETTER_H
#define GRUNTZ_GRUNTZ_WARPLETTER_H

#include <Enums.h>

// One of the four letters of W-A-R-P, the secret-warp collectibles the Booty
// screen walks on.
//
// The value is the letter's position in the word, which both switches in
// CBootyWalkAnim spell out by doing nothing but turn the index into its letter:
// 0 -> "W", 1 -> "A", 2 -> "R", 3 -> "P". That string then names a resource,
// "GRUNTZ_PICKUPS_" + letter, so retail's own image-set names are what pin the
// order.
//
// The same index addresses CBootyWalkAnim::m_animSprites[4] and selects the
// scoreboard row via GetRecordValue(i), which is why the count belongs here
// rather than as a bare 4 at each of its three loop bounds.
GZ_ENUM_BEGIN(WarpLetter)
    WARPLETTER_W = 0,
    WARPLETTER_A = 1,
    WARPLETTER_R = 2,
    WARPLETTER_P = 3,
    WARPLETTER_COUNT = 4
GZ_ENUM_END(WarpLetter)

#endif // GRUNTZ_GRUNTZ_WARPLETTER_H
