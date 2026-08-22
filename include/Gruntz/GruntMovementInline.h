#ifndef GRUNTZ_GRUNTMOVEMENTINLINE_H
#define GRUNTZ_GRUNTMOVEMENTINLINE_H

#include <Gruntz/FreeNodePool.h>
#include <Gruntz/Grunt.h>

// Two opt-in inline visibility devices, each the expanded twin of a pinned
// CGrunt member.  WORKAROUNDS for caller-side modelling error, not proven era
// structure - no dev writes a per-TU visibility header, and no dev writes a
// member as a free function beside itself.  Each was collapsed to ONE inline
// member in Grunt.h carrying its RVA pin, twin and call sites rewritten
// (2026-08-22); both lost the pinned COMDAT's only emitter, because every one
// of Grunt.h's 86 TUs then expands it and none declines:
//   * IsAtSavedScreenPos (0x29a80, BattlezMapConfig.cpp) 100.00 -> 0.00,
//     verify unique-names FATAL, -122 total;
//   * RecycleCoords (0x343f0, BattlezSpecialAnim.cpp) 100.00 -> 0.00,
//     unique-names FATAL, -111 total, -1 exact.
// RecycleCoords is doubly layered: the two copies are not even the same text -
// this one calls g_coordPool.Push while the pinned body expands PushFreeNode,
// so it also rides <Gruntz/FreeNodePoolInline.h>'s device.
// REMOVAL CONDITION: model the retail callers accurately enough that at least
// one declines on its own budget and homes the COMDAT; then one visible body
// in Grunt.h reproduces the split.
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
