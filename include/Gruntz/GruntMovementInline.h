#ifndef GRUNTZ_GRUNTMOVEMENTINLINE_H
#define GRUNTZ_GRUNTMOVEMENTINLINE_H

#include <Gruntz/FreeNodePool.h>
#include <Gruntz/Grunt.h>

inline i32 IsGruntAtSavedScreenPos(CGrunt* grunt) {
    CWwdGameObjectA* object = grunt->m_object;
    i32 x = grunt->m_lastTilePx.m_x;
    if (object->m_screenX == x && object->m_screenY == grunt->m_lastTilePx.m_y) {
        return 1;
    }
    return 0;
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
