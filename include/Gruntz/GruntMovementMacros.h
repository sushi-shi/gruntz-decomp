#ifndef GRUNTZ_GRUNTMOVEMENTMACROS_H
#define GRUNTZ_GRUNTMOVEMENTMACROS_H

// CGrunt::IsAtSavedScreenPos (0x29a80) expanded at the sites where retail
// inlined it, plus the partial one-axis tests.  The call/expand split itself is
// documented at <Gruntz/GruntMovementInline.h>, which owns the twin.
// The receiver-only variants (GRUNT_SELF_*, PIXEL_PAIR_NOT_AT_SELF_*) were
// folded onto the parameterized forms 2026-08-22 by passing `this`, byte-neutral
// (0 rows moved); nine further variants had no use site at all.

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
        target->m_tileOwnerHi,                                                                     \
        target->m_tileOwnerLo,                                                                     \
        target->m_lastTilePx.m_x,                                                                  \
        target->m_lastTilePx.m_y                                                                   \
    )

#define COMMIT_GRUNT_NEIGHBOR_COPY(target, coord)                                                  \
    Coord coord = target->m_lastTilePx;                                                            \
    CommitNeighbor(target->m_tileOwnerHi, target->m_tileOwnerLo, coord.m_x, coord.m_y)

#define COPY_LAST_TILE_TO_DEFENDER                                                                 \
    m_defenderPx.m_x = m_lastTilePx.m_x;                                                           \
    m_defenderPx.m_y = m_lastTilePx.m_y;

#define COPY_CURRENT_GRUNT_LAST_TILE_TO_DEFENDER                                                   \
    this->m_defenderPx.m_x = this->m_lastTilePx.m_x;                                               \
    this->m_defenderPx.m_y = this->m_lastTilePx.m_y;

#define SET_GRUNT_ARRIVAL_TARGET(target)                                                           \
    SetEntrancePos(1, 1);                                                                          \
    m_arrivalCell.m_x = target->m_tileOwnerHi;                                                     \
    m_arrivalCell.m_y = target->m_tileOwnerLo

#define BEGIN_GRUNT_ENTRANCE_AND_RELEASE_CELL                                                      \
    m_entranceActive = 1;                                                                          \
    m_tileMgr->RemoveCellRecord(m_tileOwnerHi, m_tileOwnerLo, 1);

#define FIND_NEAREST_ENEMY_AT_TARGET(grunt, atTarget, screenX)                                     \
    CGrunt* grunt = m_tileMgr->FindNearestEnemy(this);                                             \
    i32 atTarget = 0;                                                                              \
    MARK_NEAREST_ENEMY_AT_TARGET(grunt, atTarget, screenX)

#define FIND_NEAREST_ENEMY_AT_TARGET_WITH_FLAG(grunt, atTarget, screenX)                           \
    CGrunt* grunt = m_tileMgr->FindNearestEnemy(this);                                             \
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
