#ifndef GRUNTZ_GRUNTZ_SPELLID_H
#define GRUNTZ_GRUNTZ_SPELLID_H

#include <Enums.h>

// Which spell a Scroll or Magic Wand casts.
//
// The editor stores it in the object's Face Dir field, NOT in the object id -
// so a CGameObject whose m_smarts is PICKUP_SCROLL or PICKUP_WAND carries the
// spell in m_faceDirection (docs/domain/powerupz.md, "Spellz (0-6)", and
// docs/domain/toyz.md on Toy 30).
//
// The code agrees with the doc twice over. CInGameIcon::HandleInput guards on
// exactly that pickup pair and switches m_faceDirection over exactly 1..6, and
// the tint each arm picks reads as the spell it names:
//
//   1  SPELL_FREEZE         TINT_WHITE    ice
//   2  SPELL_HEALTH         TINT_GREEN    health
//   3  SPELL_RESURRECTION   TINT_ORANGE
//   4  SPELL_RANDOM_TOYZ    TINT_PINK
//   5  SPELL_TELEPORT       TINT_BLUE
//   6  SPELL_ROLLING_BALLZ  TINT_RED
//
// 0 is not a spell but the instruction to roll one of the other six, which is
// why it falls to the default arm and gets TINT_BLACK - the icon for "unknown
// until cast".
GZ_ENUM_BEGIN(SpellId)
    SPELL_RANDOM = 0,
    SPELL_FREEZE = 1,
    SPELL_HEALTH = 2,
    SPELL_RESURRECTION = 3,
    SPELL_RANDOM_TOYZ = 4,
    SPELL_TELEPORT = 5,
    SPELL_ROLLING_BALLZ = 6,
    // The inclusive ends of the range SPELL_RANDOM rolls within.
    SPELL_CASTABLE_FIRST = SPELL_FREEZE,
    SPELL_CASTABLE_LAST = SPELL_ROLLING_BALLZ
GZ_ENUM_END(SpellId)

#endif // GRUNTZ_GRUNTZ_SPELLID_H
