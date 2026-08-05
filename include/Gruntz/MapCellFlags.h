#ifndef GRUNTZ_GRUNTZ_MAPCELLFLAGS_H
#define GRUNTZ_GRUNTZ_MAPCELLFLAGS_H

#include <Enums.h>

GZ_ENUM_FLAGS_BEGIN(MapCellFlags, u32)
    CELL_FLAG_SPECIAL = 0x2,
    // CMapMgr::ComputeCellFlags combines this with CELL_FLAG_SPECIAL for
    // TILEKIND_REVEALED_POWERUP. CDroppedObject's retail code tests SPECIAL and
    // then compares the complete word with this bit alone, an unreachable
    // branch preserved from the executable.
    CELL_FLAG_REVEALED_POWERUP = 0x40
GZ_ENUM_FLAGS_END(MapCellFlags, u32)
GZ_ENUM_FLAGS_OPS(MapCellFlags)

#endif // GRUNTZ_GRUNTZ_MAPCELLFLAGS_H
