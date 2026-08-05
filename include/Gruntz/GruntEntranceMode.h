#ifndef GRUNTZ_GRUNTZ_GRUNTENTRANCEMODE_H
#define GRUNTZ_GRUNTZ_GRUNTENTRANCEMODE_H

#include <Enums.h>

// Controls how a grunt enters the board.  Place treats zero as an immediate
// placement with no entrance animation.  BuildEntranceAnimation also receives
// zero directly from the combat resurrection path, where its default arm is
// the resurrection animation; the alias records that narrower legacy use.
GZ_ENUM_BEGIN(GruntEntranceMode)
    GRUNT_ENTRANCE_NONE = 0,
    GRUNT_ENTRANCE_RESURRECT_DIRECT = 0,
    GRUNT_ENTRANCE_WORMHOLE = 1,
    GRUNT_ENTRANCE_DROP = 2,
    GRUNT_ENTRANCE_RESURRECT = 3
GZ_ENUM_END(GruntEntranceMode)

#endif // GRUNTZ_GRUNTZ_GRUNTENTRANCEMODE_H
