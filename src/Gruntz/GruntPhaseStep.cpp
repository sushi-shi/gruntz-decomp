#include <rva.h>

#include <Mfc.h>
#include <MfcNoInline.h>
#include <MfcWin.h>

#include <Enums.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/CoordNode.h>
#include <Gruntz/EnemyAiType.h>
#include <Gruntz/FreeNodePool.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameRand.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/MapCellFlags.h>
#include <Gruntz/GruntAiState.h>
#include <Gruntz/GruntCoordRecycleMacros.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntMovementInline.h>
#include <Gruntz/GruntMovementMacros.h>
#include <Gruntz/GruntPuddle.h>
#include <Gruntz/GruntzMapMgr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/ScanGridMacros.h>
#include <Gruntz/StaminaPct.h>
#include <Gruntz/TileCollisionKind.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TriggerMgrRecords.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/VoiceManager.h>
#include <Ints.h>
#include <MakeRect.h>
#include <Wap32/TileGeometry.h>
#include <Wap32/ZVec.h>

#include <limits.h>
#include <new>
#include <stdlib.h>
#include <string.h>

// @early-stop
RVA(0x000f60f0, 0xb30)
i32 CGrunt::StepTimeBomberBehavior() {
    m_neighborScanEnabled = false;
    bool isFlag = ANIMATION_ACT_EQUALS("F");
    if (isFlag) {
        return 1;
    }
    m_defenderPx = LastTilePx();

    if (m_defenderState == AISTATE_PHASE_MIRROR_THEN_COOLDOWN) {
        MirrorGruntAcrossArrival(this);
        m_dwell = 0;
        m_defenderState = AISTATE_COOLDOWN;
    }
    if (m_defenderState == AISTATE_PHASE_MIRROR_THEN_SEEK) {
        MirrorGruntAcrossArrival(this);
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

state2: {
    bool isFlagObj = ANIMATION_ACT_EQUALS("F");
    if (isFlagObj) {
        goto common;
    }
    {
        RECT box = MakeRect(
            m_arrivalCell.m_x - 4,
            m_arrivalCell.m_y - 4,
            m_arrivalCell.m_x + 5,
            m_arrivalCell.m_y + 5
        );
        CMapMgr* grid = g_gameReg->m_tileGrid;
        grid->Clip(&box);
    }

    CDWordArray acc;
    acc.Add(((m_arrivalCell.m_x - 2) << 16) | (m_arrivalCell.m_y - 2));
    acc.Add(((m_arrivalCell.m_x - 1) << 16) | (m_arrivalCell.m_y - 2));
    acc.Add((m_arrivalCell.m_x << 16) | (m_arrivalCell.m_y - 2));
    acc.Add(((m_arrivalCell.m_x + 1) << 16) | (m_arrivalCell.m_y - 2));
    acc.Add(((m_arrivalCell.m_x + 2) << 16) | (m_arrivalCell.m_y - 2));
    acc.Add(((m_arrivalCell.m_x - 2) << 16) | (m_arrivalCell.m_y + 2));
    acc.Add(((m_arrivalCell.m_x - 1) << 16) | (m_arrivalCell.m_y + 2));
    acc.Add((m_arrivalCell.m_x << 16) | (m_arrivalCell.m_y + 2));
    acc.Add(((m_arrivalCell.m_x + 1) << 16) | (m_arrivalCell.m_y + 2));
    acc.Add(((m_arrivalCell.m_x + 2) << 16) | (m_arrivalCell.m_y + 2));
    acc.Add(((m_arrivalCell.m_x - 2) << 16) | (m_arrivalCell.m_y - 1));
    acc.Add(((m_arrivalCell.m_x - 2) << 16) | m_arrivalCell.m_y);
    acc.Add(((m_arrivalCell.m_x - 2) << 16) | (m_arrivalCell.m_y + 1));
    acc.Add(((m_arrivalCell.m_x + 2) << 16) | (m_arrivalCell.m_y - 1));
    acc.Add(((m_arrivalCell.m_x + 2) << 16) | m_arrivalCell.m_y);
    acc.Add(((m_arrivalCell.m_x + 2) << 16) | (m_arrivalCell.m_y + 1));
    while (acc.GetSize() != 0) {
        i32 sel = rand() % acc.GetSize();
        DWORD pt = acc.GetAt(sel);
        Coord tile(HIWORD(pt), LOWORD(pt));
        CMapMgr* pl = g_gameReg->m_tileGrid;
        if (static_cast<u32>(tile.m_x) < g_gameReg->m_tileGrid->m_width
            && static_cast<u32>(tile.m_y) < g_gameReg->m_tileGrid->m_height) {
            i32 flag = pl->CellFlagsAt(tile.m_x, tile.m_y);
            if ((flag & BRICKZ_BLOCKED_MASK) == 0) {
                if (TileSwitch(tile.m_x, tile.m_y, 0, m_arrivalFlags, 1, 0) != 0) {
                    m_defenderState = AISTATE_COOLDOWN;
                    m_dwell = 0;
                    CMapMgr* hit = g_gameReg->m_tileGrid;
                    hit->Clip(NULL);
                    return 1;
                }
            }
        }
        acc.RemoveAt(sel, 1);
    }
    CMapMgr* spent = g_gameReg->m_tileGrid;
    spent->Clip(NULL);
    m_defenderState = AISTATE_SEEK;
    goto common;
}

state0: {
    CGruntzMgr* game;
    CGrunt* nb = m_triggerMgr->FindNearestEnemy(this);
    if (nb == NULL) {
        goto common;
    }
    if (nb->m_entranceCommitted == false) {
        goto common;
    }
    if (m_poweredUp == false && m_stamina >= STAMINA_FULL && GRUNT_AT_SAVED_SCREEN_POS(nb)
        && RectContains(nb->m_object->m_screenPosition.m_x, nb->m_object->m_screenPosition.m_y)
               != 0) {
        COMMIT_GRUNT_NEIGHBOR(nb);
        CWwdSpriteObject* hit = nb->m_object;
        m_arrivalCell = hit->ScreenPos();
        ScreenTile(&m_arrivalCell);
        m_defenderState = AISTATE_ATTACK;
        goto common;
    }
    if (m_dwell <= static_cast<u32>(DWELL_REPATH_MS)) {
        goto common;
    }
    if (GruntInRadius(nb->m_playerIndex, nb->m_unitIndex) == 0) {
        goto s0_reset;
    }
    {
        Coord targetTile;
        nb->GetScreenTile(&targetTile);
        if (TileSwitch(targetTile.m_x, targetTile.m_y, 0, m_arrivalFlags, 1, 0) == 0) {
            m_passableMask |= IDX(CELL_FLAG_DESTRUCTIBLE_ROCK | CELL_FLAG_GAUNTLET_BRICK);
            TileSwitch(targetTile.m_x, targetTile.m_y, 0, m_arrivalFlags, 1, 0);
            m_passableMask &= ~IDX(CELL_FLAG_DESTRUCTIBLE_ROCK | CELL_FLAG_GAUNTLET_BRICK);
        }
    }
    m_dwell = 0;
    if (m_blockedVoicePending == false) {
        goto common;
    }
    game = g_gameReg;
    if (CGameLevel::PointInBounds(
            &game->m_world->m_level->m_mainPlane->m_planeViewRect,
            m_object->m_screenPosition.m_x,
            m_object->m_screenPosition.m_y
        )
        == 0) {
        goto s0_reset;
    }
    game->m_voiceManager->PlayVoice(this, 0x366, -1, 0, -1, -1);
s0_reset:
    m_blockedVoicePending = false;
    goto common;
}

common: {
    GruntAiState st = m_defenderState;
    if (st != AISTATE_COOLDOWN && st != AISTATE_PHASE_MIRROR_THEN_COOLDOWN && CoordCount() >= 2) {
        CoordNode* head = CoordHead();
        Coord targetTile = *head->m_coord;
        Coord nextTile = *head->m_next->m_coord;
        if ((g_gameReg->m_tileGrid->CellFlagsAt(nextTile.m_x, nextTile.m_y)
             & IDX(CELL_FLAG_DESTRUCTIBLE_ROCK))
            != 0) {
            if (CoordCount() != 0) {
                RECYCLE_GRUNT_COORDS_EXPANDED(this)
            }
            Coord targetPosition = targetTile;
            TileCenter(&targetPosition);
            g_gameReg->m_triggerMgr->UseEquippedToolAt(
                m_playerIndex,
                m_unitIndex,
                targetPosition.m_x,
                targetPosition.m_y
            );
            m_arrivalCell = targetTile;
            m_defenderState = AISTATE_PHASE_MIRROR_THEN_COOLDOWN;
            return 1;
        }
    }
    if (CoordCount() == 0) {
        return 1;
    }
    Coord* head = CoordHead()->m_coord;
    if ((g_gameReg->m_tileGrid->CellFlagsAt(head->m_x, head->m_y)
         & IDX(CELL_FLAG_DESTRUCTIBLE_ROCK))
        == 0) {
        return 1;
    }
    m_arrivalCell = *head;
    if (CoordCount() != 0) {
        RECYCLE_GRUNT_COORDS_EXPANDED(this)
    }
    m_defenderState = AISTATE_PHASE_MIRROR_THEN_SEEK;
    return 1;
}
}
