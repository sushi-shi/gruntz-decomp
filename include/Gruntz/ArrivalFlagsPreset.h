#ifndef GRUNTZ_GRUNTZ_ARRIVALFLAGSPRESET_H
#define GRUNTZ_GRUNTZ_ARRIVALFLAGSPRESET_H

#include <Bute/ButeMgr.h>
#include <Enums.h>
#include <Gruntz/EnemyAiType.h>
#include <Gruntz/GameModeId.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntzMgr.h>
#include <RectMacros.h>

GZ_ENUM_CONST_BEGIN(ArrivalFlagsPreset)
    ARRIVAL_FLAGS_PLAYER = 0x4000901,
    ARRIVAL_FLAGS_PLAYER_SINGLE = 0x4000911,
    ARRIVAL_FLAGS_BATTLEZ = 0x4000983,
    ARRIVAL_FLAGS_ENEMY = 0x1c000d83
GZ_ENUM_CONST_END(ArrivalFlagsPreset)

inline void CGrunt::ResetArrivalFlags() {
    if (m_aiType == AI_NONE) {
        m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
    } else if (m_aiType == AI_BATTLEZ_PATH) {
        m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
    } else {
        m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
    }
}

inline void MarkQuestzArrival(CGrunt* grunt) {
    if (g_gameReg->GetGameMode() == GAMEMODE_QUESTZ) {
        grunt->m_arrivalFlags |= 0x10;
    }
}

#define BEGIN_GUARD(grunt)                                                                         \
    {                                                                                              \
        (grunt)->m_arrivalRerollTiming.Clear();                                                    \
        (grunt)->m_guarding = true;                                                                \
        (grunt)->m_defenderPx = (grunt)->m_lastTilePx;                                             \
        PickupType kind = (grunt)->GetActivePickupType();                                          \
                                                                                                   \
        switch (kind) {                                                                            \
            case PICKUP_BOOMERANG:                                                                 \
            case PICKUP_GUNHAT:                                                                    \
            case PICKUP_NERFGUN:                                                                   \
            case PICKUP_ROCK:                                                                      \
            case PICKUP_WELDER:                                                                    \
            case PICKUP_WINGZ:                                                                     \
                (grunt)->m_defenderRadius = 1;                                                     \
                break;                                                                             \
            default:                                                                               \
                (grunt)->m_defenderRadius =                                                        \
                    g_buteMgr.GetInt("Grunt", "PlayerDefenderRadius", 3) + 1;                      \
                break;                                                                             \
        }                                                                                          \
        (grunt)->m_aiType = AI_DEFENDER;                                                           \
        (grunt)->m_aiState = AISTATE_SEEK;                                                         \
        UNSET_COORD((grunt)->m_arrivalCell);                                                       \
        (grunt)->m_actionTargetsGrunt = false;                                                     \
        (grunt)->m_arrivalFlags |= 0x18040402;                                                     \
        SET_RECT_XY_EXTENTS((grunt)->m_object->m_extent, 0, 0, 0, 0);                              \
        (grunt)->SetEntrancePos(1, 1);                                                             \
    }

#define END_GUARD(grunt)                                                                           \
    {                                                                                              \
        (grunt)->m_arrivalRerollTiming.Clear();                                                    \
        (grunt)->m_guarding = false;                                                               \
        (grunt)->m_aiType = AI_NONE;                                                               \
        (grunt)->m_arrivalFlags &= 0xe7fbfbfd;                                                     \
        (grunt)->SetEntrancePos(1, 1);                                                             \
    }

#endif // GRUNTZ_GRUNTZ_ARRIVALFLAGSPRESET_H
