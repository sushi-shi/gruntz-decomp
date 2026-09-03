#ifndef GRUNTZ_GRUNTMOVEMENTINLINE_H
#define GRUNTZ_GRUNTMOVEMENTINLINE_H

#include <Gruntz/FreeNodePool.h>
#include <Gruntz/Grunt.h>

inline i32 IsGruntAtSavedScreenPos(CGrunt* grunt) {
    return grunt->m_object->ScreenPos() == grunt->m_lastTilePx;
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
                g_coordPool.Push(coord);
            }
        } while (node != NULL);
    }
    grunt->m_coordList.RemoveAll();
}

#endif // GRUNTZ_GRUNTMOVEMENTINLINE_H
