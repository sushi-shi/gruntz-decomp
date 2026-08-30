#ifndef GRUNTZ_GRUNTMOVEMENTINLINE_H
#define GRUNTZ_GRUNTMOVEMENTINLINE_H

#include <Gruntz/FreeNodePool.h>
#include <Gruntz/Grunt.h>

inline i32 IsGruntAtSavedScreenPos(CGrunt* grunt) {
    CWwdSpriteObject* object = grunt->m_object;
    i32 x = grunt->m_lastTilePx.m_x;
    if (object->m_screenX == x && object->m_screenY == grunt->m_lastTilePx.m_y) {
        return 1;
    }
    return 0;
}

inline void MirrorGruntAcrossArrival(CGrunt* grunt) {
    Coord pa;
    grunt->GetScreenTile(&pa);
    Coord pb;
    pb.m_y = pa.m_y;
    grunt->GetScreenPos(&pb);
    i32 gx = (pb.m_x >> TILE_SHIFT_PX) - grunt->m_arrivalCell.m_x + pa.m_x;
    grunt->GetScreenTile(&pa);
    pb.m_x = pa.m_x;
    grunt->GetScreenPos(&pb);
    i32 gy = (pb.m_y >> TILE_SHIFT_PX) - grunt->m_arrivalCell.m_y + pa.m_y;
    grunt->TileSwitch(gx, gy, 0, grunt->m_arrivalFlags, 1, 0);
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
                g_coordPool.Push(coord);
            }
        } while (node != NULL);
    }
    grunt->m_coordList.RemoveAll();
}

#endif // GRUNTZ_GRUNTMOVEMENTINLINE_H
