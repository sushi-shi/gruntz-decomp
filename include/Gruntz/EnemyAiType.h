#ifndef GRUNTZ_GRUNTZ_ENEMYAITYPE_H
#define GRUNTZ_GRUNTZ_ENEMYAITYPE_H

#include <Enums.h>

// An enemy Grunt's "nature", selected by the WWD `Points:` field
// (CGameObject +0x118) and valid only when `Smarts:` (+0x124, the team number)
// is non-zero. Names from docs/domain/enemy-ai.md, distilled from the level
// editor's AppendixC pages (01-DumbChaserz.html ... 16-ScrollGrunt.html).
//
// Not yet applied: the 16-way dispatch on +0x118 in the CGrunt brain has not
// been located. Apply it there once found, not by RVA proximity.
GZ_ENUM_BEGIN(EnemyAiType)
    AI_NONE = 0,
    AI_DUMBCHASER = 1,
    AI_SMARTCHASER = 2,
    AI_HITANDRUNNER = 3,
    AI_DEFENDER = 4,
    AI_POSTGUARD = 5,
    AI_OBJECTGUARD = 6,
    AI_BOMBER = 7,
    AI_BRICKLAYER = 8,
    AI_GAUNTLETZGRUNT = 9,
    AI_GOOSUCKER = 10,
    AI_DIGGER = 11,
    AI_TIMEBOMBER = 12,
    AI_TOOLTHIEF = 13,
    AI_TOYER = 14,
    AI_MAGICWANDGRUNT = 15,
    AI_SCROLLGRUNT = 16,
    AI_COUNT = 17
GZ_ENUM_END(EnemyAiType)

#endif // GRUNTZ_GRUNTZ_ENEMYAITYPE_H
