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

#define GRUNT_X_AT_SAVED_POS(x, grunt) ((x) == (grunt)->m_lastTilePx.m_x)
#define DECLARE_SNAPPED_SCREEN_PIXEL_PAIR(object, pixelX, pixelY)                                  \
    i32 pixelX = (object->m_screenX & ~TILE_MASK_PX) + TILE_HALF_PX;                               \
    i32 pixelY = (object->m_screenY & ~TILE_MASK_PX) + TILE_HALF_PX;

#define PIXEL_PAIR_NOT_AT_POSITION(pixelX, pixelY, savedX, savedY)                                 \
    pixelX != savedX || pixelY != savedY

#define ATTACK_GRUNT(target)                                                                       \
    AttackGrunt(                                                                                   \
        target->GetPlayerIndex(),                                                                  \
        target->GetUnitIndex(),                                                                    \
        target->LastTilePx().m_x,                                                                  \
        target->LastTilePx().m_y                                                                   \
    )

#define COMMIT_HIT_AND_RUN_ATTACK(target)                                                          \
    do {                                                                                           \
        ATTACK_GRUNT(target);                                                                      \
        m_neighborScanEnabled = false;                                                             \
        RecycleCoords();                                                                           \
        m_aiState = AISTATE_RETREAT;                                                               \
    } while (0)

#define COPY_LAST_TILE_TO_DEFENDER                                                                 \
    m_defenderPx.m_x = m_lastTilePx.m_x;                                                           \
    m_defenderPx.m_y = m_lastTilePx.m_y;

#define COPY_CURRENT_GRUNT_LAST_TILE_TO_DEFENDER                                                   \
    this->m_defenderPx.m_x = this->m_lastTilePx.m_x;                                               \
    this->m_defenderPx.m_y = this->m_lastTilePx.m_y;

#define SET_GRUNT_ARRIVAL_TARGET(target)                                                           \
    SetEntrancePos(1, 1);                                                                          \
    m_arrivalCell.m_x = target->GetPlayerIndex();                                                  \
    m_arrivalCell.m_y = target->GetUnitIndex()

#define FIND_NEAREST_ENEMY_AT_TARGET(grunt, atTarget)                                              \
    CGrunt* grunt = m_triggerMgr->FindNearestEnemy(this);                                          \
    i32 atTarget = 0;                                                                              \
    MARK_NEAREST_ENEMY_AT_TARGET(grunt, atTarget)

#define FIND_NEAREST_ENEMY_AT_TARGET_WITH_FLAG(grunt, atTarget)                                    \
    CGrunt* grunt = m_triggerMgr->FindNearestEnemy(this);                                          \
    MARK_NEAREST_ENEMY_AT_TARGET(grunt, atTarget)

#define MARK_NEAREST_ENEMY_AT_TARGET(grunt, atTarget)                                              \
    if (grunt != NULL) {                                                                           \
        if (IsGruntAtSavedScreenPos(grunt)                                                         \
            && IsWithinReach(grunt->m_object->m_screenX, grunt->m_object->m_screenY) != 0) {       \
            atTarget = 1;                                                                          \
        }                                                                                          \
    }

#define SETDIR(cell, nx, ny)                                                                       \
    do {                                                                                           \
        newPos.m_y = (ny);                                                                         \
        newPos.m_x = (nx);                                                                         \
        this->m_facing = (cell);                                                                   \
    } while (0)

#define MV_VEC(V) m_facing = g_gruntDir##V

#define MV_N                                                                                       \
    MV_VEC(North);                                                                                 \
    m_lastTilePx.m_y -= 0x10

#define MV_S                                                                                       \
    MV_VEC(South);                                                                                 \
    m_lastTilePx.m_y += 0x10

#define MV_E                                                                                       \
    MV_VEC(East);                                                                                  \
    m_lastTilePx.m_x += 0x10

#define MV_W                                                                                       \
    MV_VEC(West);                                                                                  \
    m_lastTilePx.m_x -= 0x10

#define MV_NE                                                                                      \
    MV_VEC(NorthEast);                                                                             \
    m_lastTilePx.m_x += 0x10;                                                                      \
    m_lastTilePx.m_y -= 0x10

#define MV_NW                                                                                      \
    MV_VEC(NorthWest);                                                                             \
    m_lastTilePx.m_x -= 0x10;                                                                      \
    m_lastTilePx.m_y -= 0x10

#define MV_SE                                                                                      \
    MV_VEC(SouthEast);                                                                             \
    m_lastTilePx.m_x += 0x10;                                                                      \
    m_lastTilePx.m_y += 0x10

#define MV_SW                                                                                      \
    MV_VEC(SouthWest);                                                                             \
    m_lastTilePx.m_x -= 0x10;                                                                      \
    m_lastTilePx.m_y += 0x10

#define INIT_TOY_USE_RECTS()                                                                       \
    do {                                                                                           \
        RECT bounds;                                                                               \
        bounds.left = -1;                                                                          \
        bounds.top = -1;                                                                           \
        bounds.right = 1;                                                                          \
        bounds.bottom = 1;                                                                         \
        m_toyUseRect = bounds;                                                                     \
        bounds.left = 0;                                                                           \
        bounds.top = 0;                                                                            \
        bounds.right = 0;                                                                          \
        bounds.bottom = 0;                                                                         \
        m_toyUseExclusionRect = bounds;                                                            \
    } while (0)

#endif // GRUNTZ_GRUNTMOVEMENTMACROS_H
