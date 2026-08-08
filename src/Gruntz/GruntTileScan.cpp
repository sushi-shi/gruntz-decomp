#include <rva.h>

#include <Mfc.h>
#include <MfcNoInline.h>
#include <MfcWin.h>

#include <Gruntz/BattlezMapConfig.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/FreeNodePool.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/MapMgr.h>
#include <Gruntz/StaminaPct.h>
#include <Gruntz/TileCollisionKind.h>
#include <Ints.h>
#include <Wap32/TileGeometry.h>

#include <stdlib.h>

static inline i32 ScanCellX(CGrunt* g) {
    Coord t;
    g->GetScreenPos(&t);
    t.m_x >>= TILE_SHIFT_PX;
    t.m_y >>= TILE_SHIFT_PX;
    return t.m_x;
}

static inline i32 ScanCellY(CGrunt* g) {
    Coord t;
    g->GetScreenPos(&t);
    t.m_x >>= TILE_SHIFT_PX;
    t.m_y >>= TILE_SHIFT_PX;
    return t.m_y;
}

// @early-stop
// The two signed/unsigned twins are fixed (`hits` is unsigned - retail spells the
// range guards `ja`/`jae`); what is left is register/spill colouring.
RVA(0x00032ce0, 0x448)
i32 CBattlezMapConfig::ScanRegion(CGrunt* g) {
    if (g->m_stamina >= STAMINA_FULL) {
        if (g->CoordCount() != 0) {
            Coord* c = static_cast<Coord*>(g->m_coordList.GetTail());
            i32 col = c->m_x;
            i32 row = c->m_y;
            CMapMgr* grid = m_board;
            i32 flags;
            if (static_cast<u32>(col) < static_cast<u32>(grid->m_width)
                && static_cast<u32>(row) < static_cast<u32>(grid->m_height)) {
                flags = grid->m_rows[row][col].m_flags;
            } else {
                flags = 1;
            }
            if ((flags & 0x4000)
                && grid->m_rows[row][col].m_typeCode == TILEKIND_GAUNTLET_BRICK_C) {
                POSITION pos = g->m_coordList.GetHeadPosition();
                while (pos != NULL) {
                    void* coord = g->m_coordList.GetNext(pos);
                    if (coord != NULL) {
                        g_coordPool.Push(coord);
                    }
                }
                g->m_coordList.RemoveAll();
                return 1;
            }
        }
        if (g->m_dwell > static_cast<u32>(m_nearbyRouteSearchDelay) && g->CoordCount() == 0) {
            CMapMgr* grid = m_board;
            RECT box;
            box.left = ScanCellX(g) - 5;
            box.top = ScanCellY(g) - 5;
            box.right = ScanCellX(g) + 5;
            box.bottom = (g->m_object->m_screenY >> TILE_SHIFT_PX) + 5;
            RECT gb;
            gb.left = 0;
            gb.top = 0;
            gb.right = m_board->m_width;
            gb.bottom = m_board->m_height;
            RECT isect;
            if (IntersectRect(&isect, &box, &gb)) {
                u32 hits = 0;
                for (i32 row = isect.top; row < isect.bottom; row++) {
                    if (hits > 4) {
                        break;
                    }
                    BrickzCell* cell = &grid->m_rows[row][isect.left];
                    for (i32 col = isect.left; col < isect.right; col++) {
                        if (hits < 5) {
                            i32 flags = cell->m_flags;
                            if (flags & 0x8000) {
                                if (RouteUnitTo(g, col, row, 0xd87, 0, 0)) {
                                    RECT hitClip;
                                    hitClip.left = 0;
                                    hitClip.top = 0;
                                    hitClip.right = grid->m_width;
                                    hitClip.bottom = grid->m_height;
                                    RECT hitFull = CRect(0, 0, grid->m_width, grid->m_height);
                                    RECT* hitDst = &grid->m_bounds;
                                    if (!IntersectRect(hitDst, &hitFull, &hitClip)) {
                                        *hitDst = hitFull;
                                    }
                                    grid->m_gridW = hitDst->right - hitDst->left;
                                    grid->m_gridH = hitDst->bottom - hitDst->top;
                                    return 1;
                                }
                                hits++;
                            } else if ((flags & 0x4000)
                                       && cell->m_typeCode != TILEKIND_GAUNTLET_BRICK_C) {
                                if (RouteUnitTo(g, col, row, 0xd87, 0, 0)) {
                                    RECT brickClip;
                                    brickClip.left = 0;
                                    brickClip.top = 0;
                                    brickClip.right = grid->m_width;
                                    brickClip.bottom = grid->m_height;
                                    RECT brickFull = CRect(0, 0, grid->m_width, grid->m_height);
                                    RECT* brickDst = &grid->m_bounds;
                                    if (!IntersectRect(brickDst, &brickFull, &brickClip)) {
                                        *brickDst = brickFull;
                                    }
                                    grid->m_gridW = brickDst->right - brickDst->left;
                                    grid->m_gridH = brickDst->bottom - brickDst->top;
                                    return 1;
                                }
                                hits++;
                            }
                        }
                        cell++;
                    }
                }
            }
            {
                CRect tailClip(0, 0, grid->m_width, grid->m_height);
                RECT tailFull = CRect(0, 0, grid->m_width, grid->m_height);
                RECT* tailDst = &grid->m_bounds;
                if (!IntersectRect(tailDst, &tailFull, &tailClip)) {
                    *tailDst = tailFull;
                }
                grid->m_gridW = tailDst->right - tailDst->left;
                grid->m_gridH = tailDst->bottom - tailDst->top;
            }
            if (m_attackWaypoints.GetSize() != 0) {

                Coord* e = CoordAt(rand() % m_attackWaypoints.GetSize());
                g->TileSwitch(e->m_x, e->m_y, 0, 0x983, 0, 0);
            }
            g->m_dwell = 0;
        }
    }
    return 1;
}
