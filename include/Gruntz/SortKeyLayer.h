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
//   SORTKEY_INGAME_INFO the in-game pickup icon and help-text classes
//   SORTKEY_TELEPORT    wormholes and teleporters
//
GZ_ENUM_CONST_BEGIN(SortKeyLayer)
    SORTKEY_BOOTY_WARLORD = 2,
    SORTKEY_GRUNT_CREATION = 5,
    SORTKEY_ACTION_AREA = 6,
    SORTKEY_GRUNT_PUDDLE = 10,
    SORTKEY_TOOB_SPIKE = 12,
    SORTKEY_PROJECTILE = 15,
    SORTKEY_KITCHEN_SLIME = 19,
    SORTKEY_GRUNT_SELECTED = 20,
    SORTKEY_GRUNT_POWERUP = 21,
    SORTKEY_EXIT_TRIGGER = 75000,
    SORTKEY_GRUNT_DEATH = 90000,
    SORTKEY_TELEPORT = 0x1869f,
    SORTKEY_ROLLING_BALL_BASE = 100000,
    SORTKEY_INGAME_INFO = 0x17318,
    SORTKEY_INGAME_INFO_FX = 0x17319,
    SORTKEY_ACTOR = 0xcf850,
    // One either side of the actor layer, for a sprite that belongs among the
    // grunts but must resolve behind or in front of them.
    SORTKEY_ACTOR_BEHIND = 0xcf84f,
    SORTKEY_ACTOR_FRONT = 0xcf851,
    SORTKEY_GRUNT_HUD = 0xdbba0,
    SORTKEY_GRUNT_VOICE = 0xdbba1,
    SORTKEY_WARLORD = 800000,
    SORTKEY_OVERLAY = 0xf4240
GZ_ENUM_CONST_END(SortKeyLayer)

#endif // GRUNTZ_GRUNTZ_SORTKEYLAYER_H
