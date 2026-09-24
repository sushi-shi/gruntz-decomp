#ifndef GRUNTZ_GRUNTMOVEMENTINLINE_H
#define GRUNTZ_GRUNTMOVEMENTINLINE_H

#include <Gruntz/FreeNodePool.h>
#include <Gruntz/FreeNodePoolInline.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntDirectionOffset.h>
#include <Gruntz/TriggerMgr.h>

inline i32 IsGruntAtSavedScreenPos(CGrunt* grunt) {
    return COORD_EQUALS_COMPONENTS(
        grunt->m_object->m_screenPosition,
        grunt->m_lastTilePx.m_x,
        grunt->m_lastTilePx.m_y
    );
}

inline i32 IsObjectAtGruntSavedScreenPos(CGameObject* object, CGrunt* grunt) {
    return object->ScreenPos() == grunt->m_lastTilePx;
}

inline void CopyLastTileToDefender(CGrunt* grunt) {
    grunt->m_defenderPx = grunt->m_lastTilePx;
}

inline i32 CommitGruntNeighbor(CGrunt* grunt, CGrunt* target) {
    return grunt->CommitNeighbor(
        target->m_playerIndex,
        target->m_unitIndex,
        target->LastTilePx().m_x,
        target->LastTilePx().m_y
    );
}

inline void SetGruntArrivalTarget(CGrunt* grunt, CGrunt* target) {
    grunt->SetEntrancePos(1, 1);
    grunt->m_arrivalCell.Set(target->m_playerIndex, target->m_unitIndex);
}

inline void BeginGruntEntranceAndReleaseCell(CGrunt* grunt) {
    grunt->m_entranceActive = true;
    grunt->m_triggerMgr->RemoveCellRecord(grunt->m_playerIndex, grunt->m_unitIndex, 1);
}

inline void ResetGruntPoweredState(CGrunt* grunt) {
    grunt->m_entranceActive = false;
    grunt->m_combatActive = false;
    grunt->m_neighborValid = false;
    grunt->m_poweredUp = false;
    grunt->ResetEntranceAnimation(1, 0, 0);
}

inline void MarkNearestEnemyAtTarget(CGrunt* grunt, CGrunt* target, i32* atTarget) {
    if (target != NULL) {
        Coord screenPosition = target->m_object->ScreenPos();
        if (screenPosition == target->m_lastTilePx
            && grunt->RectContains(screenPosition.m_x, screenPosition.m_y) != 0) {
            *atTarget = 1;
        }
    }
}

inline CGrunt* FindNearestEnemyAtTarget(CGrunt* grunt, i32* atTarget) {
    CGrunt* target = grunt->m_triggerMgr->FindNearestEnemy(grunt);
    *atTarget = 0;
    MarkNearestEnemyAtTarget(grunt, target, atTarget);
    return target;
}

inline void CGrunt::MirrorAcrossArrival() {
    Coord current;
    GetScreenTile(&current);
    Coord mirrored = current * 2 - m_arrivalCell;
    TileSwitch(mirrored.m_x, mirrored.m_y, 0, m_arrivalFlags, 1, 0);
}

inline void RecycleGruntCoords(CGrunt* grunt) {
    if (grunt->CoordCount() == 0) {
        return;
    }
    POSITION node = grunt->CoordHead();
    if (node != NULL) {
        do {
            POSITION current = node;
            grunt->m_coordList.GetNext(node);
            Coord* coord = static_cast<Coord*>(grunt->m_coordList.GetAt(current));
            if (coord != NULL) {
                PushFreeNode(&g_coordPool, coord);
            }
        } while (node != NULL);
    }
    grunt->m_coordList.RemoveAll();
}

inline Coord ScreenTile(CGrunt* unit) {
    Coord out;
    CGameObject* object = unit->m_object;
    out.m_x = object->m_screenPosition.m_x >> TILE_SHIFT_PX;
    out.m_y = object->m_screenPosition.m_y >> TILE_SHIFT_PX;
    return out;
}

inline i32 CGrunt::GetScreenTileX() const {
    return m_object->m_screenPosition.m_x >> TILE_SHIFT_PX;
}

inline i32 CGrunt::GetScreenTileY() const {
    return m_object->m_screenPosition.m_y >> TILE_SHIFT_PX;
}

inline Coord CGrunt::ScanCell() {
    Coord t;
    GetScreenTile(&t);
    return t;
}

inline void
SetEntranceDirection(CGrunt* grunt, const GruntDirectionCell& direction, Coord* newPosition) {
    grunt->m_entranceCell = direction;
    *newPosition = grunt->m_lastTilePx - GruntDirectionPixelOffset(direction);
}

inline void SetMovingDeathDirection(CGrunt* grunt, const GruntDirectionCell& direction) {
    grunt->m_entranceCell = direction;
    grunt->m_lastTilePx += GruntDirectionPixelOffset(direction) / 2;
}

inline void InitializeVehicleContactRegion(CGrunt* grunt) {
    CRect contact(-1, -1, 1, 1);
    grunt->m_vehicleContactRect = contact;
    contact.SetRectEmpty();
    grunt->m_vehicleContactExclusionRect = contact;
}

#endif // GRUNTZ_GRUNTMOVEMENTINLINE_H
