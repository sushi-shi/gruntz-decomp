#ifndef GRUNTZ_GRUNTMOVEMENTMACROS_H
#define GRUNTZ_GRUNTMOVEMENTMACROS_H

#define GRUNT_AT_SAVED_SCREEN_POS(grunt)                                                           \
    grunt->m_object->m_screenX == grunt->m_lastTilePx.m_x                                          \
        && grunt->m_object->m_screenY == grunt->m_lastTilePx.m_y

#define GRUNT_NOT_AT_SAVED_SCREEN_POS(grunt)                                                       \
    grunt->m_object->m_screenX != grunt->m_lastTilePx.m_x                                          \
        || grunt->m_object->m_screenY != grunt->m_lastTilePx.m_y

#define GRUNT_OBJECT_AT_SAVED_SCREEN_POS(object, grunt)                                            \
    object->m_screenX == grunt->m_lastTilePx.m_x && object->m_screenY == grunt->m_lastTilePx.m_y

#define GRUNT_SCREEN_Y_AT_SAVED_POS(object, grunt) object->m_screenY == grunt->m_lastTilePx.m_y
#define GRUNT_SCREEN_X_NOT_AT_SAVED_POS(object, grunt) object->m_screenX != grunt->m_lastTilePx.m_x
#define GRUNT_SCREEN_Y_NOT_AT_SAVED_POS(object, grunt) object->m_screenY != grunt->m_lastTilePx.m_y

#define GRUNT_OBJECT_NOT_AT_SELF_SAVED_SCREEN_POS(object)                                          \
    object->m_screenX != m_lastTilePx.m_x || object->m_screenY != m_lastTilePx.m_y

#define GRUNT_X_AT_SAVED_POS(x, grunt) x == grunt->m_lastTilePx.m_x
#define DECLARE_SNAPPED_SCREEN_PIXEL_PAIR(object, pixelX, pixelY)                                  \
    i32 pixelX = (object->m_screenX & ~TILE_MASK_PX) + TILE_HALF_PX;                               \
    i32 pixelY = (object->m_screenY & ~TILE_MASK_PX) + TILE_HALF_PX;

#define PIXEL_PAIR_NOT_AT_POSITION(pixelX, pixelY, savedX, savedY)                                 \
    pixelX != savedX || pixelY != savedY

#define COMMIT_GRUNT_NEIGHBOR(target)                                                              \
    CommitNeighbor(                                                                                \
        target->m_playerIndex,                                                                     \
        target->m_unitIndex,                                                                       \
        target->LastTilePx().m_x,                                                                  \
        target->LastTilePx().m_y                                                                   \
    )

#define COPY_LAST_TILE_TO_DEFENDER                                                                 \
    m_defenderPx.m_x = m_lastTilePx.m_x;                                                           \
    m_defenderPx.m_y = m_lastTilePx.m_y;

#define COPY_CURRENT_GRUNT_LAST_TILE_TO_DEFENDER                                                   \
    this->m_defenderPx.m_x = this->m_lastTilePx.m_x;                                               \
    this->m_defenderPx.m_y = this->m_lastTilePx.m_y;

#define SET_GRUNT_ARRIVAL_TARGET(target)                                                           \
    SetEntrancePos(1, 1);                                                                          \
    m_arrivalCell.m_x = target->m_playerIndex;                                                     \
    m_arrivalCell.m_y = target->m_unitIndex

#define BEGIN_GRUNT_ENTRANCE_AND_RELEASE_CELL                                                      \
    m_entranceActive = true;                                                                       \
    m_triggerMgr->RemoveCellRecord(m_playerIndex, m_unitIndex, 1);

#define FIND_NEAREST_ENEMY_AT_TARGET(grunt, atTarget, screenX)                                     \
    CGrunt* grunt = m_triggerMgr->FindNearestEnemy(this);                                          \
    i32 atTarget = 0;                                                                              \
    MARK_NEAREST_ENEMY_AT_TARGET(grunt, atTarget, screenX)

#define FIND_NEAREST_ENEMY_AT_TARGET_WITH_FLAG(grunt, atTarget, screenX)                           \
    CGrunt* grunt = m_triggerMgr->FindNearestEnemy(this);                                          \
    MARK_NEAREST_ENEMY_AT_TARGET(grunt, atTarget, screenX)

#define MARK_NEAREST_ENEMY_AT_TARGET(grunt, atTarget, screenX)                                     \
    if (grunt != NULL) {                                                                           \
        i32 screenX = grunt->m_object->m_screenX;                                                  \
        if (GRUNT_X_AT_SAVED_POS(screenX, grunt)                                                   \
            && grunt->GRUNT_SCREEN_Y_AT_SAVED_POS(m_object, grunt)                                 \
            && RectContains(screenX, grunt->m_object->m_screenY) != 0) {                           \
            atTarget = 1;                                                                          \
        }                                                                                          \
    }

#endif // GRUNTZ_GRUNTMOVEMENTMACROS_H
