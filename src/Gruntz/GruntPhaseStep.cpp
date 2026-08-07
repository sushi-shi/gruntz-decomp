#include <Enums.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/GameRand.h>
#include <Mfc.h>
#include <MfcNoInline.h>
#include <Gruntz/GruntSpawnConfig.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/GruntzMapMgr.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/TileCollisionKind.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/GruntAiState.h>
#include <Gruntz/GruntPuddle.h>
#include <Gruntz/GameLevel.h>
#include <Wap32/ZVec.h>
#include <Ints.h>
#include <string.h>
#include <stdlib.h>
#include <Gruntz/FreeNodePool.h>
#include <MfcWin.h>
#include <new>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/CoordNode.h>
#include <Gruntz/EnemyAiType.h>
#include <rva.h>
#include <Gruntz/GruntDirStatics.h>

#pragma intrinsic(strcmp)

#include <Gruntz/FreeNodePool.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/TriggerMgrRecords.h>
#include <Wap32/TileGeometry.h>
#include <Gruntz/StaminaPct.h>
#include <Gruntz/ScanGridMacros.h>
#include <limits.h>

// @early-stop

// @early-stop
// retail reads BOTH components of each GetScreenPos result and shifts
// them in place; the frame is also 16 bytes smaller than ours.
RVA(0x000f60f0, 0xb30)
i32 CGrunt::PhaseStep() {
    Coord pa;
    Coord pb;

    m_neighborScanEnabled = 0;
    if (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_actKey), "F") == 0) {
        return 1;
    }
    m_defenderPx.m_x = m_lastTilePx.m_x;
    m_defenderPx.m_y = m_lastTilePx.m_y;

    if (m_defenderState == AISTATE_PHASE_MIRROR_THEN_COOLDOWN) {
        GetScreenPos(&pa);
        i32 ax = pa.m_x >> TILE_SHIFT_PX;
        GetScreenPos(&pb);
        i32 gx = (pb.m_x >> TILE_SHIFT_PX) - m_arrivalCell.m_x + ax;
        GetScreenPos(&pa);
        i32 ay = pa.m_y >> TILE_SHIFT_PX;
        GetScreenPos(&pb);
        i32 gy = (pb.m_y >> TILE_SHIFT_PX) - m_arrivalCell.m_y + ay;
        TileSwitch(gx, gy, 0, m_arrivalFlags, 1, 0);
        m_dwell = 0;
        m_defenderState = AISTATE_COOLDOWN;
    }
    if (m_defenderState == AISTATE_PHASE_MIRROR_THEN_SEEK) {
        GetScreenPos(&pa);
        i32 ax = pa.m_x >> TILE_SHIFT_PX;
        GetScreenPos(&pb);
        GetScreenPos(&pa);
        i32 gx = (pb.m_x >> TILE_SHIFT_PX) - m_arrivalCell.m_x + ax;
        i32 ay = pa.m_x >> TILE_SHIFT_PX;
        GetScreenPos(&pb);
        i32 gy = (pb.m_y >> TILE_SHIFT_PX) - m_arrivalCell.m_y + ay;
        TileSwitch(gx, gy, 0, m_arrivalFlags, 1, 0);
        m_defenderState = AISTATE_SEEK;
        return 1;
    }

    if (m_defenderState == AISTATE_SEEK) {
        goto state0;
    }
    if (m_defenderState == AISTATE_ATTACK) {
        goto state2;
    }
    if (m_defenderState != AISTATE_COOLDOWN) {
        goto common;
    }
    if (m_dwell <= DWELL_COOLDOWN_MS) {
        return 1;
    }
    m_defenderState = AISTATE_SEEK;
    return 1;

state2: {
    if (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_actKey), "F") == 0) {
        goto common;
    }
    i32 x = m_arrivalCell.m_x;
    i32 y = m_arrivalCell.m_y;
    CMapMgr* grid = g_gameReg->m_tileGrid;
    {
        RECT box;
        box.left = x - 4;
        box.top = y - 4;
        box.right = x + 5;
        box.bottom = y + 5;
        RECT gb;
        gb.left = 0;
        gb.top = 0;
        gb.right = grid->m_width;
        gb.bottom = grid->m_height;
        if (!IntersectRect(&grid->m_bounds, &box, &gb)) {
            grid->m_bounds = box;
        }
        grid->m_gridW = grid->m_bounds.right - grid->m_bounds.left;
        grid->m_gridH = grid->m_bounds.bottom - grid->m_bounds.top;
    }
    CDWordArray acc;
    acc.SetAtGrow(acc.GetSize(), ((x - 2) << 16) | ((y - 2) & 0xffff));
    acc.SetAtGrow(acc.GetSize(), ((x - 1) << 16) | ((y - 2) & 0xffff));
    acc.SetAtGrow(acc.GetSize(), (x << 16) | ((y - 2) & 0xffff));
    acc.SetAtGrow(acc.GetSize(), ((x + 1) << 16) | ((y - 2) & 0xffff));
    acc.SetAtGrow(acc.GetSize(), ((x + 2) << 16) | ((y - 2) & 0xffff));
    acc.SetAtGrow(acc.GetSize(), ((x - 2) << 16) | ((y + 2) & 0xffff));
    acc.SetAtGrow(acc.GetSize(), ((x - 1) << 16) | ((y + 2) & 0xffff));
    acc.SetAtGrow(acc.GetSize(), (x << 16) | ((y + 2) & 0xffff));
    acc.SetAtGrow(acc.GetSize(), ((x + 1) << 16) | ((y + 2) & 0xffff));
    acc.SetAtGrow(acc.GetSize(), ((x + 2) << 16) | ((y + 2) & 0xffff));
    acc.SetAtGrow(acc.GetSize(), ((x - 2) << 16) | ((y - 1) & 0xffff));
    acc.SetAtGrow(acc.GetSize(), ((x - 2) << 16) | (y & 0xffff));
    acc.SetAtGrow(acc.GetSize(), ((x - 2) << 16) | ((y + 1) & 0xffff));
    acc.SetAtGrow(acc.GetSize(), ((x + 2) << 16) | ((y - 1) & 0xffff));
    acc.SetAtGrow(acc.GetSize(), ((x + 2) << 16) | (y & 0xffff));
    acc.SetAtGrow(acc.GetSize(), ((x + 2) << 16) | ((y + 1) & 0xffff));
    while (acc.GetSize() != 0) {
        i32 sel = rand() % acc.GetSize();
        i32 pt = acc.GetAt(sel);
        i32 px = static_cast<u32>(pt) >> 0x10;
        i32 py = pt & 0xffff;
        CMapMgr* pl = g_gameReg->m_tileGrid;
        i32 flag;
        if (static_cast<u32>(px) < static_cast<u32>(pl->m_width)
            && static_cast<u32>(py) < static_cast<u32>(pl->m_height) && px < pl->m_width
            && py < pl->m_height) {
            flag = pl->m_rows[py][px].m_flags;
        } else {
            flag = 1;
        }
        if ((flag & BRICKZ_BLOCKED_MASK) == 0) {
            if (TileSwitch(px, py, 0, m_arrivalFlags, 1, 0) != 0) {
                m_defenderState = AISTATE_COOLDOWN;
                m_dwell = 0;
                goto build_tail;
            }
        }
        acc.RemoveAt(sel, 1);
    }
build_tail: {
    CMapMgr* pl2 = g_gameReg->m_tileGrid;
    GRID_BOUNDS(pl2);
    goto common;
}
}

state0: {
    CGrunt* nb = m_tileMgr->FindNearestEnemy(this);
    if (nb == NULL) {
        goto common;
    }
    if (nb->m_entranceCommitted == 0) {
        goto common;
    }
    if (m_poweredUp == 0 && m_stamina >= STAMINA_FULL
        && nb->m_object->m_screenX == nb->m_lastTilePx.m_x
        && nb->m_object->m_screenY == nb->m_lastTilePx.m_y
        && RectContains(nb->m_object->m_screenX, nb->m_object->m_screenY) != 0) {
        CommitNeighbor(
            nb->m_tileOwnerHi,
            nb->m_tileOwnerLo,
            nb->m_lastTilePx.m_x,
            nb->m_lastTilePx.m_y
        );
        m_arrivalCell.m_x = nb->m_object->m_screenX >> TILE_SHIFT_PX;
        m_arrivalCell.m_y = nb->m_object->m_screenY >> TILE_SHIFT_PX;
        m_defenderState = AISTATE_ATTACK;
        goto common;
    }
    if (m_dwell <= DWELL_REPATH_MS) {
        goto common;
    }
    if (GruntInRadius(nb->m_tileOwnerHi, nb->m_tileOwnerLo) == 0) {
        goto s0_reset;
    }
    if (TileSwitch(
            nb->m_object->m_screenX >> TILE_SHIFT_PX,
            nb->m_object->m_screenY >> TILE_SHIFT_PX,
            0,
            m_arrivalFlags,
            1,
            0
        )
        == 0) {
        m_passableMask |= 0x4020;
        TileSwitch(
            nb->m_object->m_screenX >> TILE_SHIFT_PX,
            nb->m_object->m_screenY >> TILE_SHIFT_PX,
            0,
            m_arrivalFlags,
            1,
            0
        );
        m_passableMask &= 0xffffbfdf;
    }
    m_dwell = 0;
    if (m_blockedVoicePending == 0) {
        goto common;
    }
    if (CGameLevel::PointInBounds(
            &g_gameReg->m_world->m_level->m_mainPlane->m_viewRect,
            m_object->m_screenX,
            m_object->m_screenY
        )
        == 0) {
        goto s0_reset;
    }
    g_gameReg->m_cueSink->SpawnVoiceDriver(this, 0x366, -1, 0, -1, -1);
s0_reset:
    m_blockedVoicePending = 0;
    goto common;
}

common: {
    GruntAiState st = m_defenderState;
    if (st != AISTATE_COOLDOWN && st != AISTATE_PHASE_MIRROR_THEN_COOLDOWN && CoordCount() >= 2) {
        CoordNode* head = CoordHead();
        i32 bx = head->m_coord->m_x;
        i32 by = head->m_coord->m_y;
        Coord* nc = head->m_next->m_coord;
        i32 fx = nc->m_x;
        i32 fy = nc->m_y;
        CMapMgr* pl = g_gameReg->m_tileGrid;
        i32 flag;
        if (static_cast<u32>(fx) < static_cast<u32>(pl->m_width)
            && static_cast<u32>(fy) < static_cast<u32>(pl->m_height)) {
            flag = pl->m_rows[fy][fx].m_flags;
        } else {
            flag = 1;
        }
        if ((flag & 0x20) != 0) {
            if (CoordCount() != 0) {
                RECYCLE_COORDS(CoordHead());
                m_coordList.RemoveAll();
            }
            g_gameReg->m_cmdGrid
                ->ApplyTriggerA(m_tileOwnerHi, m_tileOwnerLo, bx * 32 + 16, by * 32 + 16);
            m_arrivalCell.m_x = bx;
            m_arrivalCell.m_y = by;
            m_defenderState = AISTATE_PHASE_MIRROR_THEN_COOLDOWN;
            return 1;
        }
    }
    if (CoordCount() == 0) {
        return 1;
    }
    Coord* head = CoordHead()->m_coord;
    CMapMgr* pl2 = g_gameReg->m_tileGrid;
    i32 gx = head->m_x;
    i32 gy = head->m_y;
    i32 flag2;
    if (static_cast<u32>(gx) < static_cast<u32>(pl2->m_width)
        && static_cast<u32>(gy) < static_cast<u32>(pl2->m_height)) {
        flag2 = pl2->m_rows[gy][gx].m_flags;
    } else {
        flag2 = 1;
    }
    if ((flag2 & 0x20) == 0) {
        return 1;
    }
    m_arrivalCell.m_x = gx;
    m_arrivalCell.m_y = gy;
    if (CoordCount() != 0) {
        RECYCLE_COORDS(CoordHead());
        m_coordList.RemoveAll();
    }
    m_defenderState = AISTATE_PHASE_MIRROR_THEN_SEEK;
    return 1;
}
}
