#include <Gruntz/TriggerMgr.h>

#include <AddrWord.h>
#include <Bute/ButeMgr.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/PixelShift.h>
#include <Dsndmgr/DirectSoundMgr.h>
#include <Enums.h>
#include <Gruntz/ActionOptionsMenuBar.h>
#include <Gruntz/BattlezMapConfig.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/CombatCueKind.h>
#include <Gruntz/EnemyAiType.h>
#include <Gruntz/ErrorStringId.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameModeId.h>
#include <Gruntz/GameObjectFactory.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntDeathType.h>
#include <Gruntz/GruntPuddle.h>
#include <Gruntz/GruntSpawnConfig.h>
#include <Gruntz/GruntzCmdMgr.h>
#include <Gruntz/GruntzCommandId.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/HealthPct.h>
#include <Gruntz/LeafCue.h>
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

static char s_Grunt[] = "Grunt";
static char s_CombatTimeout[] = "CombatTimeout";

// @early-stop
RVA(0x00077f80, 0xab)
CGrunt* CTriggerMgr::FindNearestInRow(CGrunt* g) {
    i32 tx = g->m_lastTilePx.m_x >> TILE_SHIFT_PX;
    i32 rowIdx = g->m_tileOwnerHi;
    CGrunt** cell = &m_grid[rowIdx * TM_GRID_COLS];
    i32 ty = g->m_lastTilePx.m_y >> TILE_SHIFT_PX;
    CGrunt* best = 0;
    i32 bestDist = INT_MAX;
    i32 i = 15;
    do {
        CGrunt* c = *cell;
        if (c != NULL) {
            CGameObject* o = c->m_object;
            i32 dx = (o->m_screenX >> TILE_SHIFT_PX) - tx;
            i32 dy = (o->m_screenY >> TILE_SHIFT_PX) - ty;
            i32 d = dx * dx + dy * dy;
            if (d < bestDist && d < g->m_defenderRadius * 2) {
                best = c;
                bestDist = d;
            }
        }
        cell++;
        i--;
    } while (i != 0);
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
    // (&m_grid[0]) outside, `mov ebx,eax` at the outer head, `add ebx,4` per inner
    // step and `mov eax,ebx` at the outer tail - so the row base carries forward and
    // the cell is m_grid[i * TM_GRID_COLS + j], not m_grid[j].
    for (i32 i = 0; i < TM_GRID_ROWS; i++) {
        for (i32 j = 0; j < TM_GRID_COLS; j++) {
            CGrunt* g = m_grid[i * TM_GRID_COLS + j];
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
i32 CTriggerMgr::RemoveCellRecord(i32 x, i32 y, i32 fromSelection) {
    if (fromSelection != 0) {
        CPtrList* list = m_selLists;
        i32 k = 10;
        do {
            POSITION pos = list->GetHeadPosition();
            while (pos != NULL) {
                POSITION cur = pos;
                i32* p = static_cast<i32*>(list->GetNext(pos));
                if (p[0] == x && p[1] == y) {
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
        if (p->m_x == x && p->m_y == y) {
            if (m_recList.GetCount() == 1) {
                StopPendingFx();
            }
            CGrunt* cell = m_grid[y + x * TM_GRID_COLS];
            if (cell != NULL) {
                (static_cast<CGrunt*>(cell))->ClearAllSprites();
            }
            i32 px = p->m_x;
            i32 py = p->m_y;
            if (px == m_recordPosition.m_x && py == m_recordPosition.m_y) {
                CWwdGameObjectA* goal = m_goal;
                if (goal != NULL) {
                    goal->m_flags |= 0x10000;
                    m_goal = NULL;
                }
                m_armed = 0;
            }
            CActionOptionsMenuBar* ov = m_overlay;
            if (ov != NULL) {
                i32 qx = p->m_x;
                i32 ax = ov->m_gridX;
                i32 qy = p->m_y;
                i32 ay = ov->m_gridY;
                if (ax == qx && ay == qy) {
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
        i32* payload = static_cast<i32*>(m_recList.GetNext(pos));
        i32 idx = payload[1] + TM_GRID_COLS * payload[0];
        CGrunt* cell = m_grid[idx];
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
i32 CTriggerMgr::RecordListHas(i32 x, i32 y) {
    POSITION pos = m_recList.GetHeadPosition();
    while (pos != NULL) {
        i32* p = static_cast<i32*>(m_recList.GetNext(pos));
        if (p[0] == x && p[1] == y) {
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
        i32* payload = static_cast<i32*>(m_recList.GetNext(pos));
        CGrunt* cell = m_grid[payload[1] + payload[0] * TM_GRID_COLS];
        firstByte = static_cast<u8>(payload[0]);
        if (cell->m_tileOwnerHi == g_curPlayer && cell->m_entranceActive == 0) {
            bytes[count] = static_cast<u8>(payload[1]);
            count++;
        }
    }
    CGruntzCmdMgr* rep = g_gameReg->m_cmdSubMgr;
    if (count == 1) {
        g_gameReg->m_cmdSubMgr->EnqueueSingle(
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
        g_gameReg->m_cmdSubMgr->EnqueueMulti(
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
        i32* payload = static_cast<i32*>(m_recList.GetNext(pos));
        CGrunt* cell = m_grid[payload[1] + payload[0] * TM_GRID_COLS];
        firstByte = static_cast<u8>(payload[0]);
        if (cell->m_tileOwnerHi == g_curPlayer && cell->m_entranceActive == 0) {
            bytes[count] = static_cast<u8>(payload[1]);
            count++;
        }
    }
    CGruntzCmdMgr* rep = g_gameReg->m_cmdSubMgr;
    if (count == 1) {
        if (flag != 0) {
            g_gameReg->m_cmdSubMgr->EnqueueSingle(
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
            g_gameReg->m_cmdSubMgr->EnqueueSingle(
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
            g_gameReg->m_cmdSubMgr->EnqueueMulti(
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
            g_gameReg->m_cmdSubMgr->EnqueueMulti(
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
    CGameObject* src = m_grid[m_recordPosition.m_x * TM_GRID_COLS + m_recordPosition.m_y]->m_object;
    i32 y = src->m_screenY;
    i32 x = src->m_screenX;
    CDDrawWorkerHost* t = m_world->m_level->m_mainPlane;
    if (t->m_flags & 1) {
        t->m_scaledX = static_cast<float>(x);
        t->m_scaledY = static_cast<float>(y);
    } else {
        t->m_scaledX = static_cast<float>(x) * t->m_scaleX;
        t->m_scaledY = static_cast<float>(y) * t->m_scaleY;
    }
    t->RecomputePlaneCoords();
    return 1;
}

RVA(0x00078960, 0x9b)
i32 CTriggerMgr::LoadCameraSprite() {
    if (m_goal != NULL) {
        return 0;
    }

    i32 vx = g_gameReg->m_modeSize.cx;
    i32 vy = g_gameReg->m_modeSize.cy;
    StatusBarDock pos = (static_cast<CPlay*>(g_gameReg->m_curState))->m_guts->m_position;

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
    spr->m_animWorker->m_notify(spr);
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
static inline u16 PackRgb16(i32 r, i32 g, i32 b) {
    return static_cast<u16>(((r >> g_rDown) << g_rUp) | ((g >> g_gDown) << g_gUp) | (b >> g_bDown));
}

// @early-stop
// The case order is read off retail's own jump table (13 slots at 0x79298, byte
// index at 0x792cc); sorting its distinct targets by address gives the source
// order, and it is the order below.
//
// Two known residues.  (1) The second path block (retail 0x79080) CALLS
// WrapCoord for `source` but has it expanded for `destination`, which never
// leaves edi/ebx - so that expansion must come from an inline WrapCoord in the
// Wwd header, not from a hand-written copy here.  Transcribing the body into
// this arm reproduces the clamp blocks and un-merges the two path-preview
// return tails, but it also makes cl re-order the switch arms (BOOMERANG moves
// from ninth to third), measured 71.89 -> 38.61; an inline clone is also what
// gruntz.audit.inline_clones exists to prevent.  The real fix is to make
// CDDrawWorkerHost::WrapCoord an inline member and let the /Ob1 budget produce
// retail's one-call/one-expansion split.  (2) cl merges identical
// `LoadCursorSprites(...); return 1;` blocks that retail keeps duplicated.
RVA(0x00078a50, 0x8a0)
i32 CTriggerMgr::PlaceObjectFull(i32 x, i32 y) {

    CGrunt* cell;
    if (m_recList.GetCount() != 1) {
        cell = NULL;
    } else {
        i32* rec = static_cast<i32*>(m_recList.GetHead());
        cell = m_grid[rec[1] + rec[0] * TM_GRID_COLS];
    }
    if (cell == NULL || cell->m_tileOwnerHi != g_curPlayer) {
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
    if (CellHitTest(x, y, 0, 0, TM_GRID_ROW_ALL)) {
        hitFlag = 1;
    }

    CGameLevel* view = m_world->m_level;
    i32 tx = x >> TILE_SHIFT_PX;
    i32 ty = y >> TILE_SHIFT_PX;
    i32 cx = tx;
    if (tx < 0) {
        cx = 0;
    } else if (tx >= view->m_mainPlane->m_gridW) {
        cx = view->m_mainPlane->m_gridW - 1;
    }
    i32 cy = ty;
    if (ty < 0) {
        cy = 0;
    } else if (ty >= view->m_mainPlane->m_gridH) {
        cy = view->m_mainPlane->m_gridH - 1;
    }
    TileCollisionKind collision;
    i32 cval = view->m_mainPlane->m_tileGrid[view->m_mainPlane->m_colOffsets[cy] + cx];
    if (cval != UNINIT_FILL && cval != -1) {
        CTileImageSet* tc = static_cast<CTileImageSet*>(view->m_imageSets.GetAt(cval & 0xffff));
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
            return 1;
        }
        CGruntzMapMgr* plane = g_gameReg->m_tileGrid;
        i32 attr;
        if (static_cast<u32>(tx) >= static_cast<u32>(plane->m_width)
            || static_cast<u32>(ty) >= static_cast<u32>(plane->m_height)) {
            attr = 1;
        } else {
            attr = plane->m_rowInts[ty][tx * 7];
        }
        if ((attr & BRICKZ_BLOCKED_MASK) != 0 || (attr & 2) != 0) {
            world->LoadCursorSprites(pfk, 0);
        } else {
            world->LoadCursorSprites(IDX(alt) + kPendingFxIdBase, 1);
        }
        return 1;
    }

    PickupType gruntKind = cell->m_entranceReason;
    if (gruntKind > PICKUP_EQUIPPABLE_LAST) {
        gruntKind = cell->m_toolId;
    }

    if (hitFlag != 0) {
        if (pfk != 0) {
            // Retail's compare order is rock, welder, boomerang, gunhat,
            // nerfgun, wingz; the positive spelling is byte-identical here.
            if (gruntKind != GRUNT_ROCK && gruntKind != GRUNT_WELDER && gruntKind != GRUNT_BOOMERANG
                && gruntKind != GRUNT_GUNHAT && gruntKind != GRUNT_NERFGUN
                && gruntKind != GRUNT_WINGZ) {
                world->LoadCursorSprites(IDX(gruntKind) + kPendingFxIdBase, 1);
                return 1;
            }

            POINT source = {cell->m_object->m_screenX, cell->m_object->m_screenY};
            view->m_mainPlane->WrapCoord(&source.x, &source.y);
            POINT destination = {x, y};
            view->m_mainPlane->WrapCoord(&destination.x, &destination.y);
            u16 color;
            if (cell->RectContains(x, y)) {
                color = PackRgb16(0xff, 0, 0);
                world->LoadCursorSprites(IDX(gruntKind) + kPendingFxIdBase, 1);
            } else {
                color = PackRgb16(0x20, 0x20, 0x20);
                world->LoadCursorSprites(pfk, 0);
            }
            world->m_pathPreviewSource = source;
            world->m_pathPreviewDestination = destination;
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
                if (g_gameReg->m_gameMode != GAMEMODE_SINGLE) {
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
                    void* out = 0;
                    CMapPtrToPtr* map = &g_gameReg->m_world->m_childGroup->m_map48;
                    CGameObject* occupant = NULL;
                    if (MapLookupById(*map, occupantId, out) != 0) {
                        occupant = static_cast<CGameObject*>(out);
                    }
                    if (occupant != NULL) {
                        CUserLogic* logic = occupant->m_animWorker->m_logic;
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
                    view->m_mainPlane->WrapCoord(&source.x, &source.y);
                    POINT destination = {x, y};
                    view->m_mainPlane->WrapCoord(&destination.x, &destination.y);
                    u16 color;
                    if (cell->RectContains(x, y)) {
                        color = PackRgb16(0xff, 0, 0);
                        world->LoadCursorSprites(IDX(gruntKind) + kPendingFxIdBase, 1);
                    } else {
                        color = PackRgb16(0x20, 0x20, 0x20);
                        world->LoadCursorSprites(pfk, 0);
                    }
                    world->m_pathPreviewSource = source;
                    world->m_pathPreviewDestination = destination;
                    world->m_pathPreviewColor = color;
                    world->m_drewThisFrame = 1;
                    return 1;
                }
                break;

            case PICKUP_TIMEBOMB: {
                CGruntzMapMgr* plane = g_gameReg->m_tileGrid;
                i32 attr;
                if (static_cast<u32>(tx) >= static_cast<u32>(plane->m_width)
                    || static_cast<u32>(ty) >= static_cast<u32>(plane->m_height)) {
                    attr = 1;
                } else {
                    attr = plane->m_rowInts[ty][tx * 7];
                }
                if (pfk != 0 && (attr & BRICKZ_BLOCKED_MASK) == 0 && (attr & 2) == 0) {
                    world->LoadCursorSprites(IDX(gruntKind) + kPendingFxIdBase, 1);
                    return 1;
                }
                break;
            }
        }
    }

    world->LoadCursorSprites(pfk, 0);
    return 1;
}

// @early-stop
// One block differs (B25, the TARGET_SELECTION_TOY arm): retail cross-jumps its
// `Activate(...,3,1)` tail into the block the other two arms share; cl duplicates
// it. NOT a merge-policy difference - cl hoisted the m_animWorker reload above the
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
    CGrunt* hit = CellHitTest(x, y, 0, 0, TM_GRID_ROW_ALL);
    CGrunt* cell;
    if (m_recList.GetCount() != 1) {
        cell = NULL;
    } else {
        i32* rec = static_cast<i32*>(m_recList.GetHead());
        cell = m_grid[rec[0] * TM_GRID_COLS + rec[1]];
    }

    TargetSelectionKind sel;
    if (cell != NULL) {
        if (cell->m_tileOwnerHi != g_curPlayer) {
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
            sprite->m_animWorker->m_notify(sprite);
            (static_cast<CLightFx*>(sprite->m_animWorker->m_logic))
                ->Activate("GAME_LIGHTING_TARGETCURSOR", "GAME_TARGETCURSOR", 2, 1);
            return 1;
        case TARGET_SELECTION_GRUNT:
            if (hit != NULL) {
                i32 owner = hit->m_tileOwnerHi;
                if (owner == g_curPlayer && g_traitorMode == 0) {
                    if (cell != hit) {
                        goto reportError;
                    }
                    PickupType v = (hit->m_entranceReason <= PICKUP_EQUIPPABLE_LAST)
                                       ? hit->m_entranceReason
                                       : hit->m_toolId;
                    if (v != PICKUP_SPY) {
                        PickupType v2 = (hit->m_entranceReason <= PICKUP_EQUIPPABLE_LAST)
                                            ? hit->m_entranceReason
                                            : hit->m_toolId;
                        if (v2 != PICKUP_WAND) {
                            goto reportError;
                        }
                    }
                }
                this->ReportRecordsB(1, owner, hit->m_tileOwnerLo, 1);
            } else {
                this->ReportRecordsB(1, x, y, 0);
            }
            if (spawnCursor == 0) {
                return 1;
            }
            sprite =
                m_world->m_childGroup->CreateSprite(0, x, y, SORTKEY_OVERLAY, "LightFx", 0x40003);
            sprite->m_animWorker->m_notify(sprite);
            (static_cast<CLightFx*>(sprite->m_animWorker->m_logic))
                ->Activate("GAME_LIGHTING_TARGETCURSOR", "GAME_TARGETCURSOR", 1, 1);
            return 1;
        case TARGET_SELECTION_TOY:
            if (hit != NULL) {
                if (hit->m_tileOwnerHi == g_curPlayer && g_traitorMode == 0
                    && (cell != hit || hit->m_vehiclePickupType != PICKUP_SCROLL)) {
                    goto reportError;
                }
                i32 hitHi = hit->m_tileOwnerHi;
                i32 hitLo = hit->m_tileOwnerLo;
                i32 cellLo = cell->m_tileOwnerLo;
                i32 cellHi = cell->m_tileOwnerHi;
                g_gameReg->m_cmdSubMgr->EnqueueSingle(
                    1,
                    cellHi,
                    cellLo,
                    static_cast<char>(IDX(PLAYERCMD_USE_TOY_ON_GRUNT)),
                    hitHi,
                    hitLo,
                    0,
                    0
                );
            } else {
                i32 cellLo2 = cell->m_tileOwnerLo;
                i32 cellHi2 = cell->m_tileOwnerHi;
                g_gameReg->m_cmdSubMgr->EnqueueSingle(
                    1,
                    cellHi2,
                    cellLo2,
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
            sprite->m_animWorker->m_notify(sprite);
            (static_cast<CLightFx*>(sprite->m_animWorker->m_logic))
                ->Activate("GAME_LIGHTING_TARGETCURSOR", "GAME_TARGETCURSOR", 3, 1);
            return 1;
        default:
            return 1;
    }

reportError:
    g_gameReg->m_cueSink->SpawnVoiceDriver(cell, 0x324, -1, 0, -1, -1);
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
        i32* rec = static_cast<i32*>(m_recList.GetHead());
        cellp = m_grid[rec[1] + rec[0] * TM_GRID_COLS];
    }
    if (cellp == NULL) {
        return 0;
    }
    if (cellp->m_tileOwnerHi != g_curPlayer) {
        return 0;
    }
    if (m_overlay->Init(
            ACTIONOPTION_HIDDEN,
            ACTIONOPTION_HIDDEN,
            screenX,
            screenY,
            cellp->m_tileOwnerHi,
            cellp->m_tileOwnerLo
        )
        == 0) {
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
    if (m_groupInitialized != 0 || g_gameReg->m_gameMode != GAMEMODE_SINGLE) {
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
    CStatusBarMgr* sbi = lvl->m_guts;
    if (sbi->m_hlBusy == 0) {
        if (sbi->m_position == STATUSBAR_HIDDEN) {
            sbi->RefreshState();
        }
        if (sbi->m_activeTab != TAB_GAME) {
            sbi->SetTabState(SBICMD_TAB_GAME, MENUITEM_SELECTED);
        }
        sbi->SetTab(GAME_TAB_MENU, 1);
        sbi->Deactivate();
    }
    if (lvl->m_guts->EnsureSub(outR, outC, fragment) != 0) {
        lvl->m_guts->m_hlBusy = 1;
    } else {
        m_byteArr.Add(static_cast<u8>(IDX(fragment)));
    }
    m_groupInitialized = 1;
}

RVA(0x00079d90, 0xc5)
void CTriggerMgr::ResetSpawnState() {
    if (g_gameReg->m_gameMode != GAMEMODE_SINGLE) {
        return;
    }
    if (m_groupInitialized == 0) {
        return;
    }
    CPlay* world = static_cast<CPlay*>(g_gameReg->m_curState);
    CStatusBarMgr* st = world->m_guts;
    if (st->m_retabNotify != NULL) {
        operator delete(st->m_retabNotify);
        st->m_retabNotify = NULL;
    }
    world->m_guts->m_hlBusy = 0;
    if (m_byteArr.GetSize() > 0) {
        m_byteArr.RemoveAt(m_byteArr.GetSize() - 1, 1);
        CStatusBarMgr* ctx = world->m_guts;
        if (ctx->m_position != STATUSBAR_HIDDEN && ctx->m_activeTab == TAB_GAME) {
            ctx->ResetWidgets(0);
            world->m_guts->TryActivate();
        }
    }
    if (g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
        CWarlord* fx = m_pendingFx;
        if (fx != NULL) {
            fx->ResolveDeathAnimation();
        }
    }
    this->LoadFinishLevelSprite(FINISH_REASON_WARPSTONE_RESET);
}

RVA(0x00079ea0, 0xc2)
i32 CTriggerMgr::SpawnTileFx(i32 x, i32 y, i32 anchorIndex) {
    if (g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
        return 0;
    }
    CGruntzMapMgr* grid = g_gameReg->m_tileGrid;
    i32 tx = x >> TILE_SHIFT_PX;
    i32 ty = y >> TILE_SHIFT_PX;
    i32 tile;
    if (static_cast<u32>(tx) >= static_cast<u32>(grid->m_width)
        || static_cast<u32>(ty) >= static_cast<u32>(grid->m_height)) {
        tile = 1;
    } else {
        tile = grid->m_rowInts[ty][tx * 8 - tx];
    }
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
// regalloc: retail spills x/y into its `sub esp,0x8` frame and never reloads them - a
// spill pair, not a source local (both values also stay in ecx/edx and every use reads
// them there). docs/patterns/dead-eight-byte-coord-temp-is-unreproduced.md
RVA(0x00079fb0, 0x169)
void CTriggerMgr::NotifyCell(i32 row, i32 col, i32 z) {
    i32 idx = row * TM_GRID_COLS + col;
    CGrunt* cell = m_grid[idx];
    if (cell == NULL) {
        return;
    }
    if (cell->m_cellRemovalNotified != 0) {
        return;
    }
    if (cell->m_arrivalPending == 0) {
        this->ApplySwitch(cell, cell->m_lastTilePx.m_x, cell->m_lastTilePx.m_y);
    }
    Coord pt;
    pt.m_x = cell->m_lastTilePx.m_x;
    pt.m_y = cell->m_lastTilePx.m_y;
    CGruntzMapMgr* tg = g_gameReg->m_tileGrid;
    i32 rowIdx = pt.m_y >> TILE_SHIFT_PX;
    i32 cellCol = pt.m_x >> TILE_SHIFT_PX;
    tg->m_rows[rowIdx][cellCol].m_flagBytes[3] &= 0xdf;
    tg->m_rows[rowIdx][cellCol].m_occupantId = -1;
    m_grid[idx] = NULL;
    m_rowCount[row] -= 1;

    PickupType k;
    if (z != 0) {
        m_cellFlag[idx] = 1;
        m_gruntzExitedByPlayer[row] += 1;
        k = cell->m_entranceReason;
        if (k > PICKUP_EQUIPPABLE_LAST) {
            k = cell->m_toolId;
        }
        if (k == PICKUP_WARPSTONE) {
            if (g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
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
        m_gruntzLostByPlayer[row] += 1;
    }
    cell->m_cellRemovalNotified = 1;
}

RVA(0x0007a180, 0x86)
i32 CTriggerMgr::SpawnPuddle(i32 x, i32 y, i32 f124, i32 f114, i32 color, i32 f118) {
    CDDrawChildGroup* fac = m_world->m_childGroup;
    CWwdGameObjectA* sprite = fac->CreateSprite(0, x, y, 0xa, "GruntPuddle", 0x40003);
    if (sprite == NULL) {

        g_gameReg->ReportError(IDX(IDS_DEFAULT_ERROR), 0x400);
        return 0;
    }
    sprite->m_animWorker->m_notify(sprite);
    sprite->m_smarts = f124;
    sprite->m_score = f114;
    sprite->m_points = f118;
    return PlacePuddle(sprite, color);
}

RVA(0x0007a240, 0x143)
i32 CTriggerMgr::PlacePuddle(CGameObject* sprite, i32 color) {
    CGruntPuddle* tgt = static_cast<CGruntPuddle*>(sprite->m_animWorker->m_logic);
    i32 d = sprite->m_points;
    if (d == 0) {
        d = 0x19;
    }
    if (tgt->Place(sprite->m_smarts, sprite->m_score, color, d) == 0) {
        tgt->m_wwdObject->m_flags |= 0x10000;
        g_gameReg->ReportError(IDX(IDS_DEFAULT_ERROR), 0x401);
        return 0;
    }
    POSITION pos = m_baseList.GetHeadPosition();
    i32 stop = 0;
    i32 manyFlag = stop;
    i32 replaced = stop;
    if (m_baseList.GetCount() > 0x3b) {
        manyFlag = 1;
    }
    while (pos != NULL && stop == 0) {
        POSITION cur = pos;
        CGruntPuddle* o = static_cast<CGruntPuddle*>(m_baseList.GetNext(pos));
        if (o->m_tileX == tgt->m_tileX && o->m_tileY == tgt->m_tileY) {
            if (o->m_pending != 0) {
                tgt->m_wwdObject->m_flags |= 0x10000;
                return 0;
            }
            o->m_wwdObject->m_flags |= 0x10000;
            m_baseList.RemoveAt(cur);
            stop = 1;
            replaced = 1;
        }
    }
    if (manyFlag != 0 && replaced == 0) {
        pos = m_baseList.GetHeadPosition();
        stop = 0;
        while (pos != NULL && stop == 0) {
            POSITION cur = pos;
            CGruntPuddle* o = static_cast<CGruntPuddle*>(m_baseList.GetNext(pos));
            if (o->m_pending == 0) {
                o->m_wwdObject->m_flags |= 0x10000;
                m_baseList.RemoveAt(cur);
                stop = 1;
            }
        }
    }
    m_baseList.AddTail(tgt);
    return 1;
}

RVA(0x0007a3f0, 0xd7)
i32 CTriggerMgr::LoadToyBoxIcon(i32 x, i32 y, i32 col, PickupType kind, i32 moveKind) {
    CDDrawChildGroup* fac = m_world->m_childGroup;
    i32 tx = x >> TILE_SHIFT_PX;
    i32 ty = y >> TILE_SHIFT_PX;

    POSITION pos = fac->m_list.GetHeadPosition();
    while (pos != NULL) {
        CGameObject* obj = static_cast<CGameObject*>(fac->m_list.GetNext(pos));
        GameObjNotifyFn init = obj->m_animWorker->m_notify;
        if (init == CreateInGameIcon || init == CreateInGameText) {
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
i32 CTriggerMgr::ClearRowAndRefresh(i32 startRow) {
    i32 row, last;
    if (startRow == TM_GRID_ROW_ALL) {
        row = 0;
        last = 3;
    } else {
        last = startRow;
        row = startRow;
    }
    if (row <= last) {
        CGrunt** cell = &m_grid[row * TM_GRID_COLS];
        i32 n = last - row + 1;
        do {
            i32 i = 15;
            do {
                CGrunt* c = *cell;
                if (c != NULL && c->m_deathAnimStarted == 0) {
                    (static_cast<CGrunt*>(c))->StartBombGruntRun();
                }
                cell++;
                i--;
            } while (i != 0);
            n--;
        } while (n != 0);
    }
    if (startRow == g_curPlayer) {
        m_groupFlag = 0;
    }

    CPlay* world = static_cast<CPlay*>(g_gameReg->m_curState);
    world->FlushPendingOps();
    world->ArmSnapshot(0, 0xbb7);
    (static_cast<CStatusBarMgr*>(world->m_guts))->SetMode(1);
    return 1;
}

RVA(0x0007a5e0, 0x121)
i32 CTriggerMgr::Serialize(CFileMemBase* ar, SerialMode kind, LogicTypeId, i32) {
    if (ar == NULL) {
        return 0;
    }

    if (kind != SERIAL_SAVE) {
        if (kind == SERIAL_LOAD) {
            if (this->Load(ar) == 0) {
                return 0;
            }
        }
    } else {
        if (this->ScanGroup(ar) == 0) {
            return 0;
        }
    }

    SerBandPair(ar, kind, &m_timerBase);
    SerBandPair(ar, kind, &m_gooTimerBaseLo);
    SerBandPair(ar, kind, &m_resourceTimerBaseLo);
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
    CGrunt** cell = m_grid;
    i32 r = 4;
    do {
        i32 c = 15;
        do {
            CGrunt* g = *cell;
            i32 id = 0;
            if (g != NULL) {
                id = g->m_object->m_objectId;
                void* found = 0;
                MapLookupById(lvl->m_childGroup->m_map48, id, found);
            }
            ar->Write(&id, sizeof(id));
            cell++;
            c--;
        } while (c != 0);
        r--;
    } while (r != 0);
    ar->Write(m_rowCount, 0x10);
    ar->Write(m_cellFlag, 0xf0);
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
        void* found = 0;
        MapLookupById(lvl->m_childGroup->m_map48, objId, found);
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
    ar->Write(&m_recordPosition, sizeof(m_recordPosition));
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
    if (m_world == NULL) {
        return 0;
    }
    m_rollingballLoop = NULL;
    m_teleportLoop = NULL;
    m_rollingballWanted = 0;
    m_teleportWanted = 0;

    CMapPtrToPtr* map = &m_world->m_childGroup->m_map48;

    for (i32 base = 7; base < 0x43; base += 0xf) {
        for (i32 i = 0; i < 0xf; i++) {
            i32 key;
            ar->Read(&key, sizeof(key));
            void* cell = 0;
            if (key != 0) {
                void* found = 0;
                void* looked = 0;
                if (MapLookupById(*map, key, found) != 0) {
                    looked = found;
                }
                if (looked == NULL) {
                    return 0;
                }
                cell = (static_cast<CGameObject*>(looked))->m_animWorker->m_logic;
                if (cell == NULL) {
                    return 0;
                }
            }
            m_grid[base - 7 + i] = static_cast<CGrunt*>(cell);
        }
    }

    ar->Read(m_rowCount, 0x10);
    ar->Read(m_cellFlag, 0xf0);
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
        void* node = 0;
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
            void* node = 0;
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
            void* found = 0;
            void* looked = 0;
            if (MapLookupById(*map, key, found) != 0) {
                looked = found;
            }
            void* obj = (looked != NULL
                         && (static_cast<CGameObject*>(looked))->GetClassId() == CLASSID_SERIALREF)
                            ? looked
                            : 0;
            m_goal = static_cast<CWwdGameObjectA*>(obj);
            if (obj == NULL) {
                return 0;
            }
        }
    }

    {
        i32 key;
        ar->Read(&key, sizeof(key));
        if (key != 0) {
            void* found = 0;
            void* looked = 0;
            if (MapLookupById(*map, key, found) != 0) {
                looked = found;
            }
            if (looked == NULL) {
                return 0;
            }
            CWarlord* obj =
                static_cast<CWarlord*>((static_cast<CGameObject*>(looked))->m_animWorker->m_logic);
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
        void* found = 0;
        void* looked = 0;
        if (MapLookupById(*map, key, found) != 0) {
            looked = found;
        }
        if (looked == NULL) {
            return 0;
        }
        void* obj = (static_cast<CGameObject*>(looked))->m_animWorker->m_logic;
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
    ar->Read(&m_recordPosition, sizeof(m_recordPosition));
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
        i32* rec = static_cast<i32*>(m_recList.GetHead());
        cell = m_grid[rec[0] * TM_GRID_COLS + rec[1]];
    }
    CPlay* world = static_cast<CPlay*>(g_gameReg->m_curState);
    ActionOptionHit kind = ov->HitHover(x, y);
    if (kind == ACTIONOPTION_HIT_PRIMARY) {
        PickupType alt = cell->m_entranceReason;
        if (alt > PICKUP_EQUIPPABLE_LAST) {
            alt = cell->m_toolId;
        }
        if (alt == PICKUP_WAND) {
            g_gameReg->m_cmdGrid->ResetGroup(
                cell->m_lastTilePx.m_x,
                cell->m_lastTilePx.m_y,
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
            g_gameReg->m_cmdGrid
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
            CDDrawWorkerHost* grid = board->m_mainPlane;
            if (tx >= grid->m_wrapW || ty >= grid->m_wrapH) {
                continue;
            }
            i32 col;
            if (pxX < 0x10) {
                col = 0;
            } else if (tx >= grid->m_gridW) {
                col = grid->m_gridW - 1;
            } else {
                col = tx;
            }
            i32 row = (ty >= grid->m_gridH) ? grid->m_gridH - 1 : ty;
            i32 cell = grid->m_tileGrid[grid->m_colOffsets[row] + col];
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
                    CGiantRockLogic* gr = root->m_beginMarker->ScanNeighborhood(tx, ty);
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
                    root->m_beginMarker->DelFromList1(gr);
                    continue;
                }
                if (type != TILEKIND_GAUNTLET_BRICK_A && type != TILEKIND_GAUNTLET_BRICK_B
                    && type != TILEKIND_GAUNTLET_BRICK_C) {
                    continue;
                }
                CTileActionEvent* o = root->m_beginMarker->FindActionByCellKey(ty + (tx << 8));
                if (o->Process(0)) {
                    root->m_beginMarker->DelFromList3(o);
                }
                continue;
            }

            CTileTriggerLogic* lo =
                root->m_beginMarker->FindInLists12(ty + (tx << 8), TRIGID_COVERED_POWERUP_26);
            if (lo != NULL) {
                lo->ApplyMove(type);
                root->m_beginMarker->DelFromList1(lo);
            } else {
                CGruntzMgr* reg = g_gameReg;
                CDDrawWorkerHost* wg = reg->m_world->m_level->m_mainPlane;
                i32 off = wg->m_colOffsets[ty];
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

            CDDrawSubMgrLeafScan* set = m_world->m_soundRegistry;
            if (set->m_emitGate == 0) {

                void* e_ob = 0;
                set->m_cues.Lookup("LEVEL_ROCKBREAK", e_ob);
                LeafCue* e = static_cast<LeafCue*>(e_ob);
                if (e != NULL && g_sndEnabled != 0) {
                    i32 tag = g_sndCueTag;
                    u32 now = g_killCueClock;
                    if (now - e->m_lastPlayTime >= e->m_replayDelay) {
                        e->m_lastPlayTime = now;
                        e->m_sound->ConfigureItem(tag, 0, 0, 0);
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
    i32 xLo = x - r - 7;
    i32 xHi = x + r + 7;
    i32 yLo = y - r - 7;
    i32 yHi = y + r + 7;
    i32 rangeA = m_world->m_level->m_mainPlane->m_gridW - 2;
    i32 rangeB = m_world->m_level->m_mainPlane->m_gridH - 2;

    CGrunt** p = m_grid;
    for (i32 i = 0; i < 4; i++) {
        for (i32 j = 0; j < 15; j++, p++) {
            CGrunt* g = *p;
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
            if (xLo <= hx && xHi >= lx && yLo <= hy && yHi >= ly) {
                switch (tier) {
                    case CUE_DROP:
                        if (g->m_gruntKind != GRUNT_INVULNERABLE) {
                            CellDispatch(i, j, DEATH_DROP, flag);
                        }
                        break;
                    case CUE_EXPLODE:
                        if (g->m_gruntKind != GRUNT_INVULNERABLE) {
                            CellDispatch(i, j, DEATH_EXPLODE, flag);
                        }
                        break;
                    case CUE_SQUASH:
                        if (g->m_gruntKind != GRUNT_INVULNERABLE) {
                            CellDispatch(i, j, DEATH_SQUASH, flag);
                        }
                        break;
                    case CUE_TELEPORT: {
                        if (gx == x && gy == y) {
                            break;
                        }
                        i32 done = 0;
                        do {
                            i32 dx = rangeA == 0 ? rand() & 1 : rand() % rangeA + 1;
                            i32 dy = rangeB == 0 ? rand() & 1 : rand() % rangeB + 1;
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
                                spr->m_animWorker->m_notify(spr);
                                (static_cast<CLightFx*>(spr->m_animWorker->m_logic))
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
                        g->m_combatTimeoutLo = static_cast<i32>(
                            g_buteMgr.GetDwordDef(s_Grunt, s_CombatTimeout, 0x1388)
                        );
                        g->m_combatTimeoutHi = 0;
                        g->m_combatClockLo = g_frameTime;
                        g->m_combatClockHi = 0;
                        CGameObject* spr =
                            g_gameReg->m_world->m_childGroup
                                ->CreateSprite(0, gx, gy, SORTKEY_OVERLAY, "LightFx", 0x40003);
                        spr->m_animWorker->m_notify(spr);
                        (static_cast<CLightFx*>(spr->m_animWorker->m_logic))
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
                        spr->m_animWorker->m_notify(spr);
                        (static_cast<CLightFx*>(spr->m_animWorker->m_logic))
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
                        spr->m_animWorker->m_notify(spr);
                        (static_cast<CLightFx*>(spr->m_animWorker->m_logic))
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
    if (m_pendingFxKind == 0 && world->m_dragEndNotify == 0) {
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
    rect.right = hx + r;
    rect.top = hy - r;
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

        i32 type = g->m_gruntType;
        GruntzPlayer* cfg = &g_gameReg->m_options[type];
        i32 aiType = 0;
        i32 ok = 0;
        i32 radius = 0;

        if (g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
            if (cfg->m_humanControlled == 0) {
                aiType = g_buteMgr.GetInt("Grunt", "RessurectAIType");
                radius = g_buteMgr.GetInt("Grunt", "RessurectAIRadius");
            }
            if (PlaceObject(
                    type,
                    (tx << TILE_SHIFT_PX) + TILE_HALF_PX,
                    (ty << TILE_SHIFT_PX) + TILE_HALF_PX,
                    0x186a0,
                    GRUNT_ENTRANCE_RESURRECT,
                    g->m_placeIndex,
                    0,
                    0,
                    aiType,
                    radius,
                    0,
                    0,
                    0
                )
                != -1) {
                ok = 1;
            }
        } else if (cfg->m_liveGate != 0 && cfg->m_doneFlag == 0 && cfg->m_clearedRound == 0) {
            if (cfg->m_humanControlled != 0) {
                if (PlaceObject(
                        type,
                        (tx << TILE_SHIFT_PX) + TILE_HALF_PX,
                        (ty << TILE_SHIFT_PX) + TILE_HALF_PX,
                        0x186a0,
                        GRUNT_ENTRANCE_RESURRECT,
                        g->m_placeIndex,
                        0,
                        0,
                        0,
                        0,
                        0,
                        0,
                        0
                    )
                    != -1) {
                    ok = 1;
                }
            } else if (cfg->m_battlezConfig.TrySeedSpawnAt(tx, ty) != 0) {
                ok = 1;
            }
        }

        if (ok) {
            g->m_wwdObject->m_flags |= 0x10000;

            m_baseList.RemoveAt(cur);
            CGameObject* spr = g_gameReg->m_world->m_childGroup->CreateSprite(
                0,
                (tx << TILE_SHIFT_PX) + TILE_HALF_PX,
                (ty << TILE_SHIFT_PX) + TILE_HALF_PX,
                SORTKEY_OVERLAY,
                "LightFx",
                0x40003
            );
            spr->m_animWorker->m_notify(spr);
            (static_cast<CLightFx*>(spr->m_animWorker->m_logic))
                ->Activate("GAME_LIGHTING_FLASH", "GAME_FLASH", 8, 1);
        }
    }
    return 1;
}

// @early-stop
RVA(0x0007c110, 0x166)
i32 CTriggerMgr::SpawnGrunt(i32 srcRow, i32 srcCol, i32 dstRow, i32 moveIcon) {
    CGrunt* src = m_grid[srcRow * TM_GRID_COLS + srcCol];
    i32 free = 0;
    CGrunt** p = &m_grid[dstRow * TM_GRID_COLS];
    while (*p != NULL) {
        if (free >= 15) {
            break;
        }
        p++;
        free++;
    }
    if (free >= 15) {
        return 0;
    }
    CGameObject* o = src->m_object;
    i32 sx = (o->m_screenX & ~TILE_MASK_PX) + TILE_HALF_PX;
    i32 sy = (o->m_screenY & ~TILE_MASK_PX) + TILE_HALF_PX;
    PickupType k = src->m_entranceReason;
    if (k > PICKUP_EQUIPPABLE_LAST) {
        k = src->m_toolId;
    }
    PickupType vis = src->m_vehiclePickupType;
    this->CellDispatch(srcRow, srcCol, DEATH_DROP, dstRow);
    CDDrawChildGroup* fac = m_world->m_childGroup;
    CWwdGameObjectA* sprite = fac->CreateSprite(0, sx, sy, 0x186a0, "Grunt", 0x40003);
    if (sprite == NULL) {
        return 0;
    }
    sprite->m_animWorker->m_notify(sprite);

    CGrunt* logic = static_cast<CGrunt*>(sprite->m_animWorker->m_logic);

    if (logic->Place(
            this,
            dstRow,
            free,
            static_cast<PickupType>(moveIcon),
            k,
            vis,
            AI_NONE,
            0,
            0,
            0,
            0,
            GRUNT_ENTRANCE_NONE
        )
        == 0) {
        logic->m_wwdObject->m_flags |= 0x10000;
        return 0;
    }
    m_grid[dstRow * TM_GRID_COLS + free] = logic;
    m_rowCount[dstRow] += 1;
    m_cellFlag[(dstRow * TM_GRID_COLS + free)] = 0;
    return 1;
}

RVA(0x0007c2e0, 0xb5)
i32 CTriggerMgr::CycleMoveIcons(i32 skipRow, i32 enable) {
    i32 r = 0;
    CGrunt** grid = m_grid;
    for (; r < 4; r++, grid += 15) {
        if (r != skipRow) {
            CGrunt** cell = grid;
            i32 i = 15;
            do {
                CGrunt* g = *cell;
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
                cell++;
                i--;
            } while (i != 0);
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
                LeafCue* p = 0;
                MapLookup(m_world->m_soundRegistry->m_cues, "GAME_FINISHLEVEL", p);
                m_timerWindow = static_cast<u32>((p->m_sound->m_durationMs + 500));
                m_timerBase = g_frameTime;
                CDDrawSubMgrLeafScan* h28 = m_world->m_soundRegistry;
                if (h28->m_emitGate == 0) {
                    p = NULL;
                    MapLookup(h28->m_cues, "GAME_FINISHLEVEL", p);
                    if (p != NULL && g_sndEnabled != 0
                        && static_cast<u32>((g_killCueClock - p->m_lastPlayTime))
                               >= static_cast<u32>(p->m_replayDelay)) {
                        p->m_lastPlayTime = g_killCueClock;
                        p->m_sound->ConfigureItem(g_sndCueTag, 0, 0, 0);
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
            if (g_gameReg->m_gameMode == GAMEMODE_SINGLE) {

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
        void* head = g_coordPool.m_freeHead;
        do {
            i32* payload = static_cast<i32*>(m_selLists[idx].GetNext(pos));
            if (payload != NULL) {
                CoordPoolNode* slot = g_coordPool.NodeOf(payload);
                slot->m_next = static_cast<CoordPoolNode*>(head);
                head = slot;
                g_coordPool.m_freeHead = static_cast<CoordPoolNode*>(head);
            }
        } while (pos != NULL);
    }
    CPtrList* sel = &m_selLists[idx];
    sel->RemoveAll();
    pos = m_recList.GetHeadPosition();
    while (pos != NULL) {
        Coord* src = static_cast<Coord*>(m_recList.GetNext(pos));
        CoordPoolNode* fhNode = g_coordPool.m_freeHead;
        Coord* dst = 0;
        if (fhNode->m_next != NULL) {
            dst = &fhNode->m_coord;
            g_coordPool.m_freeHead = fhNode->m_next;
        }
        dst->m_x = src->m_x;
        dst->m_y = src->m_y;
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
        i32* payload = static_cast<i32*>(m_selLists[slot].GetNext(pos));
        i32 idx = payload[1] + TM_GRID_COLS * payload[0];
        CGrunt* cell = m_grid[idx];
        if (cell != NULL) {
            ResetCell(payload[0], payload[1], 1, 0);
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
        i32* k = static_cast<i32*>(m_recList.GetNext(pos));
        CGrunt* cell = m_grid[k[0] * TM_GRID_COLS + k[1]];
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
            i32* head = static_cast<i32*>(m_recList.GetHead());
            cell2 = m_grid[head[0] * TM_GRID_COLS + head[1]];
        }
        if (cell2 != NULL) {
            i32 recX = cell2->m_tileOwnerHi;
            i32 recY = cell2->m_tileOwnerLo;
            if (RecordListHas(recX, recY)) {
                m_recordPosition.m_x = recX;
                m_recordPosition.m_y = recY;
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
                i32* payload = static_cast<i32*>(list->GetNext(pos));
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
i32 CTriggerMgr::ClearRow(i32 row) {
    CGrunt** cell = &m_grid[row * TM_GRID_COLS];
    i32 i = 15;
    do {
        CGrunt* c = *cell;
        if (c != NULL && c->m_deathAnimStarted == 0) {
            (static_cast<CGrunt*>(c))->BuildGruntExitAnimation();
        }
        cell++;
        i--;
    } while (i != 0);
    if (row == g_curPlayer) {
        m_groupFlag = 0;
    }
    (static_cast<CPlay*>(g_gameReg->m_curState))->FlushPendingOps();
    return 1;
}

RVA(0x0007d1d0, 0x9d)
i32 CTriggerMgr::NearestCellDist(i32 skipRow, i32 px, i32 py) {
    i32 tx = px >> TILE_SHIFT_PX;
    i32 ty = py >> TILE_SHIFT_PX;
    i32 best = INT_MAX;
    i32 r = 0;
    CGrunt** row = m_grid;
    do {
        if (r != skipRow) {
            i32 i = 15;
            CGrunt** cell = row;
            do {
                CGrunt* g = *cell;
                if (g != NULL && g->m_entranceCommitted != 0) {
                    CGameObject* o = g->m_object;
                    i32 dx = (o->m_screenX >> TILE_SHIFT_PX) - tx;
                    i32 dy = (o->m_screenY >> TILE_SHIFT_PX) - ty;
                    i32 d = abs(dx * dx + dy * dy);
                    if (d < best) {
                        best = d;
                    }
                }
                cell++;
                i--;
            } while (i != 0);
        }
        r++;
        row += 15;
    } while (r < 4);
    return best;
}

RVA(0x0007d2a0, 0x64)
i32 CTriggerMgr::SelectionListFind(i32 key, i32 y) {
    if (key != g_curPlayer) {
        return 0;
    }
    i32 result = 0;
    CPtrList* list = m_selLists;
    for (i32 i = 0; i < 10; i++, list++) {
        POSITION pos = list->GetHeadPosition();
        while (pos != NULL) {
            i32* payload = static_cast<i32*>(list->GetNext(pos));
            if (payload[0] == key && payload[1] == y) {
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
    CGrunt** cell = m_grid;
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

    CObList& chain = m_world->m_childGroup->m_list;
    POSITION pos = chain.GetHeadPosition();
    while (pos != NULL) {
        CGameObject* obj = static_cast<CGameObject*>(chain.GetNext(pos));
        if (obj != NULL) {
            AnimWorkerObj* desc = obj->m_animWorker;

            NotifyWord slot;
            NotifyWord want;
            slot.m_fn = desc->m_notify;
            want.m_fn = CreateProjectile;
            if (slot.m_bits == want.m_bits) {
                (static_cast<CGrunt*>(desc->m_logic))->m_neighborCell.m_x = 0;
            }
        }
    }

    DirectSoundMgr* ch0 = m_rollingballLoop;
    if (ch0 != NULL) {
        ch0->StopAndRewind();
        m_rollingballLoop = NULL;
    }
    DirectSoundMgr* ch1 = m_teleportLoop;
    if (ch1 != NULL) {
        ch1->StopAndRewind();
        m_teleportLoop = NULL;
    }
    CState* state = g_gameReg->PickPausedThenPlayState();
    if (state != NULL) {
        CStatusBarMgr* sub = (static_cast<CPlay*>(state))->m_guts;
        if (sub != NULL) {
            DirectSoundMgr* ch2 = sub->m_destructButton;
            if (ch2 != NULL) {
                ch2->StopAndRewind();
                sub->m_destructButton = NULL;
            }
        }
    }
}

// @early-stop
// same regalloc spill pair as NotifyCell - retail spills x/y and pushes them from the
// registers. docs/patterns/dead-eight-byte-coord-temp-is-unreproduced.md
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
        i32* rec = static_cast<i32*>(m_recList.GetHead());
        cell = m_grid[rec[1] + rec[0] * TM_GRID_COLS];
    }
    if (cell != NULL && cell->m_tileOwnerHi == g_curPlayer) {
        if ((static_cast<CGrunt*>(cell))->CanShowStamina() == 0) {
            OverlayTick();
        } else {
            PickupType v = cell->m_entranceReason;
            if (v > PICKUP_EQUIPPABLE_LAST) {
                v = cell->m_toolId;
            }
            if (v == PICKUP_WAND) {
                Coord pt = cell->m_lastTilePx;
                g_gameReg->m_cmdGrid
                    ->ResetGroup(pt.m_x, pt.m_y, 0, 0, 0, TARGET_SELECTION_GRUNT, 1);
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

// @early-stop
// 99.73: retail accumulates the grid index into the rec[0]*15 register (edi holds
// rec[1]); operand order is inert, so this is a register-assignment residue.
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
        i32* rec = static_cast<i32*>(m_recList.GetHead());
        cell = m_grid[rec[1] + rec[0] * TM_GRID_COLS];
    }
    if (cell != NULL && cell->m_tileOwnerHi == g_curPlayer) {
        if (cell->m_entranceReason >= PICKUP_TOYZ_FIRST) {
            OverlayTick();
        } else {
            PickupType kind = cell->m_vehiclePickupType;
            if (kind == PICKUP_SCROLL) {
                CGameObject* o = cell->m_object;
                g_gameReg->m_cmdGrid
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

            CGrunt* cell = m_grid[p->m_x * TM_GRID_COLS + p->m_y];
            x = static_cast<char>(p->m_x);
            if (cell->m_tileOwnerHi == magic && cell->m_entranceActive == 0) {
                buf[count] = static_cast<u8>(p->m_y);
                count++;
            }
        } while (pos != NULL);
    }
    if (count == 1) {
        g_gameReg->m_cmdSubMgr->EnqueueSingle(
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
        g_gameReg->m_cmdSubMgr
            ->EnqueueMulti(1, x, count, buf, static_cast<char>(IDX(PLAYERCMD_STOP)), 0, 0, 0);
    }
    return 1;
}

RVA(0x00085c50, 0x83)
CTriggerMgr::~CTriggerMgr() {
    Cleanup();
}
