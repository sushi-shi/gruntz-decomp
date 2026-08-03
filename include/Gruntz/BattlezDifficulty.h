#ifndef GRUNTZ_GRUNTZ_BATTLEZDIFFICULTY_H
#define GRUNTZ_GRUNTZ_BATTLEZDIFFICULTY_H

#include <Enums.h>

// The Battlez difficulty setting.
//
// Named by RETAIL'S OWN bute keys, which each arm reads by name:
//
//   0  g_buteMgr.GetIntDef("Battlez", "EasyDifficulty",   100)   g_diffTier = 20
//   1  g_buteMgr.GetIntDef("Battlez", "NormalDifficulty",  50)   g_diffTier = 10
//   2  g_buteMgr.GetIntDef("Battlez", "HardDifficulty",    25)   g_diffTier =  5
//
// The two ladders corroborate the order independently: the bute default halves
// and then halves again, and g_diffTier steps 20/10/5 alongside it.
GZ_ENUM_BEGIN(BattlezDifficulty)
    BZDIFF_EASY = 0,
    BZDIFF_NORMAL = 1,
    BZDIFF_HARD = 2
GZ_ENUM_END(BattlezDifficulty)

#endif // GRUNTZ_GRUNTZ_BATTLEZDIFFICULTY_H
