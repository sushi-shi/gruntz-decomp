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

// CMapMgr::CellFlagsAt: the bounds-checked cell-flag read cl inlines at every
// tile test (out-of-bounds reads back as flag bit 0, which every BRICKZ mask
// treats as blocked). Declared in MapMgr.h; defined here because the body needs
// BrickzCell complete.
// @early-stop
// Instruction stream and block skeleton both match 1:1; the frame is 0x64 where
// retail's is 0x44 because retail's cl overlaid the mirror Coords, the clip
// scratch RECTs and the common-block spills onto one another and ours gives each
// its own granule, so every [esp+N] operand is displaced.
RVA(0x000f60f0, 0xb30)
i32 CGrunt::PhaseStep() {
    m_neighborScanEnabled = 0;
    bool isFlag = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_actKey), s_codeF) == 0);
    if (isFlag) {
        return 1;
    }
    m_defenderPx.m_x = m_lastTilePx.m_x;
    m_defenderPx.m_y = m_lastTilePx.m_y;

    // Mirror the grunt across m_arrivalCell: the destination tile is
    // 2*here - arrival on each axis, and each axis re-reads the live screen
    // position (retail shifts BOTH components of the first read in place and
    // seeds the other axis of the second Coord before overwriting it).
    if (m_defenderState == AISTATE_PHASE_MIRROR_THEN_COOLDOWN) {
        Coord pa;
        Coord pb;
        GetScreenTile(&pa);
        pb.m_y = pa.m_y;
        GetScreenPos(&pb);
        i32 gx = (pb.m_x >> TILE_SHIFT_PX) - m_arrivalCell.m_x + pa.m_x;
        GetScreenTile(&pa);
        pb.m_x = pa.m_x;
        GetScreenPos(&pb);
        i32 gy = (pb.m_y >> TILE_SHIFT_PX) - m_arrivalCell.m_y + pa.m_y;
        TileSwitch(gx, gy, 0, m_arrivalFlags, 1, 0);
        m_dwell = 0;
        m_defenderState = AISTATE_COOLDOWN;
    }
    if (m_defenderState == AISTATE_PHASE_MIRROR_THEN_SEEK) {
        Coord pa;
        Coord pb;
        GetScreenTile(&pa);
        pb.m_y = pa.m_y;
        GetScreenPos(&pb);
        i32 gx = (pb.m_x >> TILE_SHIFT_PX) - m_arrivalCell.m_x + pa.m_x;
        GetScreenTile(&pa);
        pb.m_x = pa.m_x;
        GetScreenPos(&pb);
        i32 gy = (pb.m_y >> TILE_SHIFT_PX) - m_arrivalCell.m_y + pa.m_y;
        TileSwitch(gx, gy, 0, m_arrivalFlags, 1, 0);
        m_defenderState = AISTATE_SEEK;
        return 1;
    }

    switch (m_defenderState) {
        case AISTATE_SEEK:
            goto state0;
        case AISTATE_ATTACK:
            goto state2;
        case AISTATE_COOLDOWN:
            if (m_dwell <= static_cast<u32>(DWELL_COOLDOWN_MS)) {
                return 1;
            }
            m_defenderState = AISTATE_SEEK;
            return 1;
    }
    goto common;

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
        CWwdGameObjectA* hit = nb->m_object;
        m_arrivalCell.m_x = hit->m_screenX >> TILE_SHIFT_PX;
        m_arrivalCell.m_y = hit->m_screenY >> TILE_SHIFT_PX;
        m_defenderState = AISTATE_ATTACK;
        goto common;
    }
    if (m_dwell <= static_cast<u32>(DWELL_REPATH_MS)) {
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

state2: {
    bool isFlagObj = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_actKey), s_codeF) == 0);
    if (isFlagObj) {
        goto common;
    }
    CMapMgr* grid = g_gameReg->m_tileGrid;
    RECT box;
    box.left = m_arrivalCell.m_x - 4;
    box.top = m_arrivalCell.m_y - 4;
    box.right = m_arrivalCell.m_x + 5;
    box.bottom = m_arrivalCell.m_y + 5;
    GRID_CLIP_INL(grid, &box);

    // The 16 cells on the 5x5 ring around m_arrivalCell, packed x:y into
    // one DWORD; the loop picks one at random until a phase target takes.
    CDWordArray acc;
    acc.SetAtGrow(acc.GetSize(), ((m_arrivalCell.m_x - 2) << 16) | (m_arrivalCell.m_y - 2));
    acc.SetAtGrow(acc.GetSize(), ((m_arrivalCell.m_x - 1) << 16) | (m_arrivalCell.m_y - 2));
    acc.SetAtGrow(acc.GetSize(), (m_arrivalCell.m_x << 16) | (m_arrivalCell.m_y - 2));
    acc.SetAtGrow(acc.GetSize(), ((m_arrivalCell.m_x + 1) << 16) | (m_arrivalCell.m_y - 2));
    acc.SetAtGrow(acc.GetSize(), ((m_arrivalCell.m_x + 2) << 16) | (m_arrivalCell.m_y - 2));
    acc.SetAtGrow(acc.GetSize(), ((m_arrivalCell.m_x - 2) << 16) | (m_arrivalCell.m_y + 2));
    acc.SetAtGrow(acc.GetSize(), ((m_arrivalCell.m_x - 1) << 16) | (m_arrivalCell.m_y + 2));
    acc.SetAtGrow(acc.GetSize(), (m_arrivalCell.m_x << 16) | (m_arrivalCell.m_y + 2));
    acc.SetAtGrow(acc.GetSize(), ((m_arrivalCell.m_x + 1) << 16) | (m_arrivalCell.m_y + 2));
    acc.SetAtGrow(acc.GetSize(), ((m_arrivalCell.m_x + 2) << 16) | (m_arrivalCell.m_y + 2));
    acc.SetAtGrow(acc.GetSize(), ((m_arrivalCell.m_x - 2) << 16) | (m_arrivalCell.m_y - 1));
    acc.SetAtGrow(acc.GetSize(), ((m_arrivalCell.m_x - 2) << 16) | m_arrivalCell.m_y);
    acc.SetAtGrow(acc.GetSize(), ((m_arrivalCell.m_x - 2) << 16) | (m_arrivalCell.m_y + 1));
    acc.SetAtGrow(acc.GetSize(), ((m_arrivalCell.m_x + 2) << 16) | (m_arrivalCell.m_y - 1));
    acc.SetAtGrow(acc.GetSize(), ((m_arrivalCell.m_x + 2) << 16) | m_arrivalCell.m_y);
    acc.SetAtGrow(acc.GetSize(), ((m_arrivalCell.m_x + 2) << 16) | (m_arrivalCell.m_y + 1));
    while (acc.GetSize() != 0) {
        i32 sel = rand() % acc.GetSize();
        i32 pt = acc.GetAt(sel);
        i32 px = static_cast<u32>(pt) >> 0x10;
        i32 py = pt & 0xffff;
        CMapMgr* pl = g_gameReg->m_tileGrid;
        if (static_cast<u32>(px) < g_gameReg->m_tileGrid->m_width
            && static_cast<u32>(py) < g_gameReg->m_tileGrid->m_height) {
            i32 flag = pl->CellFlagsAt(px, py);
            if ((flag & BRICKZ_BLOCKED_MASK) == 0) {
                if (TileSwitch(px, py, 0, m_arrivalFlags, 1, 0) != 0) {
                    m_defenderState = AISTATE_COOLDOWN;
                    m_dwell = 0;
                    CMapMgr* hit = g_gameReg->m_tileGrid;
                    GRID_CLIP_INL(hit, NULL);
                    return 1;
                }
            }
        }
        acc.RemoveAt(sel, 1);
    }
    CMapMgr* spent = g_gameReg->m_tileGrid;
    GRID_CLIP_INL(spent, NULL);
    m_defenderState = AISTATE_SEEK;
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
        if ((g_gameReg->m_tileGrid->CellFlagsAt(fx, fy) & 0x20) != 0) {
            if (CoordCount() != 0) {
                RECYCLE_COORDS(CoordHead());
                m_coordList.RemoveAll();
            }
            g_gameReg->m_cmdGrid->ApplyTriggerA(
                m_tileOwnerHi,
                m_tileOwnerLo,
                (bx << TILE_SHIFT_PX) + TILE_HALF_PX,
                (by << TILE_SHIFT_PX) + TILE_HALF_PX
            );
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
    if ((g_gameReg->m_tileGrid->CellFlagsAt(head->m_x, head->m_y) & 0x20) == 0) {
        return 1;
    }
    m_arrivalCell.m_x = head->m_x;
    m_arrivalCell.m_y = head->m_y;
    if (CoordCount() != 0) {
        RECYCLE_COORDS(CoordHead());
        m_coordList.RemoveAll();
    }
    m_defenderState = AISTATE_PHASE_MIRROR_THEN_SEEK;
    return 1;
}
}
