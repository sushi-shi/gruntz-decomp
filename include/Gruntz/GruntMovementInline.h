#ifndef GRUNTZ_GRUNTMOVEMENTINLINE_H
#define GRUNTZ_GRUNTMOVEMENTINLINE_H

#include <Gruntz/FreeNodePool.h>
#include <Gruntz/FreeNodePoolInline.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/TriggerMgr.h>

inline i32 IsGruntAtSavedScreenPos(CGrunt* grunt) {
    return grunt->m_object->ScreenPos() == grunt->m_lastTilePx;
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

inline void MirrorGruntAcrossArrival(CGrunt* grunt) {
    Coord current;
    grunt->GetScreenTile(&current);
    Coord mirrored = current * 2 - grunt->m_arrivalCell;
    grunt->TileSwitch(mirrored.m_x, mirrored.m_y, 0, grunt->m_arrivalFlags, 1, 0);
}

inline void RecycleGruntCoords(CGrunt* grunt) {
    if (grunt->CoordCount() == 0) {
        return;
    }
    CoordNode* node = grunt->CoordHead();
    if (node != NULL) {
        do {
            CoordNode* current = node;
            node = node->m_next;
            Coord* coord = current->m_coord;
            if (coord != NULL) {
                PushFreeNode(&g_coordPool, coord);
            }
        } while (node != NULL);
    }
    grunt->m_coordList.RemoveAll();
}

#endif // GRUNTZ_GRUNTMOVEMENTINLINE_H
