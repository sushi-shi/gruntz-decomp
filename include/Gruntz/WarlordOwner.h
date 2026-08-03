#ifndef GRUNTZ_GRUNTZ_WARLORDOWNER_H
#define GRUNTZ_GRUNTZ_WARLORDOWNER_H

#include <Enums.h>

// The four enemy Warlords. Names are retail's own sprite-namespace strings
// (`WARLORDZ_KING`, `..._NAPOLEAN`, `..._PATTON`, `..._VIKING` -
// docs/strings-analysis.md; `src/Gruntz/Warlord.cpp` holds the literals).
// This folds the two copies that used to exist: `enum Warlord` (Enums.h) and
// the TU-private `enum WarlordOwner` (Warlord.cpp).
GZ_ENUM_BEGIN(WarlordOwner)
    WARLORDZ_KING = 0,
    WARLORDZ_NAPOLEAN = 1,
    WARLORDZ_PATTON = 2,
    WARLORDZ_VIKING = 3,
    WARLORDZ_COUNT = 4
GZ_ENUM_END(WarlordOwner)

#endif // GRUNTZ_GRUNTZ_WARLORDOWNER_H
