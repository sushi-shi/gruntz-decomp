#ifndef GRUNTZ_GRUNTZ_GRUNTNEIGHBORHOODINLINE_H
#define GRUNTZ_GRUNTZ_GRUNTNEIGHBORHOODINLINE_H

#include <Gruntz/GruntMovementInline.h>
#include <RectMacros.h>

static inline Coord ScreenPosition(CGameObject* object) {
    Coord out;
    i32 y = object->m_screenY;
    i32 x = object->m_screenX;
    out.Set(x, y);
    return out;
}

static inline RECT AttackTileNeighborhood(CGrunt* grunt) {
    i32 halfBox = grunt->m_defenderRadius + grunt->m_reachRect.right + 1;
    CGameObject* object = grunt->m_object;
    Coord pt1 = ScreenPosition(object);
    ScreenTile(&pt1);
    i32 by = pt1.m_y;
    Coord pt2 = ScreenPosition(object);
    ScreenTile(&pt2);
    i32 bx = pt2.m_x;
    Coord pt3 = ScreenPosition(object);
    ScreenTile(&pt3);
    i32 topY = pt3.m_y;
    Coord pt4 = ScreenPosition(object);
    pt4.m_x >>= TILE_SHIFT_PX;
    i32 leftX = pt4.m_x;
    RECT box;
    SET_RECT_COMPONENTS(box, leftX - halfBox, topY - halfBox, bx + halfBox + 1, by + halfBox + 1);
    return box;
}

static inline RECT AdjacentTileNeighborhood(CGrunt* grunt) {
    Coord high = ScreenTile(grunt);
    Coord low = ScreenTile(grunt);
    RECT box;
    box.top = low.m_y - 1;
    box.bottom = high.m_y + 2;
    box.left = low.m_x - 1;
    box.right = high.m_x + 2;
    return box;
}

#endif // GRUNTZ_GRUNTZ_GRUNTNEIGHBORHOODINLINE_H
