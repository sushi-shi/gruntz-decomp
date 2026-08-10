#ifndef GRUNTZ_GRUNTZ_BATTLESTATROW_H
#define GRUNTZ_GRUNTZ_BATTLESTATROW_H

#include <Enums.h>

// The seven rows of the multiplayer BATTLE STATZ panel, as indexed into
// g_labelRects[7] and dispatched by CBootyState's label switch.
//
// Retail writes each row's own caption in its arm - "Fortz:", "Killz:",
// "Gruntz:", "Toolz:", "Toyz:", "Powerupz:", "Cursez:" - so the strings name the
// domain and nothing is inferred. The value matters beyond the caption because
// it is also the subscript into the rect table that positions the row.
//
// Distinct from BootyStatRow, which is the SINGLE-player level-complete panel:
// that one counts time, gruntz exited and gruntz lost, this one counts fortz and
// killz. Only Toolz/Toyz/Powerupz appear in both.
GZ_ENUM_BEGIN(BattleStatRow)
    BATTLEROW_FORTZ = 0,
    BATTLEROW_KILLZ = 1,
    BATTLEROW_GRUNTZ = 2,
    BATTLEROW_TOOLZ = 3,
    BATTLEROW_TOYZ = 4,
    BATTLEROW_POWERUPZ = 5,
    BATTLEROW_CURSEZ = 6,
    // Retail's build loop is `cmp c,7 / jl` (0x1ed30 +0x38a), i.e. `c < COUNT` -
    // not the inclusive `c <= CURSEZ` this header used to claim.
    BATTLEROW_COUNT = 7
GZ_ENUM_END(BattleStatRow)

#endif // GRUNTZ_GRUNTZ_BATTLESTATROW_H
