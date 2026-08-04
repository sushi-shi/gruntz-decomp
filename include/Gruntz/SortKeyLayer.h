#ifndef GRUNTZ_GRUNTZ_SORTKEYLAYER_H
#define GRUNTZ_GRUNTZ_SORTKEYLAYER_H

#include <Enums.h>

// Draw-order layers, as assigned to CResolveNode::m_sortKey.
//
// The scheme gives itself away by being round in DECIMAL while written in hex:
// 0xcf850 is 850000, 0xdbba0 is 900000, 0xf4240 is 1000000. Round numbers spaced
// far apart are layer bases, and the near neighbours confirm it - CDroppedObject
// uses 0xcf84f and 0xcf851, one either side of the actor layer, to sit just
// behind or just in front of the grunts without leaving it.
//
// Only tiers with SEVERAL independent users are named, on the same standard as
// everything else in this campaign:
//
//   SORTKEY_ACTOR    six classes - grunts and everything drawn among them
//                    (GruntEntranceMove, PathHazard, Projectile, SpotLight,
//                    DroppedObject, FortressFlag)
//   SORTKEY_GRUNT_HUD  the five per-grunt indicator sprites - health, stamina,
//                    toy, toy-time, wingz-time
//   SORTKEY_OVERLAY  the front-most layer - front candy, the status bar's
//                    sprites, the fortress flag's banner
//
// The single-class layers (Warlord at 800000, InGameIcon at 95000, Wormhole at
// 99999, ExitTrigger at 75000) keep their literals: naming a layer after the one
// class that uses it would say nothing the assignment does not already say.
GZ_ENUM_CONST_BEGIN(SortKeyLayer)
    SORTKEY_ACTOR = 0xcf850,
    // One either side of the actor layer, for a sprite that belongs among the
    // grunts but must resolve behind or in front of them.
    SORTKEY_ACTOR_BEHIND = 0xcf84f,
    SORTKEY_ACTOR_FRONT = 0xcf851,
    SORTKEY_GRUNT_HUD = 0xdbba0,
    SORTKEY_OVERLAY = 0xf4240
GZ_ENUM_CONST_END(SortKeyLayer)

#endif // GRUNTZ_GRUNTZ_SORTKEYLAYER_H
