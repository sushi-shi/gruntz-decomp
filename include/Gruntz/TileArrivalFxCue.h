#ifndef GRUNTZ_TILEARRIVALFXCUE_H
#define GRUNTZ_TILEARRIVALFXCUE_H

#include <Enums.h>

// ANI draw values consumed by CTriggerMgr::LoadTileArrivalFx. Advance() is a
// generic integer-valued animation interface; this consumer interprets only
// these three values as its tile-effect protocol.
GZ_ENUM_BEGIN(TileArrivalFxCue)
    TILE_ARRIVAL_FX_END = -1,
    TILE_ARRIVAL_FX_IMPACT = 2,
    TILE_ARRIVAL_FX_APPLY = 0x63
GZ_ENUM_END(TileArrivalFxCue)

#endif // GRUNTZ_TILEARRIVALFXCUE_H
