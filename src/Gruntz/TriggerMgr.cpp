#include <Gruntz/TriggerMgr.h>

#include <Bute/ButeMgr.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/PixelShift.h>
#include <Dsndmgr/SoundBuffer.h>
#include <Enums.h>
#include <Gruntz/ActionOptionsMenuBar.h>
#include <Gruntz/BattlezMapConfig.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/CombatCueKind.h>
#include <Gruntz/EnemyAiType.h>
#include <Gruntz/ErrorStringId.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameModeId.h>
#include <Gruntz/GameObjectLogicTypes.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntCombatClockInline.h>
#include <Gruntz/GruntDeathType.h>
#include <Gruntz/GruntPickupInline.h>
#include <Gruntz/GruntPuddle.h>
#include <Gruntz/GruntzCmdMgr.h>
#include <Gruntz/GruntzCommandId.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/HealthPct.h>
#include <Gruntz/LightFx.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/Play.h>
#include <Gruntz/PlayerCommandKind.h>
#include <Gruntz/SbiMenuItemState.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialRecords.h>
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/SoundCue.h>
#include <Gruntz/SoundCueRegistry.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/SpriteStateFlags.h>
#include <Gruntz/StatusBarDock.h>
#include <Gruntz/StatusBarMgr.h>
#include <Gruntz/StatusBarTab.h>
#include <Gruntz/String.h>
#include <Gruntz/TileActionEvent.h>
#include <Gruntz/TileGrid.h>
#include <Gruntz/TileTriggerContainer.h>
#include <Gruntz/TileTriggerLogic.h>
#include <Gruntz/TileTriggerSwitchLogic.h>
#include <Gruntz/TriggerMgrRecords.h>
#include <Gruntz/UserLogic.h>
#include <Gruntz/VoiceManager.h>
#include <Gruntz/Warlord.h>
#include <Io/FileMem.h>
#include <Utils/MapTyped.h>
#include <Wap32/CoordUnset.h>
#include <Wap32/TileGeometry.h>
#include <Wwd/WwdFile.h>

#include <limits.h>
#include <stdlib.h>

DATA(0x00244ca4)
i32 g_groupSentinel;

// @early-stop
// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00077f80, 0xab)
CGrunt* CTriggerMgr::FindNearestUnitForPlayer(CGrunt* g) {
    i32 tx = g->m_lastTilePx.m_x >> TILE_SHIFT_PX;
    i32 playerIndex = g->m_playerIndex;
    CGrunt** units = &m_units[playerIndex * TM_UNITS_PER_PLAYER];
    i32 ty = g->m_lastTilePx.m_y >> TILE_SHIFT_PX;
    CGrunt* best = NULL;
    i32 bestDist = INT_MAX;
    i32 unitsRemaining = TM_UNITS_PER_PLAYER;
    do {
        CGrunt* candidate = *units;
        if (candidate != NULL) {
            CGameObject* o = candidate->m_object;
            i32 dx = (o->m_screenX >> TILE_SHIFT_PX) - tx;
            i32 dy = (o->m_screenY >> TILE_SHIFT_PX) - ty;
            i32 d = dx * dx + dy * dy;
            if (d < bestDist && d < g->m_defenderRadius * 2) {
                best = candidate;
                bestDist = d;
            }
        }
        units++;
        unitsRemaining--;
    } while (unitsRemaining != 0);
    return best;
}

// @early-stop
RVA(0x00078060, 0x18d)
void CTriggerMgr::HudRect(RECT r, i32 flag) {
    CGameLevel* view = m_world->m_level;
    const RECT* vp = &view->m_mainPlane->m_viewRect;
    r.left += vp->left - view->m_planeCtx.left;
    r.top += vp->top - view->m_planeCtx.top;
    vp = &view->m_mainPlane->m_viewRect;
    r.right += vp->left - view->m_planeCtx.left;
    r.bottom += vp->top - view->m_planeCtx.top;
    // Retail walks ONE pointer across the whole 4x15 grid: `lea eax,[ebp+0x1c]`
    // (&m_units[0]) outside, `mov ebx,eax` at the outer head, `add ebx,4` per inner
    // step and `mov eax,ebx` at the outer tail - so the row base carries forward and
    // the cell is m_units[i * TM_UNITS_PER_PLAYER + j], not m_units[j].
    for (i32 i = 0; i < TM_PLAYER_COUNT; i++) {
        for (i32 j = 0; j < TM_UNITS_PER_PLAYER; j++) {
            CGrunt* g = m_units[i * TM_UNITS_PER_PLAYER + j];
            if (g) {
                CGameObject* pos = g->m_object;
                i32 cx = pos->m_screenX;
                i32 cy = pos->m_screenY;
                RECT box;
                SetRect(&box, cx - 0xf, cy - 0xf, cx + 0xf, cy + 0xf);
                if (r.left <= box.right && r.right >= box.left && r.top <= box.bottom
                    && r.bottom >= box.top) {
                    if (i == g_curPlayer) {
                        if (flag == 0 && g->m_entranceCommitted != 0) {
                            ResetAll();
                            flag = 1;
                        }
                        ResetCell(g_curPlayer, j, 1, 1);
                    } else {
                        g->CreateHealthSprite();
                        g->m_hudRetireWindowLo =
                            g_buteMgr.GetDwordDef("Grunt", "CombatTimeout", 0x1388);
                        g->m_hudRetireWindowHi = 0;
                        g->m_hudRetireClockLo = g_frameTime;
                        g->m_hudRetireClockHi = 0;
                    }
                }
            }
        }
    }
}

// @early-stop
RVA(0x00078260, 0x165)
i32 CTriggerMgr::RemoveCellRecord(i32 playerIndex, i32 unitIndex, i32 fromSelection) {
    if (fromSelection != 0) {
        CPtrList* list = m_selLists;
        i32 k = 10;
        do {
            POSITION pos = list->GetHeadPosition();
            while (pos != NULL) {
                POSITION cur = pos;
                Coord* p = static_cast<Coord*>(list->GetNext(pos));
                if (p->m_x == playerIndex && p->m_y == unitIndex) {
                    CoordPoolNode* slot = g_coordPool.NodeOf(p);
                    slot->m_next = g_coordPool.m_freeHead;
                    g_coordPool.m_freeHead = slot;
                    list->RemoveAt(cur);
                }
            }
            list++;
            k--;
        } while (k != 0);
    }
    POSITION pos = m_recList.GetHeadPosition();
    while (pos != NULL) {
        POSITION cur = pos;
        Coord* p = static_cast<Coord*>(m_recList.GetNext(pos));
        if (p->m_x == playerIndex && p->m_y == unitIndex) {
            if (m_recList.GetCount() == 1) {
                StopPendingFx();
            }
            CGrunt* cell = m_units[unitIndex + playerIndex * TM_UNITS_PER_PLAYER];
            if (cell != NULL) {
                (static_cast<CGrunt*>(cell))->ClearAllSprites();
            }
            i32 removedPlayerIndex = p->m_x;
            i32 removedUnitIndex = p->m_y;
            if (removedPlayerIndex == m_cameraTargetIdentity.m_x
                && removedUnitIndex == m_cameraTargetIdentity.m_y) {
                CWwdGameObjectA* goal = m_goal;
                if (goal != NULL) {
                    goal->m_flags |= 0x10000;
                    m_goal = NULL;
                }
                m_armed = 0;
            }
            CActionOptionsMenuBar* ov = m_overlay;
            if (ov != NULL) {
                i32 overlayPlayerIndex = ov->m_playerIndex;
                i32 overlayUnitIndex = ov->m_unitIndex;
                if (overlayPlayerIndex == p->m_x && overlayUnitIndex == p->m_y) {
                    OverlayTick();
                }
            }
            CoordPoolNode* slot = g_coordPool.NodeOf(p);
            slot->m_next = g_coordPool.m_freeHead;
            g_coordPool.m_freeHead = slot;
            m_recList.RemoveAt(cur);
            return 1;
        }
    }
    return 0;
}

RVA(0x00078430, 0x7f)
void CTriggerMgr::ResetAll() {
    POSITION pos = m_recList.GetHeadPosition();
    while (pos != NULL) {
        Coord* payload = static_cast<Coord*>(m_recList.GetNext(pos));
        i32 idx = payload->m_y + TM_UNITS_PER_PLAYER * payload->m_x;
        CGrunt* cell = m_units[idx];
        if (cell != NULL) {
            (static_cast<CGrunt*>(cell))->ClearAllSprites();
            CoordPoolNode* slot = g_coordPool.NodeOf(payload);
            slot->m_next = g_coordPool.m_freeHead;
            g_coordPool.m_freeHead = slot;
        }
    }
    m_recList.RemoveAll();
    StopPendingFx();
    CWwdGameObjectA* goal = m_goal;
    if (goal != NULL) {
        goal->m_flags |= 0x10000;
        m_goal = NULL;
    }
}

RVA(0x000784d0, 0x3a)
i32 CTriggerMgr::RecordListHas(i32 playerIndex, i32 unitIndex) {
    POSITION pos = m_recList.GetHeadPosition();
    while (pos != NULL) {
        Coord* p = static_cast<Coord*>(m_recList.GetNext(pos));
        if (p->m_x == playerIndex && p->m_y == unitIndex) {
            return 1;
        }
    }
    return 0;
}

RVA(0x00078520, 0x106)
void CTriggerMgr::ReportRecordsA(i32 tag, i32 gx, i32 gy) {
    if (m_groupFlag == 0) {
        return;
    }
    u8 count = 0;
    u8 firstByte; // retail leaves it uninitialized - only the loop writes it
    u8 bytes[0x80];
    POSITION pos = m_recList.GetHeadPosition();
    while (pos != NULL) {
        Coord* payload = static_cast<Coord*>(m_recList.GetNext(pos));
        CGrunt* cell = m_units[payload->m_y + payload->m_x * TM_UNITS_PER_PLAYER];
        firstByte = static_cast<u8>(payload->m_x);
        if (cell->m_playerIndex == g_curPlayer && cell->m_entranceActive == 0) {
            bytes[count] = static_cast<u8>(payload->m_y);
            count++;
        }
    }
    CGruntzCmdMgr* rep = g_gameReg->m_commandMgr;
    if (count == 1) {
        g_gameReg->m_commandMgr->EnqueueSingle(
            tag,
            firstByte,
            bytes[0],
            static_cast<char>(IDX(PLAYERCMD_MOVE)),
            static_cast<i16>(gx),
            static_cast<i16>(gy),
            0,
            0
        );
    } else {
        g_gameReg->m_commandMgr->EnqueueMulti(
            tag,
            firstByte,
            count,
            bytes,
            static_cast<char>(IDX(PLAYERCMD_MOVE)),
            static_cast<i16>(gx),
            static_cast<i16>(gy),
            0
        );
    }
}

RVA(0x00078680, 0x189)
void CTriggerMgr::ReportRecordsB(i32 tag, i32 gx, i32 gy, i32 flag) {
    if (m_groupFlag == 0) {
        return;
    }
    u8 count = 0;
    u8 firstByte; // retail leaves it uninitialized - only the loop writes it
    u8 bytes[0x80];
    POSITION pos = m_recList.GetHeadPosition();
    while (pos != NULL) {
        Coord* payload = static_cast<Coord*>(m_recList.GetNext(pos));
        CGrunt* cell = m_units[payload->m_y + payload->m_x * TM_UNITS_PER_PLAYER];
        firstByte = static_cast<u8>(payload->m_x);
        if (cell->m_playerIndex == g_curPlayer && cell->m_entranceActive == 0) {
            bytes[count] = static_cast<u8>(payload->m_y);
            count++;
        }
    }
    CGruntzCmdMgr* rep = g_gameReg->m_commandMgr;
    if (count == 1) {
        if (flag != 0) {
            g_gameReg->m_commandMgr->EnqueueSingle(
                tag,
                firstByte,
                bytes[0],
                static_cast<char>(IDX(PLAYERCMD_USE_TOOL_ON_GRUNT)),
                static_cast<i16>(gx),
                static_cast<i16>(gy),
                0,
                0
            );
        } else {
            g_gameReg->m_commandMgr->EnqueueSingle(
                tag,
                firstByte,
                bytes[0],
                static_cast<char>(IDX(PLAYERCMD_USE_TOOL_AT_POINT)),
                static_cast<i16>(gx),
                static_cast<i16>(gy),
                0,
                0
            );
        }
    } else {
        if (flag != 0) {
            g_gameReg->m_commandMgr->EnqueueMulti(
                tag,
                firstByte,
                count,
                bytes,
                static_cast<char>(IDX(PLAYERCMD_USE_TOOL_ON_GRUNT)),
                static_cast<i16>(gx),
                static_cast<i16>(gy),
                0
            );
        } else {
            g_gameReg->m_commandMgr->EnqueueMulti(
                tag,
                firstByte,
                count,
                bytes,
                static_cast<char>(IDX(PLAYERCMD_USE_TOOL_AT_POINT)),
                static_cast<i16>(gx),
                static_cast<i16>(gy),
                0
            );
        }
    }
}

RVA(0x00078880, 0x3c)
void CTriggerMgr::ClearRecords() {
    POSITION pos = m_recList.GetHeadPosition();
    if (pos != NULL) {
        do {
            CoordPoolNode* slot = g_coordPool.NodeOf(m_recList.GetNext(pos));
            slot->m_next = g_coordPool.m_freeHead;
            g_coordPool.m_freeHead = slot;
        } while (pos != NULL);
    }
    m_recList.RemoveAll();
}

RVA(0x000788d0, 0x64)
i32 CTriggerMgr::ScrollToActiveRecord() {
    CGameObject* src =
        m_units[m_cameraTargetIdentity.m_x * TM_UNITS_PER_PLAYER + m_cameraTargetIdentity.m_y]
            ->m_object;
    i32 y = src->m_screenY;
    i32 x = src->m_screenX;
    CDDrawWorkerHost* t = m_world->m_level->m_mainPlane;
    SET_SCROLL_POSITION_RAW_FIRST(t, x, y);
    return 1;
}

RVA(0x00078960, 0x9b)
i32 CTriggerMgr::LoadCameraSprite() {
    if (m_goal != NULL) {
        return 0;
    }

    i32 vx = g_gameReg->m_modeSize.cx;
    i32 vy = g_gameReg->m_modeSize.cy;
    StatusBarDock pos = (static_cast<CPlay*>(g_gameReg->m_curState))->m_statusBar->m_position;

    i32 ax, cx;
    if (pos != STATUSBAR_DOCK_RIGHT) {
        if (pos > STATUSBAR_NONRIGHT_BEFORE_FIRST && pos <= STATUSBAR_NONRIGHT_LAST) {
            ax = vx - 0x28;
            cx = vy - 0x28;
        }
    } else {
        ax = vx - 0xc8;
        cx = vy - 0x28;
    }

    CDDrawChildGroup* fac = m_world->m_childGroup;
    CWwdGameObjectA* spr = fac->CreateSprite(0, ax, cx, SORTKEY_OVERLAY, "DoNothing", 1);
    m_goal = spr;
    spr->m_logicRecord->m_dispatch(spr);
    m_goal->ApplyName("GAME_CAMERASPRITE");
    return 1;
}

RVA(0x00078a30, 0x10)
void CTriggerMgr::OverlayTick() {
    CActionOptionsMenuBar* ov = m_overlay;
    if (ov) {
        ov->Deactivate();
    }
}

// The 16-bit path-preview colour: retail packs all THREE channels through the
// runtime shift globals even when green/blue are zero (cl5 does not fold
// `0 >> var`, so the zero channels are visible as xor/sar/shl).
static inline SoundCue* LookupCue(CMapStringToPtr& cues, LPCTSTR name) {
    SoundCue* found = NULL;
    MapLookup(cues, name, found);
    return found;
}

static inline u16 PackRgb16(i32 r, i32 g, i32 b) {
    return static_cast<u16>(((r >> g_rDown) << g_rUp) | ((g >> g_gDown) << g_gUp) | (b >> g_bDown));
}

// @early-stop
// One duplicated call: cl emits the vehicle preview's two LoadCursorSprites in
// full where retail shares one (both arms `push` the argument and jump to a
// common `mov ebp,<world>; mov ecx,ebp; call`). Ours differs by one instruction
// - the true arm materialises world in EBP BEFORE its call while the false arm
// loads the receiver straight into ECX and reloads EBP after - so the tail
// matcher correctly declines. Not budget: `inline-model --gap` reports
// LoadCursorSprites UNDEFINED in this obj, so /Ob1 excludes it at any budget.
// Open, measured lead: retail reads m_pendingFxKind FIVE times (+0x2a8 at
// 0x78a93, 0x78b09, the hitFlag preview, the final tail, the vehicle preview)
// and this body reads it four - the hitFlag preview's else arm should read the
// member, not `pfk`. Spelling it costs 8 points on its own (90.84 -> 82.54):
// with both previews reading the member, cl cross-jumps three more arms into
// shared tails, giving 12 LoadCursorSprites and 13 rets against retail's 14 and
// 16. Needs a terminator lever at each of those three arms to compose.
RVA(0x00078a50, 0x8a0)
i32 CTriggerMgr::PlaceObjectFull(i32 x, i32 y) {

    CGrunt* cell;
    if (m_recList.GetCount() != 1) {
        cell = NULL;
    } else {
        Coord* rec = static_cast<Coord*>(m_recList.GetHead());
        cell = m_units[rec->m_y + rec->m_x * TM_UNITS_PER_PLAYER];
    }
    if (cell == NULL || cell->m_playerIndex != g_curPlayer) {
        return 1;
    }

    CActionOptionsMenuBar* ov = m_overlay;
    if (ov != NULL && ov->m_active != 0) {
        ov->HitClick(x, y);
        return 1;
    }
    CPlay* world = static_cast<CPlay*>(g_gameReg->m_curState);
    if (m_pendingFxKind == 0) {

        if ((static_cast<CGrunt*>(cell))->CanShowStamina() == 0) {
            world->LoadCursorSprites(0, 0);
            return 1;
        }
    }

    i32 hitFlag = 0;
    if (CellHitTest(x, y, NULL, NULL, TM_ALL_PLAYERS)) {
        hitFlag = 1;
    }

    // No function-lifetime `view` local: retail re-derives m_world->m_level per
    // region (the level dies in ecx here) and the full chain at each preview
    // site - a spanning local claims a callee-saved register and cascades ty,
    // pfk and `alt` into memory homes across the whole switch.
    CGameLevel* level = m_world->m_level;
    i32 tx = x >> TILE_SHIFT_PX;
    i32 ty = y >> TILE_SHIFT_PX;
    i32 cx = tx;
    if (tx < 0) {
        cx = 0;
    } else if (tx >= level->m_mainPlane->m_gridW) {
        cx = level->m_mainPlane->m_gridW - 1;
    }
    i32 cy = ty;
    if (ty < 0) {
        cy = 0;
    } else if (ty >= level->m_mainPlane->m_gridH) {
        cy = level->m_mainPlane->m_gridH - 1;
    }
    TileCollisionKind collision;
    i32 cval = level->m_mainPlane->m_tileGrid[level->m_mainPlane->m_rowOffsets[cy] + cx];
    if (cval != UNINIT_FILL && cval != -1) {
        CTileImageSet* tc = static_cast<CTileImageSet*>(level->m_imageSets.GetAt(cval & 0xffff));
        // Ingest: the raw WWD attribute byte for this cell.
        collision = tc->GetCollisionAt(0, 0);
    } else {
        collision = TILEKIND_PASSABLE;
    }

    i32 pfk = m_pendingFxKind;
    if (pfk >= 0xdf) {
        PickupType alt = cell->m_vehiclePickupType;
        if (hitFlag != 0) {
            world->LoadCursorSprites(IDX(alt) + kPendingFxIdBase, 1);
        } else {
            CGruntzMapMgr* plane = g_gameReg->m_tileGrid;
            i32 attr = plane->CellFlagsAt(tx, ty);
            if ((attr & BRICKZ_BLOCKED_MASK) != 0 || (attr & 2) != 0) {
                world->LoadCursorSprites(pfk, 0);
            } else {
                world->LoadCursorSprites(IDX(alt) + kPendingFxIdBase, 1);
            }
        }
        return 1;
    }

    PickupType gruntKind = ARRIVAL_PICKUP_TERNARY_GT(cell);

    if (hitFlag != 0) {
        if (pfk == 0) {
            world->LoadCursorSprites(0, 0);
            return 1;
        }
        {
            // Retail's compare order is rock, welder, boomerang, gunhat,
            // nerfgun, wingz; the positive spelling is byte-identical here.
            if (gruntKind != GRUNT_ROCK && gruntKind != GRUNT_WELDER && gruntKind != GRUNT_BOOMERANG
                && gruntKind != GRUNT_GUNHAT && gruntKind != GRUNT_NERFGUN
                && gruntKind != GRUNT_WINGZ) {
                world->LoadCursorSprites(IDX(gruntKind) + kPendingFxIdBase, 1);
                return 1;
            }

            POINT source = {cell->m_object->m_screenX, cell->m_object->m_screenY};
            m_world->m_level->m_mainPlane->WrapCoord(&source.x, &source.y);
            // Retail-proven ABI seam: WrapCoord receives the two by-value i32
            // argument slots directly; Win32 LONG has the same 32-bit storage.
            m_world->m_level->m_mainPlane->WrapCoord(
                reinterpret_cast<LONG*>(&x), // PROVEN: i32/LONG argument-slot alias.
                reinterpret_cast<LONG*>(&y)  // PROVEN: i32/LONG argument-slot alias.
            );
            u16 color;
            if (cell->RectContains(x, y)) {
                color = PackRgb16(0xff, 0, 0);
                world->LoadCursorSprites(IDX(gruntKind) + kPendingFxIdBase, 1);
            } else {
                color = PackRgb16(0x20, 0x20, 0x20);
                world->LoadCursorSprites(pfk, 0);
            }
            world->m_pathPreviewSource = source;
            world->m_pathPreviewDestination.x = x;
            world->m_pathPreviewDestination.y = y;
            world->m_pathPreviewColor = color;
            world->m_drewThisFrame = 1;
            return 1;
        }
    } else {
        switch (gruntKind) {
            case PICKUP_GAUNTLETZ:
                if (collision == TILEKIND_GAUNTLET_ROCK_A || collision == TILEKIND_GAUNTLET_ROCK_B
                    || collision == TILEKIND_GIANT_ROCK || collision == TILEKIND_GAUNTLET_BRICK_A
                    || collision == TILEKIND_GAUNTLET_BRICK_B
                    || collision == TILEKIND_GAUNTLET_BRICK_C) {
                    world->LoadCursorSprites(IDX(gruntKind) + kPendingFxIdBase, 1);
                    return 1;
                }
                break;

            case PICKUP_SHOVEL:
                if (collision == TILEKIND_COVERED_POWERUP
                    || collision == TILEKIND_REVEALED_POWERUP) {
                    world->LoadCursorSprites(IDX(gruntKind) + kPendingFxIdBase, 1);
                    return 1;
                }
                break;

            case PICKUP_GOOBER: {
                POSITION pos = m_baseList.GetHeadPosition();
                while (pos != NULL) {
                    CGruntPuddle* cand = static_cast<CGruntPuddle*>(m_baseList.GetNext(pos));
                    if (cand->m_tileX == tx && cand->m_tileY == ty && cand->m_pending == 0) {
                        world->LoadCursorSprites(IDX(gruntKind) + kPendingFxIdBase, 1);
                        return 1;
                    }
                }
                break;
            }
            case PICKUP_BRICK:
                if (collision == TILEKIND_HIDDEN_POWERUP || collision == TILEKIND_GAUNTLET_BRICK_A
                    || collision == TILEKIND_GAUNTLET_BRICK_B) {
                    world->LoadCursorSprites(IDX(gruntKind) + kPendingFxIdBase, 1);
                    return 1;
                }
                break;

            case PICKUP_BOMB:
                if (pfk != 0) {
                    world->LoadCursorSprites(IDX(gruntKind) + kPendingFxIdBase, 1);
                } else {
                    world->LoadCursorSprites(0, 0);
                }
                return 1;

            case PICKUP_WARPSTONE:
                if (g_gameReg->m_gameMode != GAMEMODE_QUESTZ) {
                    if (pfk != 0) {
                        world->LoadCursorSprites(IDX(gruntKind) + kPendingFxIdBase, 1);
                    } else {
                        world->LoadCursorSprites(0, 0);
                    }
                    return 1;
                }
                break;
            case PICKUP_SPRING:
                if (pfk != 0) {
                    world->LoadCursorSprites(IDX(gruntKind) + kPendingFxIdBase, 1);
                } else {
                    world->LoadCursorSprites(0, 0);
                }
                return 1;

            case PICKUP_SPY: {
                if (pfk != 0 || collision == TILEKIND_GAUNTLET_ROCK_A
                    || collision == TILEKIND_GAUNTLET_ROCK_B || collision == TILEKIND_GIANT_ROCK
                    || collision == TILEKIND_GAUNTLET_BRICK_A
                    || collision == TILEKIND_GAUNTLET_BRICK_B
                    || collision == TILEKIND_GAUNTLET_BRICK_C) {
                    world->LoadCursorSprites(IDX(gruntKind) + kPendingFxIdBase, 1);
                    return 1;
                }
                // the spy also lights up over a hidden object parked on this cell
                CGruntzMapMgr* plane = g_gameReg->m_tileGrid;
                i32 occupantId;
                if (static_cast<u32>(tx) >= static_cast<u32>(plane->m_width)
                    || static_cast<u32>(ty) >= static_cast<u32>(plane->m_height)) {
                    occupantId = 0;
                } else {
                    occupantId = plane->m_rowInts[ty][tx * 7 + 2];
                }
                if (occupantId != 0) {
                    CMapPtrToPtr* map =
                        &g_gameReg->m_world->m_childGroup->m_registeredGameObjectsById;
                    CGameObject* occupant = NULL;
                    MapLookupById(*map, occupantId, occupant);
                    if (occupant != NULL) {
                        CUserLogic* logic = occupant->m_logicRecord->m_userLogic;
                        if (logic != NULL && logic->m_object->m_smarts == IDX(PICKUP_TOYBOX)) {
                            world->LoadCursorSprites(IDX(gruntKind) + kPendingFxIdBase, 1);
                            return 1;
                        }
                    }
                }
                break;
            }

            case PICKUP_BOOMERANG:
            case PICKUP_GUNHAT:
            case PICKUP_NERFGUN:
            case PICKUP_ROCK:
            case PICKUP_WELDER:
            case PICKUP_WINGZ:
                if (pfk != 0) {
                    POINT source = {cell->m_object->m_screenX, cell->m_object->m_screenY};
                    m_world->m_level->m_mainPlane->WrapCoord(&source.x, &source.y);
                    // The destination wrap is OPEN-CODED on the scalars (retail
                    // 0x79126-0x791b0: dx/dy never leave edi/ebx, the plane chain
                    // is re-derived, and the final adjust is the re-associated
                    // bounds50-viewRect difference) - the same wrap the source
                    // POINT goes through by call above.
                    CDDrawWorkerHost* plane = m_world->m_level->m_mainPlane;
                    i32 dx = x;
                    i32 dy = y;
                    i32 wflags = plane->m_flags;
                    if (wflags & 0x4) {
                        i32 w = plane->m_wrapW;
                        if (dx < 0) {
                            dx = dx + w;
                        } else if (dx >= w) {
                            dx = dx - w;
                        }
                        if (plane->m_viewRect.right >= w && dx < plane->m_viewRect.left
                            && dx <= plane->m_viewRect.right - w) {
                            dx = dx + w;
                        }
                    }
                    if (wflags & 0x8) {
                        i32 h = plane->m_wrapH;
                        if (dy < 0) {
                            dy = dy + h;
                        } else if (dy >= h) {
                            dy = dy - h;
                        }
                        if (plane->m_viewRect.bottom >= h && dy < plane->m_viewRect.top
                            && dy <= plane->m_viewRect.bottom - h) {
                            dy = dy + h;
                        }
                    }
                    dx += plane->m_bounds50.left - plane->m_viewRect.left;
                    dy += plane->m_bounds50.top - plane->m_viewRect.top;
                    u16 color;
                    if (cell->RectContains(x, y)) {
                        color = PackRgb16(0xff, 0, 0);
                        world->LoadCursorSprites(IDX(gruntKind) + kPendingFxIdBase, 1);
                    } else {
                        color = PackRgb16(0x20, 0x20, 0x20);
                        world->LoadCursorSprites(m_pendingFxKind, 0);
                    }
                    world->m_pathPreviewSource = source;
                    world->m_pathPreviewDestination.x = dx;
                    world->m_pathPreviewDestination.y = dy;
                    world->m_pathPreviewColor = color;
                    world->m_drewThisFrame = 1;
                    return 1;
                }
                break;

            case PICKUP_TIMEBOMB: {
                if (pfk == 0) {
                    break;
                }
                CGruntzMapMgr* plane = g_gameReg->m_tileGrid;
                i32 attr = plane->CellFlagsAt(tx, ty);
                if ((attr & BRICKZ_BLOCKED_MASK) == 0 && (attr & 2) == 0) {
                    world->LoadCursorSprites(IDX(gruntKind) + kPendingFxIdBase, 1);
                    return 1;
                }
                break;
            }
        }
    }

    world->LoadCursorSprites(m_pendingFxKind, 0);
    return 1;
}

// @early-stop
// One block differs (B25, the TARGET_SELECTION_TOY arm): retail cross-jumps its
// `Activate(...,3,1)` tail into the block the other two arms share; cl duplicates
// it. NOT a merge-policy difference - cl hoisted the m_logicRecord reload above the
// pushes in that ONE arm (`mov eax,[esi+0x7c]` / `mov ecx,[eax+0x18]`), so its tail
// is not identical to the others and the suffix matcher correctly declines. The
// other 45 blocks are byte-identical.
RVA(0x00079520, 0x2e3)
i32 CTriggerMgr::ResetGroup(
    i32 x,
    i32 y,
    i32 worldX,
    i32 worldY,
    i32 unused5,
    TargetSelectionKind selector,
    i32 spawnCursor
) {
    static_cast<void>(unused5);
    if (m_groupFlag == 0) {
        return 0;
    }
    CGrunt* hit = CellHitTest(x, y, NULL, NULL, TM_ALL_PLAYERS);
    CGrunt* cell;
    if (m_recList.GetCount() != 1) {
        cell = NULL;
    } else {
        Coord* rec = static_cast<Coord*>(m_recList.GetHead());
        cell = m_units[rec->m_x * TM_UNITS_PER_PLAYER + rec->m_y];
    }

    TargetSelectionKind sel;
    if (cell != NULL) {
        if (cell->m_playerIndex != g_curPlayer) {
            return 1;
        }
        if (selector != TARGET_SELECTION_AUTO) {
            sel = selector;
        } else if (hit != NULL) {
            if (hit == cell) {
                m_pendingFxKind = 0;
                (static_cast<CPlay*>(g_gameReg->m_curState))->LoadCursorSprites(0, 0);
                CGameObject* o = hit->m_object;

                this->DestroyGroup(o->m_screenX, o->m_screenY, worldX, worldY);
                return 1;
            }
            sel = TARGET_SELECTION_GRUNT;
        } else {
            sel = TARGET_SELECTION_POINT;
        }
    } else {
        sel = (hit != NULL) ? TARGET_SELECTION_GRUNT : TARGET_SELECTION_POINT;
    }

    CGameObject* sprite;
    switch (sel) {
        case TARGET_SELECTION_POINT:
            this->ReportRecordsA(1, x, y);
            if (spawnCursor == 0) {
                return 1;
            }
            sprite =
                m_world->m_childGroup->CreateSprite(0, x, y, SORTKEY_OVERLAY, "LightFx", 0x40003);
            sprite->m_logicRecord->m_dispatch(sprite);
            (static_cast<CLightFx*>(sprite->m_logicRecord->m_userLogic))
                ->Activate("GAME_LIGHTING_TARGETCURSOR", "GAME_TARGETCURSOR", 2, 1);
            return 1;
        case TARGET_SELECTION_GRUNT:
            if (hit != NULL) {
                i32 hitPlayerIndex = hit->m_playerIndex;
                if (hitPlayerIndex == g_curPlayer && g_traitorMode == 0) {
                    if (cell != hit) {
                        goto reportError;
                    }
                    PickupType v = ARRIVAL_PICKUP_TERNARY_LE(hit);
                    if (v != PICKUP_SPY) {
                        PickupType v2 = ARRIVAL_PICKUP_TERNARY_LE(hit);
                        if (v2 != PICKUP_WAND) {
                            goto reportError;
                        }
                    }
                }
                this->ReportRecordsB(1, hitPlayerIndex, hit->m_unitIndex, 1);
            } else {
                this->ReportRecordsB(1, x, y, 0);
            }
            if (spawnCursor == 0) {
                return 1;
            }
            sprite =
                m_world->m_childGroup->CreateSprite(0, x, y, SORTKEY_OVERLAY, "LightFx", 0x40003);
            sprite->m_logicRecord->m_dispatch(sprite);
            (static_cast<CLightFx*>(sprite->m_logicRecord->m_userLogic))
                ->Activate("GAME_LIGHTING_TARGETCURSOR", "GAME_TARGETCURSOR", 1, 1);
            return 1;
        case TARGET_SELECTION_TOY:
            if (hit != NULL) {
                if (hit->m_playerIndex == g_curPlayer && g_traitorMode == 0
                    && (cell != hit || hit->m_vehiclePickupType != PICKUP_SCROLL)) {
                    goto reportError;
                }
                i32 hitPlayerIndex = hit->m_playerIndex;
                i32 hitUnitIndex = hit->m_unitIndex;
                i32 cellUnitIndex = cell->m_unitIndex;
                i32 cellPlayerIndex = cell->m_playerIndex;
                g_gameReg->m_commandMgr->EnqueueSingle(
                    1,
                    cellPlayerIndex,
                    cellUnitIndex,
                    static_cast<char>(IDX(PLAYERCMD_USE_TOY_ON_GRUNT)),
                    hitPlayerIndex,
                    hitUnitIndex,
                    0,
                    0
                );
            } else {
                i32 cellUnitIndex = cell->m_unitIndex;
                i32 cellPlayerIndex = cell->m_playerIndex;
                g_gameReg->m_commandMgr->EnqueueSingle(
                    1,
                    cellPlayerIndex,
                    cellUnitIndex,
                    static_cast<char>(IDX(PLAYERCMD_USE_TOY_AT_POINT)),
                    x,
                    y,
                    0,
                    0
                );
            }
            if (spawnCursor == 0) {
                return 1;
            }
            sprite =
                m_world->m_childGroup->CreateSprite(0, x, y, SORTKEY_OVERLAY, "LightFx", 0x40003);
            sprite->m_logicRecord->m_dispatch(sprite);
            (static_cast<CLightFx*>(sprite->m_logicRecord->m_userLogic))
                ->Activate("GAME_LIGHTING_TARGETCURSOR", "GAME_TARGETCURSOR", 3, 1);
            return 1;
        default:
            return 1;
    }

reportError:
    g_gameReg->m_voiceManager->PlayVoice(cell, 0x324, -1, 0, -1, -1);
    return 0;
}

RVA(0x000798d0, 0x1b6)
i32 CTriggerMgr::DestroyGroup(i32 screenX, i32 screenY, i32 worldX, i32 worldY) {
    if (m_overlay == NULL) {
        m_overlay = new CActionOptionsMenuBar;
        if (m_overlay->LoadAssets() == 0) {
            CActionOptionsMenuBar* o2 = m_overlay;
            if (o2 != NULL) {
                o2->Clear();
                operator delete(o2);
                m_overlay = NULL;
            }
            g_gameReg->ReportError(IDX(IDS_INITIALIZE_GAME), 0x3ff);
            return 0;
        }
    }
    if (m_overlay->m_active != 0) {
        return 0;
    }
    CGrunt* cellp;
    if (m_recList.GetCount() != 1) {
        cellp = NULL;
    } else {
        Coord* rec = static_cast<Coord*>(m_recList.GetHead());
        cellp = m_units[rec->m_y + rec->m_x * TM_UNITS_PER_PLAYER];
    }
    if (cellp == NULL) {
        return 0;
    }
    if (cellp->m_playerIndex != g_curPlayer) {
        return 0;
    }
    if (m_overlay->Init(
            ACTIONOPTION_HIDDEN,
            ACTIONOPTION_HIDDEN,
            screenX,
            screenY,
            cellp->m_playerIndex,
            cellp->m_unitIndex
        )
        == ACTIONOPTION_HIDDEN) {
        return 0;
    }
    CGameLevel* view = m_world->m_level;
    RECT* vr = &view->m_mainPlane->m_viewRect;
    i32 ox = vr->left - view->m_planeCtx.left + worldX;
    i32 oy = vr->top - view->m_planeCtx.top + worldY;
    this->PlaceObjectFull(ox, oy);
    return 1;
}

RVA(0x00079b00, 0x15)
i32 CTriggerMgr::OverlayRelease() {
    CActionOptionsMenuBar* ov = m_overlay;
    if (ov) {
        return ov->Render();
    }
    return 1;
}

RVA(0x00079b30, 0x3e)
i32 CTriggerMgr::ByteTableHas(WarpStoneFragment fragment) {

    i32 n = m_byteArr.GetSize();
    for (i32 i = 0; i < n; i++) {
        if (IDX(fragment) == m_byteArr[i]) {
            return 1;
        }
    }
    return 0;
}

RVA(0x00079b80, 0x194)
void CTriggerMgr::ReinitGroup(i32 col, i32 row) {
    if (m_groupInitialized != 0 || g_gameReg->m_gameMode != GAMEMODE_QUESTZ) {
        return;
    }
    CPlay* lvl = static_cast<CPlay*>(g_gameReg->m_curState);
    CString name;
    name.Format("Level%i", lvl->m_levelIndex);
    WarpStoneFragment fragment = static_cast<WarpStoneFragment>(
        g_buteMgr.GetInt("WarpStone", const_cast<char*>(static_cast<const char*>(name)))
    );
    if (col >= g_gameReg->m_viewBounds.right || col < g_gameReg->m_viewBounds.left
        || row >= g_gameReg->m_viewBounds.bottom || row < g_gameReg->m_viewBounds.top) {
        lvl->ResetGoals(col, row);
    }

    CGameLevel* plane = g_gameReg->m_world->m_level;
    LONG outR = col;
    LONG outC = row;
    plane->m_mainPlane->WrapCoord(&outR, &outC);
    CStatusBarMgr* sbi = lvl->m_statusBar;
    if (sbi->m_hlBusy == 0) {
        if (sbi->m_position == STATUSBAR_HIDDEN) {
            sbi->RestoreStatusBar();
        }
        if (sbi->m_activeTab != TAB_GAME) {
            sbi->SetTabState(SBICMD_TAB_GAME, MENUITEM_SELECTED);
        }
        sbi->SetTab(GAME_TAB_MENU, 1);
        sbi->Deactivate();
    }
    if (lvl->m_statusBar->StartWarpStoneFly(outR, outC, fragment) != 0) {
        lvl->m_statusBar->m_hlBusy = 1;
    } else {
        m_byteArr.Add(static_cast<u8>(IDX(fragment)));
    }
    m_groupInitialized = 1;
}

RVA(0x00079d90, 0xc5)
void CTriggerMgr::ResetSpawnState() {
    if (g_gameReg->m_gameMode != GAMEMODE_QUESTZ) {
        return;
    }
    if (m_groupInitialized == 0) {
        return;
    }
    CPlay* world = static_cast<CPlay*>(g_gameReg->m_curState);
    CStatusBarMgr* st = world->m_statusBar;
    if (st->m_retabNotify != NULL) {
        operator delete(st->m_retabNotify);
        st->m_retabNotify = NULL;
    }
    world->m_statusBar->m_hlBusy = 0;
    if (m_byteArr.GetSize() > 0) {
        m_byteArr.RemoveAt(m_byteArr.GetSize() - 1, 1);
        CStatusBarMgr* ctx = world->m_statusBar;
        if (ctx->m_position != STATUSBAR_HIDDEN && ctx->m_activeTab == TAB_GAME) {
            ctx->ResetWidgets(0);
            world->m_statusBar->TryActivate();
        }
    }
    if (g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
        CWarlord* fx = m_pendingFx;
        if (fx != NULL) {
            fx->ResolveDeathAnimation();
        }
    }
    this->LoadFinishLevelSprite(FINISH_REASON_WARPSTONE_RESET);
}

RVA(0x00079ea0, 0xc2)
i32 CTriggerMgr::SpawnTileFx(i32 x, i32 y, i32 anchorIndex) {
    if (g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
        return 0;
    }
    CGruntzMapMgr* grid = g_gameReg->m_tileGrid;
    i32 tx = x >> TILE_SHIFT_PX;
    i32 ty = y >> TILE_SHIFT_PX;
    i32 tile = grid->CellFlagsAt(tx, ty);
    if ((tile & 0x40939) == 0 && (tile & 2) == 0) {
        this->LoadPowerupIconSprites(
            PICKUP_WARPSTONE,
            (tx << TILE_SHIFT_PX) + TILE_HALF_PX,
            (ty << TILE_SHIFT_PX) + TILE_HALF_PX,
            0,
            anchorIndex,
            0
        );
    } else {
        CPlay* world = static_cast<CPlay*>(g_gameReg->m_curState);
        i32 idx = anchorIndex - 1;
        CPlay::Anchor* rec = (idx < 0 || idx >= 4) ? NULL : &world->m_anchors[idx];
        if (rec != NULL) {
            this->LoadPowerupIconSprites(PICKUP_WARPSTONE, rec->m_x, rec->m_y, 0, anchorIndex, 0);
        }
    }
    return 1;
}

// @early-stop
RVA(0x00079fb0, 0x169)
void CTriggerMgr::UnregisterUnit(i32 playerIndex, i32 unitIndex, i32 exitedLevel) {
    i32 idx = playerIndex * TM_UNITS_PER_PLAYER + unitIndex;
    CGrunt* cell = m_units[idx];
    if (cell == NULL) {
        return;
    }
    if (cell->m_cellRemovalNotified != 0) {
        return;
    }
    if (cell->m_arrivalPending == 0) {
        this->ApplySwitch(cell, cell->m_lastTilePx.m_x, cell->m_lastTilePx.m_y);
    }
    CGruntzMapMgr* tg = g_gameReg->m_tileGrid;
    i32 rowIdx = cell->LastTilePx().m_y >> TILE_SHIFT_PX;
    i32 cellCol = cell->LastTilePx().m_x >> TILE_SHIFT_PX;
    tg->m_rows[rowIdx][cellCol].m_flags &= BRICKZ_CELL_UNOCCUPIED_MASK;
    tg->m_rows[rowIdx][cellCol].m_occupantId = -1;
    m_units[idx] = NULL;
    m_unitCountByPlayer[playerIndex] -= 1;

    PickupType k;
    if (exitedLevel != 0) {
        m_unitExited[idx] = 1;
        m_gruntzExitedByPlayer[playerIndex] += 1;
        k = cell->m_entranceReason;
        if (k > PICKUP_EQUIPPABLE_LAST) {
            k = cell->m_toolId;
        }
        if (k == PICKUP_WARPSTONE) {
            if (g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
                CWarlord* fx = m_pendingFx;
                if (fx != NULL) {
                    fx->RaiseBattleAlert();
                }
            }
            this->LoadFinishLevelSprite(FINISH_REASON_WARPSTONE_EXIT);
        }
    } else {
        k = cell->m_entranceReason;
        if (k > PICKUP_EQUIPPABLE_LAST) {
            k = cell->m_toolId;
        }
        if (k == PICKUP_WARPSTONE) {
            this->ResetSpawnState();
        }
        m_gruntzLostByPlayer[playerIndex] += 1;
    }
    cell->m_cellRemovalNotified = 1;
}

RVA(0x0007a180, 0x86)
i32 CTriggerMgr::SpawnPuddle(
    i32 x,
    i32 y,
    i32 playerIndex,
    i32 moveIcon,
    i32 animatePlacement,
    i32 gaugePoints
) {
    CDDrawChildGroup* childGroup = m_world->m_childGroup;
    CWwdGameObjectA* sprite = childGroup->CreateSprite(0, x, y, 0xa, "GruntPuddle", 0x40003);
    if (sprite == NULL) {

        g_gameReg->ReportError(IDX(IDS_DEFAULT_ERROR), 0x400);
        return 0;
    }
    sprite->m_logicRecord->m_dispatch(sprite);
    sprite->m_smarts = playerIndex;
    sprite->m_score = moveIcon;
    sprite->m_points = gaugePoints;
    return PlacePuddle(sprite, animatePlacement);
}

RVA(0x0007a240, 0x143)
i32 CTriggerMgr::PlacePuddle(CGameObject* sprite, i32 animatePlacement) {
    CGruntPuddle* puddle = static_cast<CGruntPuddle*>(sprite->m_logicRecord->m_userLogic);
    i32 gaugePoints = sprite->m_points;
    if (gaugePoints == 0) {
        gaugePoints = 0x19;
    }
    if (puddle->Place(sprite->m_smarts, sprite->m_score, animatePlacement, gaugePoints) == 0) {
        puddle->SetObjectFlags(0x10000);
        g_gameReg->ReportError(IDX(IDS_DEFAULT_ERROR), 0x401);
        return 0;
    }
    POSITION pos = m_baseList.GetHeadPosition();
    i32 stop = 0;
    i32 overCapacity = stop;
    i32 replacedExisting = stop;
    if (m_baseList.GetCount() > 0x3b) {
        overCapacity = 1;
    }
    while (pos != NULL && stop == 0) {
        POSITION cur = pos;
        CGruntPuddle* existing = static_cast<CGruntPuddle*>(m_baseList.GetNext(pos));
        if (existing->m_tileX == puddle->m_tileX && existing->m_tileY == puddle->m_tileY) {
            if (existing->m_pending != 0) {
                puddle->SetObjectFlags(0x10000);
                return 0;
            }
            existing->SetObjectFlags(0x10000);
            m_baseList.RemoveAt(cur);
            stop = 1;
            replacedExisting = 1;
        }
    }
    if (overCapacity != 0 && replacedExisting == 0) {
        pos = m_baseList.GetHeadPosition();
        stop = 0;
        while (pos != NULL && stop == 0) {
            POSITION cur = pos;
            CGruntPuddle* existing = static_cast<CGruntPuddle*>(m_baseList.GetNext(pos));
            if (existing->m_pending == 0) {
                existing->SetObjectFlags(0x10000);
                m_baseList.RemoveAt(cur);
                stop = 1;
            }
        }
    }
    m_baseList.AddTail(puddle);
    return 1;
}

RVA(0x0007a3f0, 0xd7)
i32 CTriggerMgr::LoadToyBoxIcon(i32 x, i32 y, i32 col, PickupType kind, i32 moveKind) {
    CDDrawChildGroup* fac = m_world->m_childGroup;
    i32 tx = x >> TILE_SHIFT_PX;
    i32 ty = y >> TILE_SHIFT_PX;

    POSITION pos = fac->m_list.GetHeadPosition();
    while (pos != NULL) {
        CGameObject* obj = fac->NextChild(pos);
        LogicRecordDispatchFn dispatch = obj->m_logicRecord->m_dispatch;
        if (dispatch == DispatchInGameIconLogic || dispatch == DispatchInGameTextLogic) {
            i32 ox = obj->m_screenX >> TILE_SHIFT_PX;
            i32 oy = obj->m_screenY >> TILE_SHIFT_PX;
            if (tx == ox && ty == oy) {
                return 0;
            }
        }
    }

    CWwdGameObjectA* spr = fac->CreateSprite(0, x, y, 0x17318, "InGameIcon", 0x40003);
    if (!spr) {
        g_gameReg->ReportError(IDX(IDS_DEFAULT_ERROR), 0x402);
        return 0;
    }
    spr->ApplyName("GAME_TOYBOX");
    spr->m_points = IDX(kind);
    spr->m_score = col;
    spr->m_faceDirection = moveKind;
    spr->m_stateFlags |= SPRITE_STATE_HIDDEN;
    return 1;
}

RVA(0x0007a510, 0x9e)
i32 CTriggerMgr::StartPlayerDefeatSequence(i32 playerSelector) {
    i32 firstPlayerIndex, lastPlayerIndex;
    if (playerSelector == TM_ALL_PLAYERS) {
        firstPlayerIndex = 0;
        lastPlayerIndex = 3;
    } else {
        lastPlayerIndex = playerSelector;
        firstPlayerIndex = playerSelector;
    }
    if (firstPlayerIndex <= lastPlayerIndex) {
        CGrunt** units = &m_units[firstPlayerIndex * TM_UNITS_PER_PLAYER];
        i32 playersRemaining = lastPlayerIndex - firstPlayerIndex + 1;
        do {
            i32 unitsRemaining = 15;
            do {
                CGrunt* unit = *units;
                if (unit != NULL && unit->m_deathAnimStarted == 0) {
                    (static_cast<CGrunt*>(unit))->StartBombGruntRun();
                }
                units++;
                unitsRemaining--;
            } while (unitsRemaining != 0);
            playersRemaining--;
        } while (playersRemaining != 0);
    }
    if (playerSelector == g_curPlayer) {
        m_groupFlag = 0;
    }

    CPlay* world = static_cast<CPlay*>(g_gameReg->m_curState);
    world->FlushPendingOps();
    world->SetDefeatCountdown(0, 0xbb7);
    (static_cast<CStatusBarMgr*>(world->m_statusBar))->LockDestructButton(1);
    return 1;
}

RVA(0x0007a5e0, 0x121)
i32 CTriggerMgr::Serialize(CFileMemBase* ar, SerialMode mode, LogicTypeId, i32) {
    if (ar == NULL) {
        return 0;
    }

    if (mode != SERIAL_SAVE) {
        if (mode == SERIAL_LOAD) {
            if (this->Load(ar) == 0) {
                return 0;
            }
        }
    } else {
        if (this->ScanGroup(ar) == 0) {
            return 0;
        }
    }

    SerBandPair(ar, mode, &m_timerBase);
    SerBandPair(ar, mode, &m_gooTimerBaseLo);
    SerBandPair(ar, mode, &m_resourceTimerBaseLo);
    return 1;
}

// @early-stop
// Block topology and branch sequence agree exactly (31/31 blocks, 20 branches,
// 4 rets). Residue is two scheduling hunks around the MapLookupById out-param
// store. Transcribing retail's store order (`found = 0` ahead of the m_objectId
// load) was measured WORSE (99.19 -> 98.73), so the sink is a scheduler choice,
// not statement order.
RVA(0x0007a760, 0x373)
i32 CTriggerMgr::ScanGroup(CFileMemBase* ar) {
    if (ar == NULL) {
        return 0;
    }
    CDDrawSurfaceMgr* lvl = m_world;
    if (lvl == NULL) {
        return 0;
    }
    CGrunt** cell = m_units;
    i32 r = 4;
    do {
        i32 c = 15;
        do {
            CGrunt* g = *cell;
            i32 id = 0;
            if (g != NULL) {
                id = g->m_object->m_objectId;
                CGameObject* found = NULL;
                MapLookupById(lvl->m_childGroup->m_registeredGameObjectsById, id, found);
            }
            ar->Write(&id, sizeof(id));
            cell++;
            c--;
        } while (c != 0);
        r--;
    } while (r != 0);
    ar->Write(m_unitCountByPlayer, 0x10);
    ar->Write(m_unitExited, 0xf0);
    ar->Write(m_gruntzExitedByPlayer, 0x10);
    ar->Write(m_gruntzLostByPlayer, 0x10);
    u32 n = static_cast<u32>(m_byteArr.GetSize());
    ar->Write(&n, sizeof(n));
    for (u32 i = 0; i < n; i++) {
        u8 b = m_byteArr.GetData()[i];
        ar->Write(&b, sizeof(b));
    }
    n = static_cast<u32>(m_recList.GetCount());
    ar->Write(&n, sizeof(n));
    POSITION pos = m_recList.GetHeadPosition();
    while (pos != NULL) {
        ar->Write(m_recList.GetNext(pos), 8);
    }
    CPtrList* list = m_selLists;
    i32 k = 10;
    do {
        n = static_cast<u32>(list->GetCount());
        ar->Write(&n, sizeof(n));
        POSITION selPos = list->GetHeadPosition();
        while (selPos != NULL) {
            ar->Write(list->GetNext(selPos), 8);
        }
        list++;
        k--;
    } while (k != 0);
    CWwdGameObjectA* goal = m_goal;
    i32 objId = 0;
    if (goal != NULL) {
        objId = goal->m_objectId;
    }
    ar->Write(&objId, sizeof(objId));
    CWarlord* ov = m_pendingFx;
    objId = 0;
    if (ov != NULL && ov->m_object != NULL) {
        objId = ov->m_object->m_objectId;
    }
    ar->Write(&objId, sizeof(objId));
    ar->Write(m_reserved274, 0x10);
    n = static_cast<u32>(m_baseList.GetCount());
    ar->Write(&n, sizeof(n));
    i32 hasOv;
    pos = m_baseList.GetHeadPosition();
    while (pos != NULL) {
        CGruntPuddle* obj = static_cast<CGruntPuddle*>(m_baseList.GetNext(pos));
        if (obj == NULL) {
            goto fail;
        }
        objId = obj->m_object->m_objectId;
        CGameObject* found = NULL;
        MapLookupById(lvl->m_childGroup->m_registeredGameObjectsById, objId, found);
        ar->Write(&objId, sizeof(objId));
    }
    hasOv = (m_overlay != NULL) ? 1 : 0;
    ar->Write(&hasOv, sizeof(hasOv));
    if (m_overlay != NULL) {
        if (m_overlay->Serialize(ar) == 0) {
            goto fail;
        }
    }
    ar->Write(&m_armed, sizeof(m_armed));
    ar->Write(&m_groupInitialized, sizeof(m_groupInitialized));
    ar->Write(&m_phase, sizeof(m_phase));
    ar->Write(&m_cameraTargetIdentity, sizeof(m_cameraTargetIdentity));
    ar->Write(&m_countdownActive, sizeof(m_countdownActive));
    ar->Write(&m_finishReasonFrame, sizeof(m_finishReasonFrame));
    ar->Write(&m_groupFlag, sizeof(m_groupFlag));
    ar->Write(&g_curPlayer, sizeof(g_curPlayer));
    ar->Write(&g_groupSentinel, sizeof(g_groupSentinel));
    ar->Write(&m_pendingFxKind, sizeof(m_pendingFxKind));
    ar->Write(&m_selSentinel, sizeof(m_selSentinel));
    return 1;
fail:
    return 0;
}

// @early-stop
RVA(0x0007abc0, 0x4b6)
i32 CTriggerMgr::Load(CFileMemBase* ar) {
    if (ar == NULL) {
        return 0;
    }
    CDDrawSurfaceMgr* world = m_world;
    if (world == NULL) {
        return 0;
    }
    m_rollingballLoop = NULL;
    m_teleportLoop = NULL;
    m_rollingballWanted = 0;
    m_teleportWanted = 0;

    for (i32 owner = 0; owner < TM_PLAYER_COUNT; owner++) {
        for (i32 i = 0; i < TM_UNITS_PER_PLAYER; i++) {
            i32 key;
            ar->Read(&key, sizeof(key));
            CGrunt* cell = NULL;
            if (key != 0) {
                CGameObject* found = NULL;
                if (MapLookupById(world->m_childGroup->m_registeredGameObjectsById, key, found)
                    == 0) {
                    return 0;
                }
                if (found == NULL) {
                    return 0;
                }
                cell = static_cast<CGrunt*>(found->m_logicRecord->m_userLogic);
                if (cell == NULL) {
                    return 0;
                }
            }
            m_units[owner * TM_UNITS_PER_PLAYER + i] = cell;
        }
    }

    ar->Read(m_unitCountByPlayer, 0x10);
    ar->Read(m_unitExited, 0xf0);
    ar->Read(m_gruntzExitedByPlayer, 0x10);
    ar->Read(m_gruntzLostByPlayer, 0x10);

    i32 count;
    u32 ci;
    ar->Read(&count, sizeof(count));
    CByteArray* arr = &m_byteArr;
    arr->SetSize(0, -1);
    for (ci = 0; ci < static_cast<u32>(count); ci++) {
        i32 b;
        ar->Read(&b, 1);
        arr->SetAtGrow(ci, b);
    }
    ClearRecords();

    ar->Read(&count, sizeof(count));
    CPtrList* rec = &m_recList;
    for (ci = 0; ci < static_cast<u32>(count); ci++) {
        CoordPoolNode* fl = g_coordPool.m_freeHead;
        Coord* node = NULL;
        if (fl->m_next != NULL) {
            node = &fl->m_coord;
            g_coordPool.m_freeHead = fl->m_next;
        }
        ar->Read(node, 8);
        rec->AddTail(node);
    }

    CPtrList* sel = m_selLists;
    i32 slot = 0xa;
    do {
        ar->Read(&count, sizeof(count));
        for (ci = 0; ci < static_cast<u32>(count); ci++) {
            CoordPoolNode* fl = g_coordPool.m_freeHead;
            Coord* node = NULL;
            if (fl->m_next != NULL) {
                node = &fl->m_coord;
                g_coordPool.m_freeHead = fl->m_next;
            }
            ar->Read(node, 8);
            sel->AddTail(node);
        }
        sel++;
    } while (--slot != 0);

    {
        i32 key;
        ar->Read(&key, sizeof(key));
        if (key != 0) {
            CGameObject* found = NULL;
            CGameObject* looked = NULL;
            if (MapLookupById(world->m_childGroup->m_registeredGameObjectsById, key, found) != 0) {
                looked = found;
            }
            CWwdGameObjectA* obj;
            if (looked == NULL) {
                obj = NULL;
            } else {
                obj = (looked->GetClassId() == CLASSID_SERIALREF)
                          ? static_cast<CWwdGameObjectA*>(looked)
                          : NULL;
            }
            m_goal = obj;
            if (obj == NULL) {
                return 0;
            }
        }
    }

    {
        i32 key;
        ar->Read(&key, sizeof(key));
        if (key != 0) {
            CGameObject* found = NULL;
            CGameObject* looked = NULL;
            if (MapLookupById(world->m_childGroup->m_registeredGameObjectsById, key, found) != 0) {
                looked = found;
            }
            if (looked == NULL) {
                return 0;
            }
            CWarlord* obj = static_cast<CWarlord*>(looked->m_logicRecord->m_userLogic);
            m_pendingFx = obj;
            if (obj == NULL) {
                return 0;
            }
        } else {
            m_pendingFx = NULL;
        }
    }

    ar->Read(m_reserved274, 0x10);
    m_baseList.RemoveAll();
    ar->Read(&count, sizeof(count));
    for (ci = 0; ci < static_cast<u32>(count); ci++) {
        i32 key;
        ar->Read(&key, sizeof(key));
        if (key == 0) {
            return 0;
        }
        CGameObject* found = NULL;
        CGameObject* looked = NULL;
        if (MapLookupById(world->m_childGroup->m_registeredGameObjectsById, key, found) != 0) {
            looked = found;
        }
        if (looked == NULL) {
            return 0;
        }
        CGruntPuddle* obj = static_cast<CGruntPuddle*>(looked->m_logicRecord->m_userLogic);
        if (obj == NULL) {
            return 0;
        }
        m_baseList.AddTail(obj);
    }

    CActionOptionsMenuBar* old = m_overlay;
    if (old != NULL) {
        old->Clear();
        ::operator delete(old);
        m_overlay = NULL;
    }
    i32 hasOverlay;
    ar->Read(&hasOverlay, sizeof(hasOverlay));
    if (hasOverlay != 0) {
        CActionOptionsMenuBar* ov = new CActionOptionsMenuBar;
        m_overlay = ov;
        if (ov->Deserialize(ar) == 0) {
            return 0;
        }
    }

    ar->Read(&m_armed, sizeof(m_armed));
    ar->Read(&m_groupInitialized, sizeof(m_groupInitialized));
    ar->Read(&m_phase, sizeof(m_phase));
    ar->Read(&m_cameraTargetIdentity, sizeof(m_cameraTargetIdentity));
    ar->Read(&m_countdownActive, sizeof(m_countdownActive));
    ar->Read(&m_finishReasonFrame, sizeof(m_finishReasonFrame));
    ar->Read(&m_groupFlag, sizeof(m_groupFlag));
    ar->Read(&g_curPlayer, sizeof(g_curPlayer));
    ar->Read(&g_groupSentinel, sizeof(g_groupSentinel));
    ar->Read(&m_pendingFxKind, sizeof(m_pendingFxKind));
    ar->Read(&m_selSentinel, sizeof(m_selSentinel));
    return 1;
}

// @early-stop
RVA(0x0007b1b0, 0x12b)
i32 CTriggerMgr::TriggerCell(i32 x, i32 y) {
    CActionOptionsMenuBar* ov = m_overlay;
    m_pendingFxKind = 0;
    if (ov == NULL || ov->m_active == 0) {
        return 0;
    }
    CGrunt* cell;
    if (m_recList.GetCount() != 1) {
        cell = NULL;
    } else {
        Coord* rec = static_cast<Coord*>(m_recList.GetHead());
        cell = m_units[rec->m_x * TM_UNITS_PER_PLAYER + rec->m_y];
    }
    CPlay* world = static_cast<CPlay*>(g_gameReg->m_curState);
    ActionOptionHit kind = ov->HitHover(x, y);
    if (kind == ACTIONOPTION_HIT_PRIMARY) {
        PickupType alt = ArrivalPickup(cell);
        if (alt == PICKUP_WAND) {
            g_gameReg->m_triggerMgr->ResetGroup(
                cell->LastTilePx().m_x,
                cell->LastTilePx().m_y,
                0,
                0,
                0,
                TARGET_SELECTION_GRUNT,
                1
            );
        }
    } else if (kind == ACTIONOPTION_HIT_SECONDARY) {

        PickupType alt = cell->m_vehiclePickupType;
        if (alt == PICKUP_SCROLL) {
            CGameObject* o = cell->m_object;
            g_gameReg->m_triggerMgr
                ->ResetGroup(o->m_screenX, o->m_screenY, 0, 0, 0, TARGET_SELECTION_TOY, 1);
        } else if (alt != PICKUP_NONE) {
            i32 v = IDX(alt) + kPendingFxIdBase;
            m_pendingFxKind = v;
            world->LoadCursorSprites(v, 0);
        }
    }
    this->OverlayTick();
    this->PlaceObjectFull(x, y);
    return 1;
}

RVA(0x0007b330, 0xc6)

i32 CTriggerMgr::LoadExplosionSprites(i32 x, i32 y, i32 id, i32 kind) {
    CDDrawChildGroup* fac = m_world->m_childGroup;
    CWwdGameObjectA* spr = fac->CreateSprite(0, x, y, 0, "Explosion", 0x40003);
    if (spr) {
        i32 v = kind;
        if (v == 0) {
            v = (rand(), 1);
        }
        CString key;
        key.Format("GAME_EXPLOSION%d", v);
        spr->ApplyLookupGeometry(key, 0);
        spr->m_smarts = id;
        spr->m_score = 1;
    }
    return spr != NULL;
}

// @early-stop
RVA(0x0007b440, 0x3f0)
i32 CTriggerMgr::BuildRockBreakParticles(i32 cx, i32 cy, i32 r, i32 flag) {
    CombatCue(cx, cy, r, CUE_EXPLODE, flag);

    CPlay* root = static_cast<CPlay*>(g_gameReg->m_curState);
    i32 tileCx = cx >> TILE_SHIFT_PX;
    i32 tileCy = cy >> TILE_SHIFT_PX;
    for (i32 tx = tileCx - r; tx <= tileCx + r; tx++) {
        i32 pxX = (tx << TILE_SHIFT_PX) + TILE_HALF_PX;
        for (i32 ty = tileCy - r; ty <= tileCy + r; ty++) {
            i32 pxY = (ty << TILE_SHIFT_PX) + TILE_HALF_PX;
            if (pxX < 0x10 || pxY < 0x10) {
                continue;
            }
            CGameLevel* board = m_world->m_level;
            if (tx >= board->m_mainPlane->m_wrapW || ty >= board->m_mainPlane->m_wrapH) {
                continue;
            }
            i32 col = tx;
            i32 row = ty;
            if (pxX < 0x10) {
                col = 0;
            } else if (tx >= board->m_mainPlane->m_gridW) {
                col = board->m_mainPlane->m_gridW - 1;
            }
            if (ty >= board->m_mainPlane->m_gridH) {
                row = board->m_mainPlane->m_gridH - 1;
            }
            i32 cell = board->m_mainPlane->m_tileGrid[board->m_mainPlane->m_rowOffsets[row] + col];
            TileCollisionKind type;
            if (cell == UNINIT_FILL || cell == -1) {
                type = TILEKIND_PASSABLE;
            } else {
                CTileImageSet* o =
                    static_cast<CTileImageSet*>(board->m_imageSets.GetAt(cell & 0xffff));
                // Ingest: the raw WWD attribute byte for this cell.
                type = o->GetCollisionAt(0, 0);
            }

            if (type != TILEKIND_GAUNTLET_ROCK_A && type != TILEKIND_GAUNTLET_ROCK_B) {
                if (type == TILEKIND_GIANT_ROCK) {
                    CGiantRockLogic* gr = root->m_tileTriggers->ScanNeighborhood(tx, ty);
                    if (gr == NULL) {
                        CString msg;
                        msg.Format("No giant rock logic found around: x=%d, y=%d", cx, cy);
                        g_gameReg->EnterModalUI(msg);
                        g_gameReg->ReportError(
                            IDX(TRIGERR_LOOKUP_MISS),
                            IDX(TRIGSITE_ROCK_SCAN_MISS)
                        );
                        return 0;
                    }
                    gr->BuildRockBreakInGameText();
                    root->m_tileTriggers->RemoveIdleLogic(gr);
                    continue;
                }
                if (type != TILEKIND_GAUNTLET_BRICK_A && type != TILEKIND_GAUNTLET_BRICK_B
                    && type != TILEKIND_GAUNTLET_BRICK_C) {
                    continue;
                }
                CTileActionEvent* o = root->m_tileTriggers->FindActionByCellKey(ty + (tx << 8));
                if (o->Process(NULL)) {
                    root->m_tileTriggers->RemoveActionEvent(o);
                }
                continue;
            }

            CTileTriggerLogic* lo =
                root->m_tileTriggers->FindLogic(ty + (tx << 8), TRIGID_COVERED_POWERUP_26);
            if (lo != NULL) {
                lo->ApplyMove(type);
                root->m_tileTriggers->RemoveIdleLogic(lo);
            } else {
                CGruntzMgr* reg = g_gameReg;
                CDDrawWorkerHost* wg = reg->m_world->m_level->m_mainPlane;
                i32 off = wg->m_rowOffsets[ty];
                if (type == TILEKIND_GAUNTLET_ROCK_A) {
                    wg->m_tileGrid[off + tx] = 0x5a;
                    (reg->m_tileGrid)->ComputeCellFlags(tx, ty, 0x5a);
                } else {
                    wg->m_tileGrid[off + tx] = 0x5b;
                    (reg->m_tileGrid)->ComputeCellFlags(tx, ty, 0x5b);
                }
            }

            POINT pt;
            pt.x = pxX;
            pt.y = pxY;
            if (!PtInRect(&g_gameReg->m_viewBounds, pt)) {
                continue;
            }
            CWwdGameObjectA* spr =
                m_world->m_childGroup
                    ->CreateSprite(0, pxX, pxY, SORTKEY_ACTOR_BEHIND, "Particlez", 0x40003);
            if (spr == NULL) {
                continue;
            }
            spr->ApplyName("LEVEL_ROCKBREAK");
            spr->ApplyLookupGeometry("LEVEL_ROCKBREAK", 0);

            SoundCueRegistry* registry = m_world->m_soundRegistry;
            if (registry->m_silentMode == 0) {

                SoundCue* found = NULL;
                MapLookup(registry->m_cues, "LEVEL_ROCKBREAK", found);
                // SoundCue::PlayIfElapsed inlined: the call's `this` copy holds the
                // cue in a register across the m_lastPlayTimeMs store.
                SoundCue* cue = found;
                if (cue != NULL) {
                    i32 soundEnabled = g_soundEnabled;
                    i32 volumePercent = g_soundVolumePercent;
                    if (soundEnabled != 0) {
                        u32 cueTimeMs = g_soundCueTimeMs;
                        if (cueTimeMs - cue->m_lastPlayTimeMs >= cue->m_replayDelayMs) {
                            cue->m_lastPlayTimeMs = cueTimeMs;
                            cue->m_sound->AcquireAndPlay(volumePercent, 0, 0, 0);
                        }
                    }
                }
            }
        }
    }
    return 1;
}

// @early-stop
RVA(0x0007b930, 0x3e0)
i32 CTriggerMgr::CombatCue(i32 x, i32 y, i32 radius, CombatCueKind tier, i32 flag) {
    i32 r = radius << TILE_SHIFT_PX;
    RECT area;
    area.left = x - r - 7;
    area.right = x + r + 7;
    area.top = y - r - 7;
    area.bottom = y + r + 7;
    i32 rangeA = m_world->m_level->m_mainPlane->m_gridW - 2;
    i32 rangeB = m_world->m_level->m_mainPlane->m_gridH - 2;

    CGrunt** units = m_units;
    for (i32 playerIndex = 0; playerIndex < TM_PLAYER_COUNT; playerIndex++) {
        for (i32 unitIndex = 0; unitIndex < TM_UNITS_PER_PLAYER; unitIndex++, units++) {
            CGrunt* g = *units;
            if (g == NULL) {
                continue;
            }
            if (g->m_entranceCommitted == 0) {
                continue;
            }
            if (g->m_entranceDropActive != 0) {
                continue;
            }
            i32 gx = g->m_object->m_screenX;
            i32 gy = g->m_object->m_screenY;
            i32 lx = gx - 7;
            i32 ly = gy - 7;
            i32 hx = lx + 14;
            i32 hy = ly + 14;
            if (area.left <= hx && area.right >= lx && area.top <= hy && area.bottom >= ly) {
                switch (tier) {
                    case CUE_DROP:
                        if (g->m_gruntKind != GRUNT_INVULNERABLE) {
                            StartUnitDeath(playerIndex, unitIndex, DEATH_DROP, flag);
                        }
                        break;
                    case CUE_EXPLODE:
                        if (g->m_gruntKind != GRUNT_INVULNERABLE) {
                            StartUnitDeath(playerIndex, unitIndex, DEATH_EXPLODE, flag);
                        }
                        break;
                    case CUE_SQUASH:
                        if (g->m_gruntKind != GRUNT_INVULNERABLE) {
                            StartUnitDeath(playerIndex, unitIndex, DEATH_SQUASH, flag);
                        }
                        break;
                    case CUE_TELEPORT: {
                        if (gx == x && gy == y) {
                            break;
                        }
                        i32 done = 0;
                        do {
                            i32 dx =
                                rangeA == 0 ? static_cast<char>(rand()) & 1 : rand() % rangeA + 1;
                            i32 dy =
                                rangeB == 0 ? static_cast<char>(rand()) & 1 : rand() % rangeB + 1;
                            if (g->TryTeleportToCell(dx, dy, 0, 1)) {
                                CGameObject* spr = g_gameReg->m_world->m_childGroup->CreateSprite(
                                    0,
                                    gx,
                                    gy,
                                    SORTKEY_OVERLAY,
                                    "LightFx",
                                    0x40003
                                );
                                done = 1;
                                spr->m_logicRecord->m_dispatch(spr);
                                (static_cast<CLightFx*>(spr->m_logicRecord->m_userLogic))
                                    ->Activate("GAME_LIGHTING_FLASH", "GAME_FLASH", 3, 1);
                            }
                        } while (done == 0);
                        break;
                    }
                    case CUE_HEAL: {
                        if (gx == x && gy == y) {
                            break;
                        }
                        g->m_health = HEALTH_FULL;
                        g->CreateHealthSprite();
                        ArmGruntCombatTimeout(g);
                        CGameObject* spr =
                            g_gameReg->m_world->m_childGroup
                                ->CreateSprite(0, gx, gy, SORTKEY_OVERLAY, "LightFx", 0x40003);
                        spr->m_logicRecord->m_dispatch(spr);
                        (static_cast<CLightFx*>(spr->m_logicRecord->m_userLogic))
                            ->Activate("GAME_LIGHTING_FLASH", "GAME_FLASH", 2, 1);
                        break;
                    }
                    case CUE_GIVE_TOY: {
                        if (gx == x && gy == y) {
                            break;
                        }
                        PickupType toy =
                            static_cast<PickupType>(rand() % 9 + IDX(PICKUP_TOYZ_FIRST));
                        if (toy == PICKUP_SCROLL) {
                            toy = PICKUP_YOYO;
                        }
                        g->LoadGruntTypeTable(toy, 1, 0, 0);
                        CGameObject* spr =
                            g_gameReg->m_world->m_childGroup
                                ->CreateSprite(0, gx, gy, SORTKEY_OVERLAY, "LightFx", 0x40003);
                        spr->m_logicRecord->m_dispatch(spr);
                        (static_cast<CLightFx*>(spr->m_logicRecord->m_userLogic))
                            ->Activate("GAME_LIGHTING_FLASH", "GAME_FLASH", 7, 1);
                        break;
                    }
                    case CUE_FREEZE: {
                        if (gx == x && gy == y) {
                            break;
                        }
                        g->StepArrivalCommit();
                        CGameObject* h = g->m_object;
                        CWwdGameObjectA* spr = g_gameReg->m_world->m_childGroup->CreateSprite(
                            0,
                            h->m_screenX,
                            h->m_screenY,
                            SORTKEY_OVERLAY,
                            "LightFx",
                            0x40003
                        );
                        spr->m_logicRecord->m_dispatch(spr);
                        (static_cast<CLightFx*>(spr->m_logicRecord->m_userLogic))
                            ->Activate("GAME_LIGHTING_FLASH", "GAME_FLASH", 9, 1);
                        break;
                    }
                }
            }
        }
    }
    return 1;
}

RVA(0x0007be10, 0x34)
void CTriggerMgr::StopPendingFx() {
    CPlay* world = static_cast<CPlay*>(g_gameReg->m_curState);
    if (m_pendingFxKind == 0 && world->m_cursorTargetValid == 0) {
        return;
    }
    world->LoadCursorSprites(0, 0);
    m_pendingFxKind = 0;
}

// @early-stop
RVA(0x0007be60, 0x21e)
i32 CTriggerMgr::LoadGruntResurrectTuning(i32 cx, i32 cy, i32 r) {
    RECT rect;
    i32 hx = cx >> TILE_SHIFT_PX;
    i32 hy = cy >> TILE_SHIFT_PX;
    rect.left = hx - r;
    rect.top = hy - r;
    rect.right = hx + r;
    rect.bottom = hy + r;

    POSITION pos = m_baseList.GetHeadPosition();
    while (pos != NULL) {
        POSITION cur = pos;
        CGruntPuddle* g = static_cast<CGruntPuddle*>(m_baseList.GetNext(pos));
        if (g->m_pending != 0) {
            continue;
        }
        i32 tx = g->m_tileX;
        i32 ty = g->m_tileY;
        POINT pt;
        pt.x = tx;
        pt.y = ty;
        if (!PtInRect(&rect, pt)) {
            continue;
        }

        i32 playerIndex = g->m_playerIndex;
        GruntzPlayer* player = &g_gameReg->m_players[playerIndex];
        i32 aiType = 0;
        i32 ok = 0;
        i32 radius = 0;

        if (g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
            if (player->m_humanControlled == 0) {
                aiType = g_buteMgr.GetInt("Grunt", "RessurectAIType");
                radius = g_buteMgr.GetInt("Grunt", "RessurectAIRadius");
            }
            if (PlaceObject(
                    playerIndex,
                    (tx << TILE_SHIFT_PX) + TILE_HALF_PX,
                    (ty << TILE_SHIFT_PX) + TILE_HALF_PX,
                    0x186a0,
                    GRUNT_ENTRANCE_RESURRECT,
                    g->m_moveIcon,
                    0,
                    0,
                    aiType,
                    radius,
                    0,
                    0,
                    NULL
                )
                != -1) {
                ok = 1;
            }
        } else if (player->m_active != 0 && player->m_doneFlag == 0
                   && player->m_clearedRound == 0) {
            if (player->m_humanControlled != 0) {
                if (PlaceObject(
                        playerIndex,
                        (tx << TILE_SHIFT_PX) + TILE_HALF_PX,
                        (ty << TILE_SHIFT_PX) + TILE_HALF_PX,
                        0x186a0,
                        GRUNT_ENTRANCE_RESURRECT,
                        g->m_moveIcon,
                        0,
                        0,
                        0,
                        0,
                        0,
                        0,
                        NULL
                    )
                    != -1) {
                    ok = 1;
                }
            } else if (player->m_battlezConfig.TrySeedSpawnAt(tx, ty) != 0) {
                ok = 1;
            }
        }

        if (ok) {
            g->SetObjectFlags(0x10000);

            m_baseList.RemoveAt(cur);
            CGameObject* spr = g_gameReg->m_world->m_childGroup->CreateSprite(
                0,
                (tx << TILE_SHIFT_PX) + TILE_HALF_PX,
                (ty << TILE_SHIFT_PX) + TILE_HALF_PX,
                SORTKEY_OVERLAY,
                "LightFx",
                0x40003
            );
            spr->m_logicRecord->m_dispatch(spr);
            (static_cast<CLightFx*>(spr->m_logicRecord->m_userLogic))
                ->Activate("GAME_LIGHTING_FLASH", "GAME_FLASH", 8, 1);
        }
    }
    return 1;
}

// @early-stop
RVA(0x0007c110, 0x166)
i32 CTriggerMgr::SpawnGrunt(
    i32 srcPlayerIndex,
    i32 srcUnitIndex,
    i32 dstPlayerIndex,
    i32 moveIcon
) {
    CGrunt* src = m_units[srcPlayerIndex * TM_UNITS_PER_PLAYER + srcUnitIndex];
    i32 freeUnitIndex = 0;
    i32 dstBaseIndex = dstPlayerIndex * TM_UNITS_PER_PLAYER;
    if (m_units[dstBaseIndex] != NULL) {
        CGrunt** units = &m_units[dstPlayerIndex * TM_UNITS_PER_PLAYER];
        while (freeUnitIndex < TM_UNITS_PER_PLAYER) {
            units++;
            freeUnitIndex++;
            if (*units == NULL) {
                break;
            }
        }
    }
    if (freeUnitIndex >= TM_UNITS_PER_PLAYER) {
        return 0;
    }
    CGameObject* o = src->m_object;
    i32 sx = (o->m_screenX & ~TILE_MASK_PX) + TILE_HALF_PX;
    i32 sy = (o->m_screenY & ~TILE_MASK_PX) + TILE_HALF_PX;
    PickupType k = ARRIVAL_PICKUP_TERNARY_GT(src);
    PickupType vis = src->m_vehiclePickupType;
    this->StartUnitDeath(srcPlayerIndex, srcUnitIndex, DEATH_DROP, dstPlayerIndex);
    CDDrawChildGroup* fac = m_world->m_childGroup;
    CWwdGameObjectA* sprite = fac->CreateSprite(0, sx, sy, 0x186a0, "Grunt", 0x40003);
    if (sprite == NULL) {
        return 0;
    }
    sprite->m_logicRecord->m_dispatch(sprite);

    CGrunt* logic = static_cast<CGrunt*>(sprite->m_logicRecord->m_userLogic);

    if (logic->Place(
            this,
            dstPlayerIndex,
            freeUnitIndex,
            static_cast<PickupType>(moveIcon),
            k,
            vis,
            AI_NONE,
            0,
            0,
            0,
            NULL,
            GRUNT_ENTRANCE_NONE
        )
        == 0) {
        logic->SetObjectFlags(0x10000);
        return 0;
    }
    m_units[dstBaseIndex + freeUnitIndex] = logic;
    m_unitCountByPlayer[dstPlayerIndex] += 1;
    m_unitExited[dstBaseIndex + freeUnitIndex] = 0;
    return 1;
}

RVA(0x0007c2e0, 0xb5)
i32 CTriggerMgr::CycleMoveIcons(i32 skipPlayerIndex, i32 enable) {
    i32 playerIndex = 0;
    CGrunt** playerUnits = m_units;
    for (; playerIndex < TM_PLAYER_COUNT; playerIndex++, playerUnits += TM_UNITS_PER_PLAYER) {
        if (playerIndex != skipPlayerIndex) {
            CGrunt** units = playerUnits;
            i32 unitsRemaining = TM_UNITS_PER_PLAYER;
            do {
                CGrunt* g = *units;
                if (g != NULL) {
                    if (enable != 0) {
                        i32 t = rand() % 0x11;
                        if (g->m_savedMoveIcon == -1) {
                            g->m_savedMoveIcon = IDX(g->m_moveIcon);
                        }
                        (static_cast<CGrunt*>(g))->SelectMoveIcon(t);
                        (static_cast<CPlay*>(g_gameReg->m_curState))->SetRandomMoveIconsCurse(1);
                    } else if (g->m_savedMoveIcon != -1) {
                        (static_cast<CGrunt*>(g))->SelectMoveIcon(g->m_savedMoveIcon);
                        g->m_savedMoveIcon = -1;
                    }
                }
                units++;
                unitsRemaining--;
            } while (unitsRemaining != 0);
        }
    }
    return 1;
}

// @early-stop
RVA(0x0007c3d0, 0x1d0)
void CTriggerMgr::LoadFinishLevelSprite(FinishLevelReason state) {
    switch (state) {
        case FINISH_REASON_WARPSTONE_EXIT:
            if (m_phase != FINISH_STATE_DEFEAT) {
                SoundCue* p = LookupCue(m_world->m_soundRegistry->m_cues, "GAME_FINISHLEVEL");
                m_timerWindow = static_cast<u32>((p->m_sound->m_durationMs + 500));
                m_timerBase = g_frameTime;
                if (m_world->m_soundRegistry->m_silentMode == 0) {
                    SoundCue* cue = LookupCue(m_world->m_soundRegistry->m_cues, "GAME_FINISHLEVEL");
                    if (cue != NULL) {
                        i32 volumePercent = g_soundVolumePercent;
                        if (g_soundEnabled != 0
                            && static_cast<u32>((g_soundCueTimeMs - cue->m_lastPlayTimeMs))
                                   >= static_cast<u32>(cue->m_replayDelayMs)) {
                            cue->m_lastPlayTimeMs = g_soundCueTimeMs;
                            cue->m_sound->AcquireAndPlay(volumePercent, 0, 0, 0);
                        }
                    }
                }
                m_phase = FINISH_STATE_VICTORY;
                m_groupFlag = 0;
                m_finishReasonFrame = state;
                return;
            }
            goto Lab_56b;
        case FINISH_REASON_WARPSTONE_RESET:
            m_phase = FINISH_STATE_DEFEAT;
            goto Lab_522;
        case FINISH_REASON_BATTLEZ_VICTORY:
            m_phase = FINISH_STATE_VICTORY;
            break;
        case FINISH_REASON_TIME_EXPIRED:
            m_phase = FINISH_STATE_DEFEAT;
            m_timerWindow = 3000;
            m_timerBase = g_frameTime;
            goto Lab_56b;
        case FINISH_REASON_NO_GRUNTZ_REMAIN:
            if (m_phase == FINISH_STATE_ACTIVE) {
                m_phase = FINISH_STATE_DEFEAT;
                if (m_pendingFx != NULL) {
                    m_pendingFx->ResolveDeathAnimation();
                }
            }
        Lab_522:
            m_timerWindow = 3000;
            m_timerBase = g_frameTime;
            goto Lab_56b;
        case FINISH_REASON_BATTLEZ_DEFEAT:
            m_phase = FINISH_STATE_DEFEAT;
            break;
        default:
            return;
    }
    m_timerWindow = 3000;
    m_timerBase = g_frameTime;
Lab_56b:
    m_groupFlag = 0;
    m_finishReasonFrame = state;
}

RVA(0x0007c620, 0x500)
i32 CTriggerMgr::LoadPowerupIconSprites(
    PickupType type,
    i32 geoB,
    i32 geoA,
    i32 m130,
    i32 warpIdx,
    i32 m120
) {
    if (type == PICKUP_NONE) {
        return 0;
    }

    CString name;
    switch (type) {
        case PICKUP_BOMB:
            name = "GAME_INGAMEICONZ_TOOLZ_BOMBZ";
            break;
        case PICKUP_BOOMERANG:
            name = "GAME_INGAMEICONZ_TOOLZ_BOOMERANGZ";
            break;
        case PICKUP_BRICK:
            name = "GAME_INGAMEICONZ_TOOLZ_BRICKZ";
            break;
        case PICKUP_CLUB:
            name = "GAME_INGAMEICONZ_TOOLZ_CLUBZ";
            break;
        case PICKUP_GAUNTLETZ:
            name = "GAME_INGAMEICONZ_TOOLZ_GAUNTLETZ";
            break;
        case PICKUP_GLOVEZ:
            name = "GAME_INGAMEICONZ_TOOLZ_GLOVEZ";
            break;
        case PICKUP_GOOBER:
            name = "GAME_INGAMEICONZ_TOOLZ_GOOBERZ";
            break;
        case PICKUP_GRAVITYBOOTZ:
            name = "GAME_INGAMEICONZ_TOOLZ_GRAVITYBOOTZ";
            break;
        case PICKUP_GUNHAT:
            name = "GAME_INGAMEICONZ_TOOLZ_GUNHATZ";
            break;
        case PICKUP_NERFGUN:
            name = "GAME_INGAMEICONZ_TOOLZ_NERFGUNZ";
            break;
        case PICKUP_ROCK:
            name = "GAME_INGAMEICONZ_TOOLZ_ROCKZ";
            break;
        case PICKUP_SHIELD:
            name = "GAME_INGAMEICONZ_TOOLZ_SHIELDZ";
            break;
        case PICKUP_SHOVEL:
            name = "GAME_INGAMEICONZ_TOOLZ_SHOVELZ";
            break;
        case PICKUP_SPRING:
            name = "GAME_INGAMEICONZ_TOOLZ_SPRINGZ";
            break;
        case PICKUP_SPY:
            name = "GAME_INGAMEICONZ_TOOLZ_SPYZ";
            break;
        case PICKUP_SWORD:
            name = "GAME_INGAMEICONZ_TOOLZ_SWORDZ";
            break;
        case PICKUP_TIMEBOMB:
            name = "GAME_INGAMEICONZ_TOOLZ_TIMEBOMBZ";
            break;
        case PICKUP_TOOB:
            name = "GAME_INGAMEICONZ_TOOLZ_TOOBZ";
            break;
        case PICKUP_WAND:
            name = "GAME_INGAMEICONZ_TOOLZ_WANDZ";
            break;
        case PICKUP_WARPSTONE:
            if (g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {

                CState* st = g_gameReg->m_curState;
                CString lvl;
                lvl.Format("Level%i", st->m_levelIndex);
                name.Format(
                    "GAME_INGAMEICONZ_TOOLZ_WARPSTONEZ%i",
                    g_buteMgr.GetInt("WarpStone", static_cast<const char*>(lvl))
                );
            } else {
                name.Format("GAME_INGAMEICONZ_TOOLZ_WARPSTONEZ%i", warpIdx);
            }
            break;
        case PICKUP_WELDER:
            name = "GAME_INGAMEICONZ_TOOLZ_WELDERZ";
            break;
        case PICKUP_WINGZ:
            name = "GAME_INGAMEICONZ_TOOLZ_WINGZ";
            break;
        case PICKUP_BABYWALKER:
            name = "GAME_INGAMEICONZ_TOYZ_BABYWALKERZ";
            break;
        case PICKUP_BEACHBALL:
            name = "GAME_INGAMEICONZ_TOYZ_BEACHBALLZ";
            break;
        case PICKUP_BIGWHEEL:
            name = "GAME_INGAMEICONZ_TOYZ_BIGWHEELZ";
            break;
        case PICKUP_GOKART:
            name = "GAME_INGAMEICONZ_TOYZ_GOKARTZ";
            break;
        case PICKUP_JACKINTHEBOX:
            name = "GAME_INGAMEICONZ_TOYZ_JACKINTHEBOXZ";
            break;
        case PICKUP_JUMPROPE:
            name = "GAME_INGAMEICONZ_TOYZ_JUMPROPEZ";
            break;
        case PICKUP_POGOSTICK:
            name = "GAME_INGAMEICONZ_TOYZ_POGOSTICKZ";
            break;
        case PICKUP_SCROLL:
            name = "GAME_INGAMEICONZ_TOYZ_SCROLLZ";
            break;
        case PICKUP_SQUEAKTOY:
            name = "GAME_INGAMEICONZ_TOYZ_SQUEAKTOYZ";
            break;
        case PICKUP_YOYO:
            name = "GAME_INGAMEICONZ_TOYZ_YOYOZ";
            break;
        case PICKUP_MEGAPHONE:
            name = "GAME_INGAMEICONZ_POWERUPZ_MEGAPHONEZ";
            break;
        case PICKUP_HEALTH1:
            name = "GAME_INGAMEICONZ_POWERUPZ_HEALTH1";
            break;
        case PICKUP_HEALTH2:
            name = "GAME_INGAMEICONZ_POWERUPZ_HEALTH2";
            break;
        case PICKUP_HEALTH3:
            name = "GAME_INGAMEICONZ_POWERUPZ_HEALTH3";
            break;
        case PICKUP_CONVERSION:
            name = "GAME_INGAMEICONZ_POWERUPZ_CONVERSION";
            break;
        case PICKUP_DEATHTOUCH:
            name = "GAME_INGAMEICONZ_POWERUPZ_DEATHTOUCH";
            break;
        case PICKUP_GHOST:
            name = "GAME_INGAMEICONZ_POWERUPZ_GHOST";
            break;
        case PICKUP_INVULNERABILITY:
            name = "GAME_INGAMEICONZ_POWERUPZ_INVULNERABILITY";
            break;
        case PICKUP_REACTIVEARMOR:
            name = "GAME_INGAMEICONZ_POWERUPZ_REACTIVEARMOR";
            break;
        case PICKUP_ROIDZ:
            name = "GAME_INGAMEICONZ_POWERUPZ_ROIDZ";
            break;
        case PICKUP_SUPERSPEED:
            name = "GAME_INGAMEICONZ_POWERUPZ_SUPERSPEED";
            break;
        case PICKUP_W:
            name = "GAME_INGAMEICONZ_SECRETW";
            break;
        case PICKUP_A:
            name = "GAME_INGAMEICONZ_SECRETA";
            break;
        case PICKUP_R:
            name = "GAME_INGAMEICONZ_SECRETR";
            break;
        case PICKUP_P:
            name = "GAME_INGAMEICONZ_SECRETP";
            break;
        case PICKUP_STOPWATCH:
            name = "GAME_INGAMEICONZ_POWERUPZ_STOPWATCH";
            break;
        case PICKUP_COIN:
            name = "GAME_INGAMEICONZ_POWERUPZ_COIN";
            break;
        case PICKUP_COVEREDTIMEBOMB: {
            CGameObject* tb = g_gameReg->m_world->m_childGroup
                                  ->CreateSprite(0, geoB, geoA, 0xf, "TimeBomb", 0x40003);
            if (tb) {
                tb->m_damage = g_buteMgr.GetDwordDef("Powerupz", "CoveredTimeBombTime", 0x7d0);
            }
            return tb != NULL;
        }
        default:
            return 0;
    }

    CWwdGameObjectA* spr = g_gameReg->m_world->m_childGroup
                               ->CreateSprite(0, geoB, geoA, 0x17318, "InGameIcon", 0x40003);
    if (!spr) {
        return 0;
    }
    spr->ApplyName(name);
    spr->m_damage = m120;
    spr->m_score = 0;
    spr->m_points = 0;
    spr->m_smarts = 0;
    spr->m_powerup = 0;
    spr->m_health = 0;
    spr->m_direction = 0;
    spr->m_faceDirection = m130;
    return 1;
}

// @early-stop
RVA(0x0007cc60, 0xa7)
i32 CTriggerMgr::RebuildSelectionList(i32 idx) {
    POSITION pos = m_selLists[idx].GetHeadPosition();
    if (pos != NULL) {
        CoordPoolNode* head = g_coordPool.m_freeHead;
        do {
            Coord* payload = static_cast<Coord*>(m_selLists[idx].GetNext(pos));
            if (payload != NULL) {
                CoordPoolNode* slot = g_coordPool.NodeOf(payload);
                slot->m_next = head;
                head = slot;
                g_coordPool.m_freeHead = head;
            }
        } while (pos != NULL);
    }
    CPtrList* sel = &m_selLists[idx];
    sel->RemoveAll();
    pos = m_recList.GetHeadPosition();
    while (pos != NULL) {
        Coord* src = static_cast<Coord*>(m_recList.GetNext(pos));
        CoordPoolNode* fhNode = g_coordPool.m_freeHead;
        Coord* dst = NULL;
        if (fhNode->m_next != NULL) {
            dst = &fhNode->m_coord;
            g_coordPool.m_freeHead = fhNode->m_next;
        }
        *dst = *src;
        sel->AddTail(dst);
    }
    m_selSentinel = -1;
    return 1;
}

RVA(0x0007cd40, 0x18f)
i32 CTriggerMgr::CenterSelectionGroup(i32 slot) {
    ResetAll();
    CActionOptionsMenuBar* ov = m_overlay;
    if (ov != NULL && ov->m_active != 0) {
        OverlayTick();
    }
    POSITION pos = m_selLists[slot].GetHeadPosition();
    if (pos == NULL) {
        m_selSentinel = -1;
        return 0;
    }

    RECT bbox;
    bbox.right = 0;
    bbox.bottom = 0;
    CDDrawWorkerHost* grid = g_gameReg->m_world->m_level->m_mainPlane;
    bbox.left = grid->m_wrapW - 1;
    bbox.top = grid->m_wrapH - 1;
    do {
        POSITION cur = pos;
        Coord* payload = static_cast<Coord*>(m_selLists[slot].GetNext(pos));
        i32 idx = payload->m_y + TM_UNITS_PER_PLAYER * payload->m_x;
        CGrunt* cell = m_units[idx];
        if (cell != NULL) {
            ResetCell(payload->m_x, payload->m_y, 1, 0);
            if (m_selSentinel == slot) {
                CGameObject* disp = cell->m_object;
                i32 x = disp->m_screenX;
                i32 y = disp->m_screenY;
                if (x < bbox.left) {
                    bbox.left = x;
                }
                if (x > bbox.right) {
                    bbox.right = x;
                }
                if (y < bbox.top) {
                    bbox.top = y;
                }
                if (y > bbox.bottom) {
                    bbox.bottom = y;
                }
            }
        } else {
            CoordPoolNode* node = g_coordPool.NodeOf(payload);
            node->m_next = g_coordPool.m_freeHead;
            g_coordPool.m_freeHead = node;
            m_selLists[slot].RemoveAt(cur);
        }
    } while (pos != NULL);
    if (m_selSentinel == slot) {
        (static_cast<CPlay*>(g_gameReg->m_curState))
            ->ResetGoals(
                bbox.left + (bbox.right - bbox.left) / 2,
                bbox.top + (bbox.bottom - bbox.top) / 2
            );
        m_selSentinel = -1;
        return 1;
    }
    m_selSentinel = slot;
    return 1;
}

// @early-stop
RVA(0x0007cf40, 0x12e)
i32 CTriggerMgr::CenterOnGroup(i32 doSelect) {
    POSITION pos = m_recList.GetHeadPosition();
    if (pos == NULL) {
        return 0;
    }
    RECT bbox;
    i32 count = 0;
    CDDrawWorkerHost* dims = g_gameReg->m_world->m_level->m_mainPlane;
    bbox.left = dims->m_wrapW - 1;
    bbox.top = dims->m_wrapH - 1;
    bbox.right = 0;
    bbox.bottom = 0;
    do {
        Coord* k = static_cast<Coord*>(m_recList.GetNext(pos));
        CGrunt* cell = m_units[k->m_x * TM_UNITS_PER_PLAYER + k->m_y];
        if (cell != NULL) {
            count++;
            CGameObject* g = cell->m_object;
            i32 gx = g->m_screenX;
            i32 gy = g->m_screenY;
            if (gx < bbox.left) {
                bbox.left = gx;
            }
            if (gx > bbox.right) {
                bbox.right = gx;
            }
            if (gy < bbox.top) {
                bbox.top = gy;
            }
            if (gy > bbox.bottom) {
                bbox.bottom = gy;
            }
        }
    } while (pos != NULL);
    i32 cy = bbox.top + (bbox.bottom - bbox.top) / 2;
    i32 cx = bbox.left + (bbox.right - bbox.left) / 2;
    (static_cast<CPlay*>(g_gameReg->m_curState))->ResetGoals(cx, cy);
    if (doSelect != 0 && count == 1) {
        CGrunt* cell2;
        if (m_recList.GetCount() != 1) {
            cell2 = NULL;
        } else {
            Coord* head = static_cast<Coord*>(m_recList.GetHead());
            cell2 = m_units[head->m_x * TM_UNITS_PER_PLAYER + head->m_y];
        }
        if (cell2 != NULL) {
            i32 playerIndex = cell2->m_playerIndex;
            i32 unitIndex = cell2->m_unitIndex;
            if (RecordListHas(playerIndex, unitIndex)) {
                m_cameraTargetIdentity.m_x = playerIndex;
                m_cameraTargetIdentity.m_y = unitIndex;
                m_armed = 1;
                LoadCameraSprite();
            }
        }
    }
    return 1;
}

RVA(0x0007d0c0, 0x57)
void CTriggerMgr::ClearSelections() {
    CPtrList* list = m_selLists;
    i32 k = 10;
    do {
        POSITION pos = list->GetHeadPosition();
        if (pos != NULL) {
            do {
                Coord* payload = static_cast<Coord*>(list->GetNext(pos));
                if (payload != NULL) {
                    CoordPoolNode* slot = g_coordPool.NodeOf(payload);
                    slot->m_next = g_coordPool.m_freeHead;
                    g_coordPool.m_freeHead = slot;
                }
            } while (pos != NULL);
        }
        list->RemoveAll();
        list++;
        k--;
    } while (k != 0);
    m_selSentinel = -1;
}

RVA(0x0007d140, 0x61)
i32 CTriggerMgr::StartPlayerVictorySequence(i32 playerIndex) {
    CGrunt** units = &m_units[playerIndex * TM_UNITS_PER_PLAYER];
    i32 unitsRemaining = 15;
    do {
        CGrunt* unit = *units;
        if (unit != NULL && unit->m_deathAnimStarted == 0) {
            (static_cast<CGrunt*>(unit))->BuildGruntExitAnimation();
        }
        units++;
        unitsRemaining--;
    } while (unitsRemaining != 0);
    if (playerIndex == g_curPlayer) {
        m_groupFlag = 0;
    }
    (static_cast<CPlay*>(g_gameReg->m_curState))->FlushPendingOps();
    return 1;
}

RVA(0x0007d1d0, 0x9d)
i32 CTriggerMgr::NearestOtherPlayerUnitDistSq(i32 skipPlayerIndex, i32 px, i32 py) {
    i32 tx = px >> TILE_SHIFT_PX;
    i32 ty = py >> TILE_SHIFT_PX;
    i32 best = INT_MAX;
    i32 playerIndex = 0;
    CGrunt** playerUnits = m_units;
    do {
        if (playerIndex != skipPlayerIndex) {
            i32 unitsRemaining = TM_UNITS_PER_PLAYER;
            CGrunt** units = playerUnits;
            do {
                CGrunt* g = *units;
                if (g != NULL && g->m_entranceCommitted != 0) {
                    CGameObject* o = g->m_object;
                    i32 dx = (o->m_screenX >> TILE_SHIFT_PX) - tx;
                    i32 dy = (o->m_screenY >> TILE_SHIFT_PX) - ty;
                    i32 d = abs(dx * dx + dy * dy);
                    if (d < best) {
                        best = d;
                    }
                }
                units++;
                unitsRemaining--;
            } while (unitsRemaining != 0);
        }
        playerIndex++;
        playerUnits += TM_UNITS_PER_PLAYER;
    } while (playerIndex < TM_PLAYER_COUNT);
    return best;
}

RVA(0x0007d2a0, 0x64)
i32 CTriggerMgr::SelectionListFind(i32 playerIndex, i32 unitIndex) {
    if (playerIndex != g_curPlayer) {
        return 0;
    }
    i32 result = 0;
    CPtrList* list = m_selLists;
    for (i32 i = 0; i < 10; i++, list++) {
        POSITION pos = list->GetHeadPosition();
        while (pos != NULL) {
            Coord* payload = static_cast<Coord*>(list->GetNext(pos));
            if (payload->m_x == playerIndex && payload->m_y == unitIndex) {
                if (result != 0) {
                    return 10;
                }
                result = i;
            }
        }
    }
    return result;
}

// @early-stop
RVA(0x0007d330, 0xd3)
void CTriggerMgr::DestroyAllAnims() {
    CGrunt** cell = m_units;
    i32 r = 4;
    do {
        i32 i = 15;
        do {
            CGrunt* g = *cell;
            if (g != NULL) {
                (static_cast<CGrunt*>(g))->DestroyAnims();
            }
            cell++;
            i--;
        } while (i != 0);
        r--;
    } while (r != 0);

    CDDrawChildGroup* children = m_world->m_childGroup;
    POSITION pos = children->m_list.GetHeadPosition();
    while (pos != NULL) {
        CGameObject* obj = children->NextChild(pos);
        if (obj != NULL) {
            CLogicRecord* record = obj->m_logicRecord;

            LogicDispatchWord actualDispatch;
            LogicDispatchWord projectileDispatch;
            actualDispatch.m_dispatch = record->m_dispatch;
            projectileDispatch.m_dispatch = DispatchProjectileLogic;
            if (actualDispatch.m_bits == projectileDispatch.m_bits) {
                (static_cast<CGrunt*>(record->m_userLogic))->m_neighborPlayerIndex = 0;
            }
        }
    }

    SoundBuffer* rollingballSound = m_rollingballLoop;
    if (rollingballSound != NULL) {
        rollingballSound->StopAndRewind();
        m_rollingballLoop = NULL;
    }
    SoundBuffer* teleportSound = m_teleportLoop;
    if (teleportSound != NULL) {
        teleportSound->StopAndRewind();
        m_teleportLoop = NULL;
    }
    CState* state = g_gameReg->PickPausedThenPlayState();
    if (state != NULL) {
        CStatusBarMgr* sub = (static_cast<CPlay*>(state))->m_statusBar;
        if (sub != NULL) {
            SoundBuffer* destructWarningSound = sub->m_destructWarningSound;
            if (destructWarningSound != NULL) {
                destructWarningSound->StopAndRewind();
                sub->m_destructWarningSound = NULL;
            }
        }
    }
}

RVA(0x0007d450, 0x112)
i32 CTriggerMgr::ToggleRegionA() {
    if (m_pendingFxKind != 0) {
        m_pendingFxKind = 0;
        (static_cast<CPlay*>(g_gameReg->m_curState))->LoadCursorSprites(0, 0);
        return 0;
    }
    m_pendingFxKind = 0;

    CGrunt* cell;
    if (m_recList.GetCount() != 1) {
        cell = NULL;
    } else {
        Coord* rec = static_cast<Coord*>(m_recList.GetHead());
        cell = m_units[rec->m_y + rec->m_x * TM_UNITS_PER_PLAYER];
    }
    if (cell != NULL && cell->m_playerIndex == g_curPlayer) {
        if ((static_cast<CGrunt*>(cell))->CanShowStamina() == 0) {
            OverlayTick();
        } else {
            PickupType v = ArrivalPickup(cell);
            if (v == PICKUP_WAND) {
                g_gameReg->m_triggerMgr->ResetGroup(
                    cell->LastTilePx().m_x,
                    cell->LastTilePx().m_y,
                    0,
                    0,
                    0,
                    TARGET_SELECTION_GRUNT,
                    1
                );
            } else {
                m_pendingFxKind = IDX(v) + kPendingFxIdBase;
                (static_cast<CPlay*>(g_gameReg->m_curState))
                    ->LoadCursorSprites(IDX(v) + kPendingFxIdBase, 0);
            }
            OverlayTick();
        }
    }
    return 1;
}

RVA(0x0007d5c0, 0xdc)
i32 CTriggerMgr::ToggleRegionB() {
    if (m_pendingFxKind != 0) {
        m_pendingFxKind = 0;
        (static_cast<CPlay*>(g_gameReg->m_curState))->LoadCursorSprites(0, 0);
        return 0;
    }
    m_pendingFxKind = 0;
    CGrunt* cell;
    if (m_recList.GetCount() != 1) {
        cell = NULL;
    } else {
        Coord* rec = static_cast<Coord*>(m_recList.GetHead());
        cell = m_units[rec->m_y + rec->m_x * TM_UNITS_PER_PLAYER];
    }
    if (cell != NULL && cell->m_playerIndex == g_curPlayer) {
        if (cell->m_entranceReason >= PICKUP_TOYZ_FIRST) {
            OverlayTick();
        } else {
            PickupType kind = cell->m_vehiclePickupType;
            if (kind == PICKUP_SCROLL) {
                CGameObject* o = cell->m_object;
                g_gameReg->m_triggerMgr
                    ->ResetGroup(o->m_screenX, o->m_screenY, 0, 0, 0, TARGET_SELECTION_TOY, 1);
            } else if (kind != PICKUP_NONE) {
                m_pendingFxKind = IDX(kind) + kPendingFxIdBase;
                (static_cast<CPlay*>(g_gameReg->m_curState))
                    ->LoadCursorSprites(IDX(kind) + kPendingFxIdBase, 0);
            }
            OverlayTick();
        }
    }
    return 1;
}

RVA(0x0007d6e0, 0xea)
i32 CTriggerMgr::EnqueueGroupCells() {
    if (m_groupFlag == 0) {
        return 0;
    }

    u8 buf[0x80];
    u8 count = 0;
    char x;
    POSITION pos = m_recList.GetHeadPosition();
    if (pos != NULL) {
        i32 magic = g_curPlayer;
        do {
            Coord* p = static_cast<Coord*>(m_recList.GetNext(pos));

            CGrunt* cell = m_units[p->m_x * TM_UNITS_PER_PLAYER + p->m_y];
            x = static_cast<char>(p->m_x);
            if (cell->m_playerIndex == magic && cell->m_entranceActive == 0) {
                buf[count] = static_cast<u8>(p->m_y);
                count++;
            }
        } while (pos != NULL);
    }
    if (count == 1) {
        g_gameReg->m_commandMgr->EnqueueSingle(
            1,
            x,
            static_cast<char>(buf[0]),
            static_cast<char>(IDX(PLAYERCMD_STOP)),
            0,
            0,
            0,
            0
        );
    } else {
        g_gameReg->m_commandMgr
            ->EnqueueMulti(1, x, count, buf, static_cast<char>(IDX(PLAYERCMD_STOP)), 0, 0, 0);
    }
    return 1;
}
