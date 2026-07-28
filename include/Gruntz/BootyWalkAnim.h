// BootyWalkAnim.h - the BootyWalkAnim TU's exported globals/data.
#ifndef GRUNTZ_GRUNTZ_BOOTYWALKANIM_H
#define GRUNTZ_GRUNTZ_BOOTYWALKANIM_H

#include <rva.h>
#include <Gruntz/CoordNode.h> // Coord {x,y}

extern i32 g_idleSpriteIds[4];

// 0x1e9078 - the multi-booty scoreboard geometry (8 rows x 4 player columns of
// on-screen {x,y} anchors). Defined in BootyWalkAnim.cpp (its .rdata band);
// read by CMultiBootyState::LoadGameAssetNamespaces.
extern Coord g_multiBootyGeom[8][4];
#endif // GRUNTZ_GRUNTZ_BOOTYWALKANIM_H
