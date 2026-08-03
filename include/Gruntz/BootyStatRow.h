#ifndef GRUNTZ_GRUNTZ_BOOTYSTATROW_H
#define GRUNTZ_GRUNTZ_BOOTYSTATROW_H

#include <Enums.h>

// Which row of the level-complete stat panel to format, as passed to
// CBootyState::FormatHudText and iterated by its callers.
//
// Retail names every one of them itself - each arm reads the group accessor of
// the same name:
//
//   0 SumElapsedTimeForGroup      4 SumToyzAvailable/CollectedForGroup
//   1 SumGruntzExitedForGroup     5 SumPowerupzAvailable/CollectedForGroup
//   2 SumGruntzLostForGroup       6 SumCoinsAvailable/CollectedForGroup
//   3 SumToolzAvailable/CollectedForGroup
//                                 7 SumSecretsAvailable/FoundForGroup
//
// Rows 3..7 all format "%d of %d" from an available/collected pair; 0 formats
// mm:ss and 1..2 a bare count, which is why they have no _AVAILABLE half.
GZ_ENUM_BEGIN(BootyStatRow)
    BOOTYSTAT_TIME = 0,
    BOOTYSTAT_GRUNTZ_EXITED = 1,
    BOOTYSTAT_GRUNTZ_LOST = 2,
    BOOTYSTAT_TOOLZ = 3,
    BOOTYSTAT_TOYZ = 4,
    BOOTYSTAT_POWERUPZ = 5,
    BOOTYSTAT_COINZ = 6,
    BOOTYSTAT_SECRETZ = 7,
    BOOTYSTAT_COUNT = 8
GZ_ENUM_END(BootyStatRow)

#endif // GRUNTZ_GRUNTZ_BOOTYSTATROW_H
