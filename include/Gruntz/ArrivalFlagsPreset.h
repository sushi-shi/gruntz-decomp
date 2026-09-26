#ifndef GRUNTZ_GRUNTZ_ARRIVALFLAGSPRESET_H
#define GRUNTZ_GRUNTZ_ARRIVALFLAGSPRESET_H

#include <Enums.h>
#include <Gruntz/EnemyAiType.h>
#include <Gruntz/GameModeId.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntzMgr.h>

GZ_ENUM_CONST_BEGIN(ArrivalFlagsPreset)
    ARRIVAL_FLAGS_PLAYER = 0x4000901,
    ARRIVAL_FLAGS_PLAYER_SINGLE = 0x4000911,
    ARRIVAL_FLAGS_BATTLEZ = 0x4000983,
    ARRIVAL_FLAGS_ENEMY = 0x1c000d83
GZ_ENUM_CONST_END(ArrivalFlagsPreset)

inline void ResetArrivalFlags(CGrunt* grunt) {
    if (grunt->m_arrivalState == AI_NONE) {
        grunt->m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
    } else if (grunt->m_arrivalState == AI_BATTLEZ_PATH) {
        grunt->m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
    } else {
        grunt->m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
    }
}

inline void MarkQuestzArrival(CGrunt* grunt) {
    if (g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
        grunt->m_arrivalFlags |= 0x10;
    }
}

#endif // GRUNTZ_GRUNTZ_ARRIVALFLAGSPRESET_H
