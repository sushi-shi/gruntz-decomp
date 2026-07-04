// CViewport.h - the world plane / world->screen transform object (reached as
// g_gameReg->m_resMgr->m_view->m_viewport, and g_gameReg->m_world->m_24->m_5c).
// ONE object, several field views recovered from different call sites - all folded
// here so every consumer reaches it typed, cast-free:
//   * transform view (ActionOptionsMenuBar / UserLogic): WrapCoord clamps a screen
//     coord into the world; m_worldWidth (+0x30) clamps the bar position; the
//     visible-bounds RECT lives at +0x40 (read as the m_5c base + 0x40); +0x5c is
//     the visible-rect base pointer.
//   * tile-grid view (LevelTileValidation / TileActionEvent / TileGridCommand /
//     TriggerMgr / GroupOps): the plane's tile map - a flat cell array (m_cells,
//     +0x20) indexed by a per-row start-index table (m_rowBase, +0x24), so
//     cell(x,y) = m_cells[m_rowBase[y] + x]; m_worldWidth/m_worldHeight (+0x30/
//     +0x34) are the clamp bounds; m_shiftX/m_shiftY (+0x8c/+0x90) convert world
//     coords to tile coords (tx = x >> m_shiftX). Cells are plain i32 (a
//     tile-class index; -1 / 0xeeeeeeee = empty). This is the SAME +0x5c object the
//     transform view reaches (proven by the shared +0x30 world-width field).
// WrapCoord is external/no-body so the call reloc-masks. Only offsets + code bytes
// are load-bearing; field names carry the recovered roles.
#ifndef GRUNTZ_GRUNTZ_CVIEWPORT_H
#define GRUNTZ_GRUNTZ_CVIEWPORT_H

#include <rva.h>

struct CViewport {
    void WrapCoord(i32* px, i32* py); // reloc-masked
    char m_pad00[0x20];
    i32* m_cells;   // +0x20  tile cell array (cell = m_cells[m_rowBase[y] + x])
    i32* m_rowBase; // +0x24  per-row start-index table
    char m_pad28[0x30 - 0x28];
    i32 m_worldWidth;  // +0x30  world width (also the grid clamp width)
    i32 m_worldHeight; // +0x34  world height (grid clamp height)
    char m_pad38[0x5c - 0x38];
    char* m_5c; // +0x5c  visible-rect base pointer (visible-bounds RECT at +0x40)
    char m_pad60[0x8c - 0x60];
    i32 m_shiftX; // +0x8c  world->tile X shift (tx = x >> m_shiftX)
    i32 m_shiftY; // +0x90  world->tile Y shift
};

#endif // GRUNTZ_GRUNTZ_CVIEWPORT_H
