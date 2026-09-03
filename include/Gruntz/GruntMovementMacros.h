#ifndef GRUNTZ_GRUNTMOVEMENTMACROS_H
#define GRUNTZ_GRUNTMOVEMENTMACROS_H

#define GRUNT_AT_SAVED_SCREEN_POS(grunt) grunt->m_object->ScreenPos() == grunt->m_lastTilePx

#define GRUNT_NOT_AT_SAVED_SCREEN_POS(grunt) grunt->m_object->ScreenPos() != grunt->m_lastTilePx

#define GRUNT_OBJECT_AT_SAVED_SCREEN_POS(object, grunt) object->ScreenPos() == grunt->m_lastTilePx

#define GRUNT_OBJECT_NOT_AT_SELF_SAVED_SCREEN_POS(object) object->ScreenPos() != m_lastTilePx

#define COMMIT_GRUNT_NEIGHBOR(target)                                                              \
    CommitNeighbor(                                                                                \
        target->m_playerIndex,                                                                     \
        target->m_unitIndex,                                                                       \
        target->LastTilePx().m_x,                                                                  \
        target->LastTilePx().m_y                                                                   \
    )

#define COPY_LAST_TILE_TO_DEFENDER m_defenderPx = m_lastTilePx;

#define COPY_CURRENT_GRUNT_LAST_TILE_TO_DEFENDER this->m_defenderPx = this->m_lastTilePx;

#define SET_GRUNT_ARRIVAL_TARGET(target)                                                           \
    SetEntrancePos(1, 1);                                                                          \
    m_arrivalCell.Set(target->m_playerIndex, target->m_unitIndex)

#define BEGIN_GRUNT_ENTRANCE_AND_RELEASE_CELL                                                      \
    m_entranceActive = true;                                                                       \
    m_triggerMgr->RemoveCellRecord(m_playerIndex, m_unitIndex, 1);

#define FIND_NEAREST_ENEMY_AT_TARGET(grunt, atTarget)                                              \
    CGrunt* grunt = m_triggerMgr->FindNearestEnemy(this);                                          \
    i32 atTarget = 0;                                                                              \
    MARK_NEAREST_ENEMY_AT_TARGET(grunt, atTarget)

#define FIND_NEAREST_ENEMY_AT_TARGET_WITH_FLAG(grunt, atTarget)                                    \
    CGrunt* grunt = m_triggerMgr->FindNearestEnemy(this);                                          \
    MARK_NEAREST_ENEMY_AT_TARGET(grunt, atTarget)

#define MARK_NEAREST_ENEMY_AT_TARGET(grunt, atTarget)                                              \
    if (grunt != NULL) {                                                                           \
        Coord screenPosition = grunt->m_object->ScreenPos();                                       \
        if (screenPosition == grunt->m_lastTilePx                                                  \
            && RectContains(screenPosition.m_x, screenPosition.m_y) != 0) {                        \
            atTarget = 1;                                                                          \
        }                                                                                          \
    }

#endif // GRUNTZ_GRUNTMOVEMENTMACROS_H
