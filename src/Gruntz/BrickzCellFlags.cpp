#include <StdAfx.h>

#include <rva.h>

#include <Globals.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/BrickzNeighborMacros.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntMovementInline.h>
#include <Gruntz/GruntNeighborhoodInline.h>
#include <Gruntz/LevelCollisionInline.h>
#include <Gruntz/MapCellFlags.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/TriggerMgr.h>
#include <Lith/BDefs.h>
#include <Wap32/CoordUnset.h>
#include <Wap32/TileGeometry.h>

#include <limits.h>

// @early-stop
RVA(0x00077790, 0x4f0)
void CMapMgr::ComputeCellFlags(i32 x, i32 y, i32 tileId) {

    BrickzCell* cell = &m_rows[y][x];
    TileCollisionKind typeCode = LookupTileOriginCollisionDirect(m_attrMgr->GetLevel(), x, y);
    cell->SetTileAttributes(tileId, typeCode);

    for (i32 c = x - 1; c <= x + 1; c++) {
        for (i32 r = y - 1; r <= y + 1; r++) {
            if (r < 0 || static_cast<u32>(r) >= m_height) {
                continue;
            }
            if (c <= 0 || static_cast<u32>(c) >= m_width) {
                continue;
            }
            BrickzCell* nc = &m_rows[r][c];
            i32 nf = nc->m_flags & ~0x1000;
            nc->m_flags = nf;
            if ((nf & 0x100) == 0) {
                continue;
            }
            DECLARE_BRICKZ_NEIGHBORS(nc, c, r)
            if (BRICKZ_OPPOSITE_NEIGHBORS_OPEN) {
                nc->m_flags = nf | 0x1000;
            }
        }
    }
}

RVA(0x00077dc0, 0x1d)
void CLevelPlane::SetCell(i32 x, i32 y, i32 id) {
    SET_LEVEL_PLANE_CELL(this, x, y, id);
}

RVA(0x00077df0, 0x13d)
CGrunt* CTriggerMgr::FindNearestEnemy(CGrunt* w) {
    CGrunt* best = NULL;
    i32 bestDist = INT_MAX;
    Coord lastTilePx = w->LastTilePx();
    i32 tileX = lastTilePx.m_x >> TILE_SHIFT_PX;
    i32 tileY = lastTilePx.m_y >> TILE_SHIFT_PX;
    i32 i = 0;
    CGrunt** rowPtr = m_units;
    for (; i < PLAYER_SLOT_COUNT; i++, rowPtr += TM_UNITS_PER_PLAYER) {
        if (i != w->GetPlayerIndex()) {
            CGrunt** colPtr = rowPtr;
            i32 j = TM_UNITS_PER_PLAYER;
            do {
                CGrunt* cell = *colPtr;
                if (cell && cell->IsEntranceCommitted() != false
                    && cell->GetPowerupType() != GRUNT_GHOST) {
                    i32 dx = (cell->GetSpriteObject()->m_screenX >> TILE_SHIFT_PX) - tileX;
                    i32 dy = (cell->GetSpriteObject()->m_screenY >> TILE_SHIFT_PX) - tileY;
                    i32 dist = SquaredDistance(dx, dy);
                    if (dist < bestDist) {
                        best = cell;
                        bestDist = dist;
                    }
                }
                colPtr++;
            } while (--j != 0);
        }
    }
    RECT rc = AttackTileNeighborhood(w);
    if (best) {
        Coord bestPos = ScreenPosition(best->GetSpriteObject());
        POINT pt;
        pt.x = bestPos.m_x >> TILE_SHIFT_PX;
        pt.y = bestPos.m_y >> TILE_SHIFT_PX;
        if (!PtInRect(&rc, pt)) {
            best = NULL;
        }
    }
    return best;
}
