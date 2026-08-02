#include <Gruntz/TriggerMgr.h>

#include <AddrWord.h>
#include <Bute/ButeMgr.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/PixelShift.h>
#include <Dsndmgr/DirectSoundMgr.h>
#include <Gruntz/ActionOptionsMenuBar.h>
#include <Gruntz/BattlezMapConfig.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameObjectFactory.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntPuddle.h>
#include <Gruntz/GruntSpawnConfig.h>
#include <Gruntz/GruntzCmdMgr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LeafCue.h>
#include <Gruntz/LightFx.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/Play.h>
#include <Gruntz/SerialRecords.h>
#include <Gruntz/SoundCue.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/StatusBarMgr.h>
#include <Gruntz/String.h>
#include <Gruntz/TileActionEvent.h>
#include <Gruntz/TileGrid.h>
#include <Gruntz/TileTriggerContainer.h>
#include <Gruntz/TileTriggerLogic.h>
#include <Gruntz/TileTriggerSwitchLogic.h>
#include <Gruntz/TriggerMgrViews.h>
#include <Gruntz/UserLogic.h>
#include <Gruntz/Warlord.h>
#include <Io/FileMem.h>
#include <Utils/MapTyped.h>
#include <Wwd/WwdFile.h>

#include <stdlib.h>

DATA(0x00244ca4)
i32 g_groupSentinel;

DATA(0x0020a5dc)
static const char s_LightFx[] = "LightFx";
DATA(0x0020dd20)
static const char s_GAME_FLASH[] = "GAME_FLASH";
DATA(0x0020dd08)
static const char s_GAME_LIGHTING_FLASH[] = "GAME_LIGHTING_FLASH";
static char s_Grunt[] = "Grunt";
static char s_CombatTimeout[] = "CombatTimeout";

// @early-stop
RVA(0x00077f80, 0xab)
CGrunt* CTriggerMgr::FindNearestInRow(CGrunt* g) {
    i32 tx = g->m_lastTilePx.m_x >> 5;
    i32 rowIdx = g->m_tileOwnerHi;
    CGrunt** cell = &m_grid[rowIdx * TM_GRID_COLS];
    i32 ty = g->m_lastTilePx.m_y >> 5;
    CGrunt* best = 0;
    i32 bestDist = 0x7fffffff;
    i32 i = 15;
    do {
        CGrunt* c = *cell;
        if (c != 0) {
            CGameObject* o = c->m_object;
            i32 dx = (o->m_screenX >> 5) - tx;
            i32 dy = (o->m_screenY >> 5) - ty;
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
    r.left += view->m_mainPlane->m_viewRect.left - view->m_planeCtx.left;
    r.top += view->m_mainPlane->m_viewRect.top - view->m_planeCtx.top;
    r.right += view->m_mainPlane->m_viewRect.left - view->m_planeCtx.left;
    r.bottom += view->m_mainPlane->m_viewRect.top - view->m_planeCtx.top;
    for (i32 i = 0; i < 4; i++) {
        for (i32 j = 0; j < 15; j++) {
            CGrunt* g = m_grid[j];
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
            while (pos != 0) {
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
    if (pos == 0) {
        return 0;
    }
    POSITION cur;
    i32* p;
    do {
        cur = pos;
        p = static_cast<i32*>(m_recList.GetNext(pos));
        if (p[0] == x && p[1] == y) {
            goto found;
        }
    } while (pos != 0);
    return 0;
found:
    if (m_recList.GetCount() == 1) {
        StopPendingFx();
    }
    CGrunt* cell = m_grid[y + x * TM_GRID_COLS];
    if (cell != 0) {
        (static_cast<CGrunt*>(cell))->ClearAllSprites();
    }
    if (m_recordPosition.m_x == p[0] && m_recordPosition.m_y == p[1]) {
        CWwdGameObjectA* goal = m_goal;
        if (goal != 0) {
            goal->m_flags |= 0x10000;
            m_goal = 0;
        }
        m_armed = 0;
    }
    CActionOptionsMenuBar* ov = m_overlay;
    if (ov != 0 && ov->m_gridX == p[0] && ov->m_gridY == p[1]) {
        OverlayTick();
    }
    CoordPoolNode* slot = g_coordPool.NodeOf(p);
    slot->m_next = g_coordPool.m_freeHead;
    g_coordPool.m_freeHead = slot;
    m_recList.RemoveAt(cur);
    return 1;
}

RVA(0x00078430, 0x7f)
void CTriggerMgr::ResetAll() {
    POSITION pos = m_recList.GetHeadPosition();
    while (pos != 0) {
        i32* payload = static_cast<i32*>(m_recList.GetNext(pos));
        i32 idx = payload[1] + TM_GRID_COLS * payload[0];
        CGrunt* cell = m_grid[idx];
        if (cell != 0) {
            (static_cast<CGrunt*>(cell))->ClearAllSprites();
            CoordPoolNode* slot = g_coordPool.NodeOf(payload);
            slot->m_next = g_coordPool.m_freeHead;
            g_coordPool.m_freeHead = slot;
        }
    }
    m_recList.RemoveAll();
    StopPendingFx();
    CWwdGameObjectA* goal = m_goal;
    if (goal != 0) {
        goal->m_flags |= 0x10000;
        m_goal = 0;
    }
}

RVA(0x000784d0, 0x3a)
i32 CTriggerMgr::RecordListHas(i32 x, i32 y) {
    POSITION pos = m_recList.GetHeadPosition();
    while (pos != 0) {
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
    u8 firstByte = 0;
    u8 bytes[0x70];
    POSITION pos = m_recList.GetHeadPosition();
    while (pos != 0) {
        i32* payload = static_cast<i32*>(m_recList.GetNext(pos));
        firstByte = static_cast<u8>(payload[0]);
        CGrunt* cell = m_grid[payload[1] + payload[0] * TM_GRID_COLS];
        if (cell->m_tileOwnerHi == g_curPlayer && cell->m_entranceActive == 0) {
            bytes[count] = static_cast<u8>(payload[1]);
            count++;
        }
    }
    CGruntzCmdMgr* rep = g_gameReg->m_cmdSubMgr;
    if (count == 1) {
        rep->EnqueueSingle(
            tag,
            firstByte,
            bytes[0],
            2,
            static_cast<i16>(gx),
            static_cast<i16>(gy),
            0,
            0
        );
    } else {
        rep->EnqueueMulti(
            tag,
            firstByte,
            count,
            bytes,
            2,
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
    u8 firstByte = 0;
    u8 bytes[0x70];
    POSITION pos = m_recList.GetHeadPosition();
    while (pos != 0) {
        i32* payload = static_cast<i32*>(m_recList.GetNext(pos));
        firstByte = static_cast<u8>(payload[0]);
        CGrunt* cell = m_grid[payload[1] + payload[0] * TM_GRID_COLS];
        if (cell->m_tileOwnerHi == g_curPlayer && cell->m_entranceActive == 0) {
            bytes[count] = static_cast<u8>(payload[1]);
            count++;
        }
    }
    CGruntzCmdMgr* rep = g_gameReg->m_cmdSubMgr;
    if (count == 1) {
        if (flag != 0) {
            rep->EnqueueSingle(
                tag,
                firstByte,
                bytes[0],
                9,
                static_cast<i16>(gx),
                static_cast<i16>(gy),
                0,
                0
            );
        } else {
            rep->EnqueueSingle(
                tag,
                firstByte,
                bytes[0],
                3,
                static_cast<i16>(gx),
                static_cast<i16>(gy),
                0,
                0
            );
        }
    } else {
        if (flag != 0) {
            rep->EnqueueMulti(
                tag,
                firstByte,
                count,
                bytes,
                9,
                static_cast<i16>(gx),
                static_cast<i16>(gy),
                0
            );
        } else {
            rep->EnqueueMulti(
                tag,
                firstByte,
                count,
                bytes,
                3,
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
    if (pos != 0) {
        do {
            CoordPoolNode* slot = g_coordPool.NodeOf(m_recList.GetNext(pos));
            slot->m_next = g_coordPool.m_freeHead;
            g_coordPool.m_freeHead = slot;
        } while (pos != 0);
    }
    m_recList.RemoveAll();
}

// @early-stop
RVA(0x000788d0, 0x64)
i32 CTriggerMgr::ScrollToActiveRecord() {
    CGameObject* src = m_grid[m_recordPosition.m_x * TM_GRID_COLS + m_recordPosition.m_y]->m_object;
    i32 y = src->m_screenY;
    i32 x = src->m_screenX;
    CDDrawWorkerHost* t = m_world->m_level->m_mainPlane;
    float fy = static_cast<float>(y);
    float fx = static_cast<float>(x);
    if (!(t->m_flags & 1)) {
        fx *= t->m_scaleX;
        fy *= t->m_scaleY;
    }
    t->m_scaledX = fx;
    t->m_scaledY = fy;
    t->RecomputePlaneCoords();
    return 1;
}

RVA(0x00078960, 0x9b)
i32 CTriggerMgr::LoadCameraSprite() {
    if (m_goal != 0) {
        return 0;
    }

    i32 vx = g_gameReg->m_modeW;
    i32 vy = g_gameReg->m_modeH;
    i32 pos = (static_cast<CPlay*>(g_gameReg->m_curState))->m_guts->m_position;

    i32 ax, cx;
    if (pos != 0) {
        if (pos > 0 && pos <= 2) {
            ax = vx - 0x28;
            cx = vy - 0x28;
        }
    } else {
        ax = vx - 0xc8;
        cx = vy - 0x28;
    }

    CDDrawChildGroup* fac = m_world->m_childGroup;
    CWwdGameObjectA* spr = fac->CreateSprite(0, ax, cx, 0xf4240, "DoNothing", 1);
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

RVA(0x00078a50, 0x8a0)
i32 CTriggerMgr::PlaceObjectFull(i32 x, i32 y) {

    CGrunt* cell;
    if (m_recList.GetCount() != 1) {
        cell = 0;
    } else {
        i32* rec = static_cast<i32*>(m_recList.GetHead());
        cell = m_grid[rec[0] * TM_GRID_COLS + rec[1]];
    }
    if (cell == 0) {
        return 1;
    }
    if (cell->m_tileOwnerHi != g_curPlayer) {
        return 1;
    }

    CActionOptionsMenuBar* ov = m_overlay;
    if (ov != 0 && ov->m_active != 0) {
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
    if (CellHitTest(x, y, 0, 0, 5)) {
        hitFlag = 1;
    }

    CGameLevel* view = m_world->m_level;
    CDDrawWorkerHost* grid = view->m_mainPlane;
    i32 tx = x >> 5;
    i32 ty = y >> 5;
    i32 cx = tx;
    if (tx < 0) {
        cx = 0;
    } else if (tx >= grid->m_gridW) {
        cx = grid->m_gridW - 1;
    }
    i32 cy = ty;
    if (ty < 0) {
        cy = 0;
    } else if (ty >= grid->m_gridH) {
        cy = grid->m_gridH - 1;
    }
    i32 collision = 0;
    i32 cval = grid->m_tileGrid[grid->m_colOffsets[cy] + cx];
    if (cval != static_cast<i32>(0xeeeeeeee) && cval != -1) {

        CTileImageSet* tc = static_cast<CTileImageSet*>(view->m_imageSets.GetAt(cval & 0xffff));
        collision = tc->GetCollisionAt(0, 0);
    }

    i32 pfk = m_pendingFxKind;
    if (pfk >= 0xdf) {
        i32 alt = cell->m_vehiclePickupType;
        if (hitFlag != 0) {
            world->LoadCursorSprites(alt + 0xc8, 1);
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
        if ((attr & 0x939) != 0 || (attr & 2) != 0) {
            world->LoadCursorSprites(pfk, 0);
        } else {
            world->LoadCursorSprites(alt + 0xc8, 1);
        }
        return 1;
    }

    i32 gruntKind = cell->m_entranceReason;
    if (gruntKind > 0x16) {
        gruntKind = cell->m_toolId;
    }

    const u16 grey = static_cast<u16>(
        ((0x20 >> g_rDown) << g_rUp) | ((0x20 >> g_gDown) << g_gUp) | (0x20 >> g_bDown)
    );
    const u16 red = static_cast<u16>((0xff >> g_rDown) << g_rUp);

    if (hitFlag != 0) {
        if (pfk == 0) {
            world->LoadCursorSprites(0, 0);
            return 1;
        }
        if (gruntKind != 2 && gruntKind != 9 && gruntKind != 10 && gruntKind != 0xb
            && gruntKind != 0x15 && gruntKind != 0x16) {
            world->LoadCursorSprites(gruntKind + 0xc8, 1);
            return 1;
        }

        POINT source = {cell->m_object->m_screenX, cell->m_object->m_screenY};
        grid->WrapCoord(&source.x, &source.y);
        POINT destination = {x, y};
        grid->WrapCoord(&destination.x, &destination.y);
        i32 blocked = cell->RectContains(x, y);
        world->LoadCursorSprites(blocked ? gruntKind + 0xc8 : pfk, blocked != 0);
        world->m_pathPreviewSource = source;
        world->m_pathPreviewDestination = destination;
        world->m_pathPreviewColor = blocked ? red : grey;
        world->m_drewThisFrame = 1;
        return 1;
    }

    switch (gruntKind) {
        case 1:
            world->LoadCursorSprites(pfk ? gruntKind + 0xc8 : 0, pfk != 0);
            return 1;

        case 2:
        case 9:
        case 10:
        case 0xb:
        case 0x15:
        case 0x16:
            if (pfk != 0) {
                POINT source = {cell->m_object->m_screenX, cell->m_object->m_screenY};
                grid->WrapCoord(&source.x, &source.y);
                POINT destination = {x, y};
                grid->WrapCoord(&destination.x, &destination.y);
                i32 blocked = cell->RectContains(x, y);
                world->LoadCursorSprites(blocked ? gruntKind + 0xc8 : pfk, blocked != 0);
                world->m_pathPreviewSource = source;
                world->m_pathPreviewDestination = destination;
                world->m_pathPreviewColor = blocked ? red : grey;
                world->m_drewThisFrame = 1;
                return 1;
            }
            break;

        case 3:
            if (collision == 0x96 || collision == 0x97 || collision == 0x98) {
                world->LoadCursorSprites(gruntKind + 0xc8, 1);
                return 1;
            }
            break;

        case 5:
            if (collision == 0x1e || collision == 0x1f || collision == 0x21 || collision == 0x97
                || collision == 0x98 || collision == 0x99) {
                world->LoadCursorSprites(gruntKind + 0xc8, 1);
                return 1;
            }
            break;

        case 0xd:
            if (collision == 0x22 || collision == 0x23) {
                world->LoadCursorSprites(gruntKind + 0xc8, 1);
                return 1;
            }
            break;

        case 0xe:
            world->LoadCursorSprites(pfk ? gruntKind + 0xc8 : 0, pfk != 0);
            return 1;

        case 0xf:
            if (pfk != 0 || collision == 0x1e || collision == 0x1f || collision == 0x21
                || collision == 0x97 || collision == 0x98 || collision == 0x99) {
                world->LoadCursorSprites(gruntKind + 0xc8, 1);
                return 1;
            }
            break;

        case 0x11: {
            CGruntzMapMgr* plane = g_gameReg->m_tileGrid;
            i32 attr;
            if (static_cast<u32>(tx) >= static_cast<u32>(plane->m_width)
                || static_cast<u32>(ty) >= static_cast<u32>(plane->m_height)) {
                attr = 1;
            } else {
                attr = plane->m_rowInts[ty][tx * 7];
            }
            if (pfk != 0 && (attr & 0x939) == 0 && (attr & 2) == 0) {
                world->LoadCursorSprites(gruntKind + 0xc8, 1);
                return 1;
            }
            break;
        }

        case 0x14:
            if (g_gameReg->m_gameMode != 1) {
                world->LoadCursorSprites(pfk ? gruntKind + 0xc8 : 0, pfk != 0);
                return 1;
            }
            break;
    }

    world->LoadCursorSprites(pfk, 0);
    return 1;
}

// @early-stop
RVA(0x00079520, 0x2e3)
i32 CTriggerMgr::ResetGroup(
    i32 x,
    i32 y,
    i32 worldX,
    i32 worldY,
    i32 unused5,
    i32 selector,
    i32 spawnCursor
) {
    static_cast<void>(unused5);
    if (m_groupFlag == 0) {
        return 0;
    }
    CGrunt* hit = CellHitTest(x, y, 0, 0, 5);
    CGrunt* cell;
    if (m_recList.GetCount() != 1) {
        cell = 0;
    } else {
        i32* rec = static_cast<i32*>(m_recList.GetHead());
        cell = m_grid[rec[1] + rec[0] * TM_GRID_COLS];
    }

    i32 sel;
    if (cell != 0) {
        if (cell->m_tileOwnerHi != g_curPlayer) {
            return 1;
        }
        if (selector != 0) {
            sel = selector;
        } else if (hit == 0) {
            sel = 1;
        } else if (hit == cell) {
            m_pendingFxKind = 0;
            (static_cast<CPlay*>(g_gameReg->m_curState))->LoadCursorSprites(0, 0);
            CGameObject* o = hit->m_object;

            this->DestroyGroup(o->m_screenX, o->m_screenY, worldX, worldY);
            return 1;
        } else {
            sel = 2;
        }
    } else {
        sel = (hit != 0) ? 2 : 1;
    }

    CGameObject* sprite;
    i32 kindArg;
    switch (sel) {
        case 1:
            this->ReportRecordsA(1, x, y);
            if (spawnCursor == 0) {
                return 1;
            }
            sprite = m_world->m_childGroup->CreateSprite(0, x, y, 0xf4240, "LightFx", 0x40003);
            sprite->m_animWorker->m_notify(sprite);
            kindArg = 2;
            goto arm;
        case 2:
            if (hit == 0) {
                this->ReportRecordsB(1, x, y, 0);
            } else {
                i32 owner = hit->m_tileOwnerHi;
                if (owner == g_curPlayer && g_traitorMode == 0) {
                    if (hit != cell) {
                        goto reportError;
                    }
                    i32 v = (hit->m_entranceReason <= 0x16) ? hit->m_entranceReason : hit->m_toolId;
                    if (v != 0xf) {
                        i32 v2 =
                            (hit->m_entranceReason <= 0x16) ? hit->m_entranceReason : hit->m_toolId;
                        if (v2 != 0x13) {
                            goto reportError;
                        }
                    }
                }
                this->ReportRecordsB(1, owner, hit->m_tileOwnerLo, 1);
            }
            if (spawnCursor == 0) {
                return 1;
            }
            sprite = m_world->m_childGroup->CreateSprite(0, x, y, 0xf4240, "LightFx", 0x40003);
            sprite->m_animWorker->m_notify(sprite);
            kindArg = 1;
            goto arm;
        case 3:
            if (hit != 0) {
                if (hit->m_tileOwnerHi == g_curPlayer && g_traitorMode == 0
                    && (hit != cell || hit->m_vehiclePickupType != 0x1e)) {
                    goto reportError;
                }
                g_gameReg->m_cmdSubMgr->EnqueueSingle(
                    1,
                    static_cast<char>(cell->m_tileOwnerHi),
                    static_cast<char>(cell->m_tileOwnerLo),
                    10,
                    static_cast<i16>(hit->m_tileOwnerHi),
                    static_cast<i16>(hit->m_tileOwnerLo),
                    0,
                    0
                );
            } else {
                g_gameReg->m_cmdSubMgr->EnqueueSingle(
                    1,
                    static_cast<char>(cell->m_tileOwnerHi),
                    static_cast<char>(cell->m_tileOwnerLo),
                    4,
                    static_cast<i16>(x),
                    static_cast<i16>(y),
                    0,
                    0
                );
            }
            if (spawnCursor == 0) {
                return 1;
            }
            sprite = m_world->m_childGroup->CreateSprite(0, x, y, 0xf4240, "LightFx", 0x40003);
            sprite->m_animWorker->m_notify(sprite);
            kindArg = 3;
            goto arm;
        default:
            return 1;
    }

arm:
    (static_cast<CLightFx*>(sprite->m_animWorker->m_logic))
        ->Activate("GAME_LIGHTING_TARGETCURSOR", "GAME_TARGETCURSOR", kindArg, 1);
    return 1;

reportError:
    g_gameReg->m_cueSink->SpawnVoiceDriver(cell, 0x324, -1, 0, -1, -1);
    return 0;
}

// @early-stop
RVA(0x000798d0, 0x1b6)
i32 CTriggerMgr::DestroyGroup(i32 screenX, i32 screenY, i32 worldX, i32 worldY) {
    CActionOptionsMenuBar* ov = m_overlay;
    if (ov == 0) {
        m_overlay = new CActionOptionsMenuBar;
        if (m_overlay->LoadAssets() == 0) {
            CActionOptionsMenuBar* o2 = m_overlay;
            if (o2 != 0) {
                o2->Clear();
                operator delete(o2);
                m_overlay = 0;
            }
            g_gameReg->ReportError(0x800a, 0x3ff);
        }
        return 0;
    }
    if (ov->m_active != 0 || m_recList.GetCount() != 1) {
        return 0;
    }
    i32* rec = static_cast<i32*>(m_recList.GetHead());
    CGrunt* cellp = m_grid[rec[1] + rec[0] * TM_GRID_COLS];
    if (cellp == 0 || cellp->m_tileOwnerHi != g_curPlayer) {
        return 0;
    }
    if (ov->Init(0, 0, screenX, screenY, cellp->m_tileOwnerHi, cellp->m_tileOwnerLo) == 0) {
        return 0;
    }
    CGameLevel* view = m_world->m_level;
    CDDrawWorkerHost* pl = view->m_mainPlane;
    i32 ox = pl->m_viewRect.left - view->m_planeCtx.left + worldX;
    i32 oy = pl->m_viewRect.top - view->m_planeCtx.top + worldY;
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
i32 CTriggerMgr::ByteTableHas(i32 b) {

    i32 n = m_byteArr.GetSize();
    for (i32 i = 0; i < n; i++) {
        if (b == m_byteArr[i]) {
            return 1;
        }
    }
    return 0;
}

// @early-stop
RVA(0x00079b80, 0x194)
i32 CTriggerMgr::ReinitGroup(i32 col, i32 row) {
    if (m_groupInitialized != 0) {
        return 0;
    }
    if (g_gameReg->m_gameMode != 1) {
        return 0;
    }
    CPlay* lvl = static_cast<CPlay*>(g_gameReg->m_curState);
    CString name;
    name.Format("Level%i", lvl->m_levelIndex, 0);
    i32 color =
        g_buteMgr.GetIntDef(const_cast<char*>(static_cast<const char*>(name)), "WarpStone", 0);
    if (row >= g_gameReg->m_viewBounds.right || row < g_gameReg->m_viewBounds.left
        || col >= g_gameReg->m_viewBounds.bottom || col < g_gameReg->m_viewBounds.top) {
        lvl->ResetGoals(row, col);
    }

    CGameLevel* plane = g_gameReg->m_world->m_level;
    LONG outR = col;
    LONG outC = row;
    plane->m_mainPlane->WrapCoord(&outR, &outC);
    CStatusBarMgr* sbi = lvl->m_guts;
    if (sbi->m_hlBusy == 0) {
        if (sbi->m_position == 2) {
            sbi->Reset();
        }
        if (sbi->m_activeTab != 5) {
            sbi->SetTabState(5, 3);
        }
        sbi->SetTab(5, 1);
        sbi->Deactivate();
    }
    if (sbi->EnsureSub(color, outR, outC) != 0) {
        sbi->m_hlBusy = 1;
    } else {
        m_byteArr.InsertAt(m_byteArr.GetSize(), 0, 0);
    }
    m_groupInitialized = 1;
    return 1;
}

RVA(0x00079d90, 0xc5)
void CTriggerMgr::ResetSpawnState() {
    if (g_gameReg->m_gameMode != 1) {
        return;
    }
    if (m_groupInitialized == 0) {
        return;
    }
    CPlay* world = static_cast<CPlay*>(g_gameReg->m_curState);
    CStatusBarMgr* st = world->m_guts;
    if (st->m_retabNotify != 0) {
        operator delete(st->m_retabNotify);
        st->m_retabNotify = 0;
    }
    world->m_guts->m_hlBusy = 0;
    if (m_byteArr.GetSize() > 0) {
        m_byteArr.RemoveAt(m_byteArr.GetSize() - 1, 1);
        CStatusBarMgr* ctx = world->m_guts;
        if (ctx->m_position != kSubtypeTag && ctx->m_activeTab == 5) {
            ctx->ResetWidgets(0);
            world->m_guts->TryActivate();
        }
    }
    if (g_gameReg->m_gameMode == 1) {
        CWarlord* fx = m_pendingFx;
        if (fx != 0) {
            fx->ResolveDeathAnimation();
        }
    }
    this->LoadFinishLevelSprite(6);
}

// @early-stop
RVA(0x00079ea0, 0xc2)
i32 __stdcall SpawnTileFx(i32 x, i32 y, i32 anchorIndex) {
    if (g_gameReg->m_gameMode != 1) {
        return 0;
    }
    CGruntzMapMgr* grid = g_gameReg->m_tileGrid;
    i32 tx = x >> 5;
    i32 ty = y >> 5;
    i32 tile;
    if (static_cast<u32>(tx) >= static_cast<u32>(grid->m_width)
        || static_cast<u32>(ty) >= static_cast<u32>(grid->m_height)) {
        tile = 1;
    } else {
        tile = grid->m_rowInts[ty][tx * 8 - tx];
    }
    if ((tile & 0x40939) == 0 && (tile & 2) == 0) {
        g_gameReg->m_cmdGrid
            ->LoadPowerupIconSprites(0x14, (tx << 5) + 0x10, (ty << 5) + 0x10, 0, anchorIndex, 0);
        return 1;
    }
    CPlay* world = static_cast<CPlay*>(g_gameReg->m_curState);
    i32 idx = anchorIndex - 1;
    CPlay::Anchor* rec = (static_cast<u32>(idx) < 4) ? &world->m_anchors[idx] : 0;
    if (rec != 0) {
        g_gameReg->m_cmdGrid->LoadPowerupIconSprites(0x14, rec->m_x, rec->m_y, 0, anchorIndex, 0);
    }
    return 1;
}

// @early-stop
RVA(0x00079fb0, 0x169)
void CTriggerMgr::NotifyCell(i32 row, i32 col, i32 z) {
    i32 idx = col * TM_GRID_COLS + row;
    CGrunt* cell = m_grid[idx];
    if (cell == 0) {
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
    i32 rowIdx = pt.m_y >> 5;
    i32 cellCol = pt.m_x >> 5;
    tg->m_rows[rowIdx][cellCol].m_flagBytes[3] &= 0xdf;
    tg->m_rows[rowIdx][cellCol].m_occupantId = -1;
    m_grid[idx] = 0;
    m_rowCount[col] -= 1;

    i32 k;
    if (z != 0) {
        m_cellFlag[idx] = 1;
        m_gruntzExitedByPlayer[col] += 1;
        k = cell->m_entranceReason;
        if (k > 0x16) {
            k = cell->m_toolId;
        }
        if (k != 0x14) {
            goto mark;
        }
        if (g_gameReg->m_gameMode == 1) {
            CWarlord* fx = m_pendingFx;
            if (fx != 0) {
                fx->ResolveDeathAnimation();
            }
        }
        this->LoadFinishLevelSprite(1);
        cell->m_cellRemovalNotified = 1;
        return;
    }
    k = cell->m_entranceReason;
    if (k > 0x16) {
        k = cell->m_toolId;
    }
    if (k == 0x14) {
        this->ResetSpawnState();
    }
    m_gruntzLostByPlayer[col] += 1;
mark:
    cell->m_cellRemovalNotified = 1;
}

RVA(0x0007a180, 0x86)
i32 CTriggerMgr::SpawnPuddle(i32 x, i32 y, i32 f124, i32 f114, i32 color, i32 f118) {
    CDDrawChildGroup* fac = m_world->m_childGroup;
    CWwdGameObjectA* sprite = fac->CreateSprite(0, x, y, 0xa, "GruntPuddle", 0x40003);
    if (sprite == 0) {

        g_gameReg->ReportError(0x8009, 0x400);
        return 0;
    }
    sprite->m_animWorker->m_notify(sprite);
    sprite->m_smarts = f124;
    sprite->m_score = f114;
    sprite->m_points = f118;
    return PlacePuddle(sprite, color);
}

// @early-stop
RVA(0x0007a240, 0x143)
i32 CTriggerMgr::PlacePuddle(CGameObject* sprite, i32 color) {
    CGruntPuddle* tgt = static_cast<CGruntPuddle*>(sprite->m_animWorker->m_logic);
    i32 d = sprite->m_points;
    if (d == 0) {
        d = 0x19;
    }
    if (tgt->Place(sprite->m_smarts, sprite->m_score, color, d) == 0) {
        tgt->m_wwdObject->m_flags |= 0x10000;
        g_gameReg->ReportError(0x8009, 0x401);
        return 0;
    }
    POSITION pos = m_baseList.GetHeadPosition();
    i32 manyFlag = (m_baseList.GetCount() > 0x3b) ? 1 : 0;
    i32 unlinked = 0;
    while (pos != 0 && unlinked == 0) {
        POSITION cur = pos;
        CGruntPuddle* o = static_cast<CGruntPuddle*>(m_baseList.GetNext(pos));
        if (o->m_tileX == tgt->m_tileX && o->m_tileY == tgt->m_tileY) {
            if (o->m_pending != 0) {
                tgt->m_wwdObject->m_flags |= 0x10000;
                return 0;
            }
            o->m_wwdObject->m_flags |= 0x10000;
            m_baseList.RemoveAt(cur);
            unlinked = 1;
        }
    }
    if (manyFlag != 0 && unlinked == 0) {
        pos = m_baseList.GetHeadPosition();
        while (pos != 0) {
            POSITION cur = pos;
            CGruntPuddle* o = static_cast<CGruntPuddle*>(m_baseList.GetNext(pos));
            if (o->m_pending == 0) {
                o->m_wwdObject->m_flags |= 0x10000;
                m_baseList.RemoveAt(cur);
            }
        }
    }
    m_baseList.AddTail(tgt);
    return 1;
}

RVA(0x0007a3f0, 0xd7)
i32 CTriggerMgr::LoadToyBoxIcon(i32 x, i32 y, i32 col, i32 kind, i32 moveKind) {
    CDDrawChildGroup* fac = m_world->m_childGroup;
    i32 tx = x >> 5;
    i32 ty = y >> 5;

    POSITION pos = fac->m_list.GetHeadPosition();
    while (pos != 0) {
        CGameObject* obj = static_cast<CGameObject*>(fac->m_list.GetNext(pos));
        GameObjNotifyFn init = obj->m_animWorker->m_notify;
        if (init == CreateInGameIcon || init == CreateInGameText) {
            i32 ox = obj->m_screenX >> 5;
            i32 oy = obj->m_screenY >> 5;
            if (tx == ox && ty == oy) {
                return 0;
            }
        }
    }

    CWwdGameObjectA* spr = fac->CreateSprite(0, x, y, 0x17318, "InGameIcon", 0x40003);
    if (!spr) {
        g_gameReg->ReportError(0x8009, 0x402);
        return 0;
    }
    spr->ApplyName("GAME_TOYBOX");
    spr->m_points = kind;
    spr->m_score = col;
    spr->m_faceDirection = moveKind;
    spr->m_stateFlags |= 1;
    return 1;
}

RVA(0x0007a510, 0x9e)
i32 CTriggerMgr::ClearRowAndRefresh(i32 startRow) {
    i32 row, last;
    if (startRow == 5) {
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
                if (c != 0 && c->m_deathAnimStarted == 0) {
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
i32 CTriggerMgr::Serialize(CFileMemBase* ar, i32 kind, i32, i32) {
    if (ar == 0) {
        return 0;
    }

    if (kind != 4) {
        if (kind == 7) {
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
RVA(0x0007a760, 0x373)
i32 CTriggerMgr::ScanGroup(CFileMemBase* ar) {
    if (ar == 0) {
        return 0;
    }
    CDDrawSurfaceMgr* lvl = m_world;
    if (lvl == 0) {
        return 0;
    }
    CGrunt** cell = m_grid;
    i32 r = 4;
    do {
        i32 c = 15;
        do {
            CGrunt* g = *cell;
            i32 id = 0;
            if (g != 0) {
                id = g->m_object->m_objectId;
                void* found = 0;
                MapLookupById(lvl->m_childGroup->m_map48, id, found);
            }
            ar->Write(&id, 4);
            cell++;
            c--;
        } while (c != 0);
        r--;
    } while (r != 0);
    ar->Write(m_rowCount, 0x10);
    ar->Write(m_cellFlag, 0xf0);
    ar->Write(m_gruntzExitedByPlayer, 0x10);
    ar->Write(m_gruntzLostByPlayer, 0x10);
    i32 cnt = m_byteArr.GetSize();
    ar->Write(&cnt, 4);
    for (i32 i = 0; i < cnt; i++) {
        u8 b = m_byteArr.GetData()[i];
        ar->Write(&b, 1);
    }
    i32 flag24c = m_recList.GetCount();
    ar->Write(&flag24c, 4);
    POSITION pos = m_recList.GetHeadPosition();
    while (pos != 0) {
        ar->Write(m_recList.GetNext(pos), 8);
    }
    CPtrList* list = m_selLists;
    i32 k = 10;
    do {
        i32 cnt2 = list->GetCount();
        ar->Write(&cnt2, 4);
        POSITION selPos = list->GetHeadPosition();
        while (selPos != 0) {
            ar->Write(list->GetNext(selPos), 8);
        }
        list++;
        k--;
    } while (k != 0);
    CWwdGameObjectA* goal = m_goal;
    i32 goalId = 0;
    if (goal != 0) {
        goalId = goal->m_objectId;
    }
    ar->Write(&goalId, 4);
    CWarlord* ov = m_pendingFx;
    i32 ovId = 0;
    if (ov != 0 && ov->m_object != 0) {
        ovId = ov->m_object->m_objectId;
    }
    ar->Write(&ovId, 4);
    ar->Write(m_reserved274, 0x10);
    i32 cntC = m_baseList.GetCount();
    ar->Write(&cntC, 4);
    pos = m_baseList.GetHeadPosition();
    while (pos != 0) {
        CGruntPuddle* obj = static_cast<CGruntPuddle*>(m_baseList.GetNext(pos));
        if (obj == 0) {
            return 0;
        }
        i32 oid = obj->m_object->m_objectId;
        void* found = 0;
        MapLookupById(lvl->m_childGroup->m_map48, oid, found);
        ar->Write(&oid, 4);
    }
    i32 hasOv = (m_overlay != 0) ? 1 : 0;
    ar->Write(&hasOv, 4);
    if (m_overlay != 0) {
        if (m_overlay->Serialize(ar) == 0) {
            return 0;
        }
    } else {
        return 0;
    }
    ar->Write(&m_armed, 4);
    ar->Write(&m_groupInitialized, 4);
    ar->Write(&m_phase, 4);
    ar->Write(&m_recordPosition, 8);
    ar->Write(&m_countdownActive, 4);
    ar->Write(&m_finishReasonFrame, 4);
    ar->Write(&m_groupFlag, 4);
    ar->Write(&g_curPlayer, 4);
    ar->Write(&g_groupSentinel, 4);
    ar->Write(&m_pendingFxKind, 4);
    ar->Write(&m_selSentinel, 4);
    return 1;
}

// @early-stop
RVA(0x0007abc0, 0x4b6)
i32 CTriggerMgr::Load(CFileMemBase* ar) {
    if (ar == 0) {
        return 0;
    }
    if (m_world == 0) {
        return 0;
    }
    m_rollingballLoop = 0;
    m_teleportLoop = 0;
    m_rollingballWanted = 0;
    m_teleportWanted = 0;

    CMapPtrToPtr* map = &m_world->m_childGroup->m_map48;

    for (i32 base = 7; base < 0x43; base += 0xf) {
        for (i32 i = 0; i < 0xf; i++) {
            i32 key;
            ar->Read(&key, 4);
            void* cell = 0;
            if (key != 0) {
                void* found = 0;
                void* looked = MapLookupById(*map, key, found) ? found : 0;
                if (looked == 0) {
                    return 0;
                }
                cell = (static_cast<CGameObject*>(looked))->m_animWorker->m_logic;
                if (cell == 0) {
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
    ar->Read(&count, 4);
    CByteArray* arr = &m_byteArr;
    arr->SetSize(0, -1);
    for (ci = 0; ci < static_cast<u32>(count); ci++) {
        i32 b;
        ar->Read(&b, 1);
        arr->SetAtGrow(ci, b);
    }
    ClearRecords();

    ar->Read(&count, 4);
    CPtrList* rec = &m_recList;
    for (ci = 0; ci < static_cast<u32>(count); ci++) {
        CoordPoolNode* fl = g_coordPool.m_freeHead;
        void* node = 0;
        if (fl->m_next != 0) {
            node = &fl->m_coord;
            g_coordPool.m_freeHead = fl->m_next;
        }
        ar->Read(node, 8);
        rec->AddTail(node);
    }

    CPtrList* sel = m_selLists;
    i32 slot = 0xa;
    do {
        ar->Read(&count, 4);
        for (ci = 0; ci < static_cast<u32>(count); ci++) {
            CoordPoolNode* fl = g_coordPool.m_freeHead;
            void* node = 0;
            if (fl->m_next != 0) {
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
        ar->Read(&key, 4);
        if (key != 0) {
            void* found = 0;
            void* looked = MapLookupById(*map, key, found) ? found : 0;
            void* obj = (looked != 0
                         && (static_cast<CGameObject*>(looked))->GetClassId() == CLASSID_SERIALREF)
                            ? looked
                            : 0;
            m_goal = static_cast<CWwdGameObjectA*>(obj);
            if (obj == 0) {
                return 0;
            }
        }
    }

    {
        i32 key;
        ar->Read(&key, 4);
        if (key != 0) {
            void* found = 0;
            void* looked = MapLookupById(*map, key, found) ? found : 0;
            if (looked == 0) {
                return 0;
            }
            CWarlord* obj =
                static_cast<CWarlord*>((static_cast<CGameObject*>(looked))->m_animWorker->m_logic);
            m_pendingFx = obj;
            if (obj == 0) {
                return 0;
            }
        } else {
            m_pendingFx = 0;
        }
    }

    ar->Read(m_reserved274, 0x10);
    m_baseList.RemoveAll();
    ar->Read(&count, 4);
    for (ci = 0; ci < static_cast<u32>(count); ci++) {
        i32 key;
        ar->Read(&key, 4);
        if (key == 0) {
            return 0;
        }
        void* found = 0;
        void* looked = MapLookupById(*map, key, found) ? found : 0;
        if (looked == 0) {
            return 0;
        }
        void* obj = (static_cast<CGameObject*>(looked))->m_animWorker->m_logic;
        if (obj == 0) {
            return 0;
        }
        m_baseList.AddTail(obj);
    }

    CActionOptionsMenuBar* old = m_overlay;
    if (old != 0) {
        old->Clear();
        ::operator delete(old);
        m_overlay = 0;
    }
    i32 hasOverlay;
    ar->Read(&hasOverlay, 4);
    if (hasOverlay != 0) {
        CActionOptionsMenuBar* ov = new CActionOptionsMenuBar;
        m_overlay = ov;
        if (ov->Deserialize(ar) == 0) {
            return 0;
        }
    }

    ar->Read(&m_armed, 4);
    ar->Read(&m_groupInitialized, 4);
    ar->Read(&m_phase, 4);
    ar->Read(&m_recordPosition, 8);
    ar->Read(&m_countdownActive, 4);
    ar->Read(&m_finishReasonFrame, 4);
    ar->Read(&m_groupFlag, 4);
    ar->Read(&g_curPlayer, 4);
    ar->Read(&g_groupSentinel, 4);
    ar->Read(&m_pendingFxKind, 4);
    ar->Read(&m_selSentinel, 4);
    return 1;
}

// @early-stop
RVA(0x0007b1b0, 0x12b)
i32 CTriggerMgr::TriggerCell(i32 x, i32 y) {
    CActionOptionsMenuBar* ov = m_overlay;
    m_pendingFxKind = 0;
    if (ov == 0 || ov->m_active == 0) {
        return 0;
    }
    CGrunt* cell;
    if (m_recList.GetCount() != 1) {
        cell = 0;
    } else {
        i32* rec = static_cast<i32*>(m_recList.GetHead());
        cell = m_grid[rec[1] + rec[0] * TM_GRID_COLS];
    }
    CPlay* world = static_cast<CPlay*>(g_gameReg->m_curState);
    i32 kind = ov->HitHover(x, y);
    if (kind == 2) {
        i32 alt = cell->m_entranceReason;
        if (alt > 0x16) {
            alt = cell->m_toolId;
        }
        if (alt == 0x13) {
            g_gameReg->m_cmdGrid
                ->ResetGroup(cell->m_lastTilePx.m_x, cell->m_lastTilePx.m_y, 0, 0, 0, 2, 1);
        }
    } else if (kind == 3) {

        i32 alt = cell->m_vehiclePickupType;
        if (alt == 0x1e) {
            CGameObject* o = cell->m_object;
            g_gameReg->m_cmdGrid->ResetGroup(o->m_screenX, o->m_screenY, 0, 0, 0, 3, 1);
        } else if (alt != 0) {
            i32 v = alt + kPendingFxIdBase;
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
    return spr != 0;
}

// @early-stop
RVA(0x0007b440, 0x3f0)
i32 CTriggerMgr::BuildRockBreakParticles(i32 cx, i32 cy, i32 r, i32 flag) {
    CombatCue(cx, cy, r, 6, flag);

    CPlay* root = static_cast<CPlay*>(g_gameReg->m_curState);
    i32 tileCx = cx >> 5;
    i32 tileCy = cy >> 5;
    i32 hiX = tileCx + r;
    for (i32 tx = tileCx - r; tx <= hiX; tx++) {
        i32 pxX = (tx << 5) + 0x10;
        for (i32 ty = tileCy - r; ty <= tileCy + r; ty++) {
            i32 pxY = (ty << 5) + 0x10;
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
            i32 type;
            if (cell == static_cast<i32>(0xeeeeeeee) || cell == -1) {
                type = 0;
            } else {
                CTileImageSet* o =
                    static_cast<CTileImageSet*>(board->m_imageSets.GetAt(cell & 0xffff));
                type = o->GetCollisionAt(0, 0);
            }

            if (type != 0x1e && type != 0x1f) {
                if (type == 0x21) {
                    CGiantRockLogic* gr = root->m_beginMarker->ScanNeighborhood(tx, ty);
                    if (gr == 0) {
                        CString msg;
                        msg.Format("No giant rock logic found around: x=%d, y=%d", cx, cy);
                        g_gameReg->EnterModalUI(msg);
                        g_gameReg->ReportError(TRIGERR_LOOKUP_MISS, TRIGSITE_ROCK_SCAN_MISS);
                        return 0;
                    }
                    gr->BuildRockBreakInGameText();
                    root->m_beginMarker->DelFromList1(gr);
                    continue;
                }
                if (type != 0x97 && type != 0x98 && type != 0x99) {
                    continue;
                }
                CTileActionEvent* o = root->m_beginMarker->FindActionByCellKey(ty + (tx << 8));
                if (o->Process(0)) {
                    root->m_beginMarker->DelFromList3(o);
                }
                continue;
            }

            CTileTriggerLogic* lo = root->m_beginMarker->FindInLists12(ty + (tx << 8), 0x1a);
            if (lo != 0) {
                lo->ApplyMove(type);
                root->m_beginMarker->DelFromList1(lo);
            } else {
                CDDrawWorkerHost* wg = g_gameReg->m_world->m_level->m_mainPlane;
                i32 off = wg->m_colOffsets[ty];
                if (type == 0x1e) {
                    wg->m_tileGrid[off + tx] = 0x5a;
                    (g_gameReg->m_tileGrid)->ComputeCellFlags(tx, ty, 0x5a);
                } else {
                    wg->m_tileGrid[off + tx] = 0x5b;
                    (g_gameReg->m_tileGrid)->ComputeCellFlags(tx, ty, 0x5b);
                }
            }

            POINT pt;
            pt.x = pxX;
            pt.y = pxY;
            if (!PtInRect(&g_gameReg->m_viewBounds, pt)) {
                continue;
            }
            CWwdGameObjectA* spr =
                m_world->m_childGroup->CreateSprite(0, pxX, pxY, 0xcf84f, "Particlez", 0x40003);
            if (spr == 0) {
                continue;
            }
            spr->ApplyName("LEVEL_ROCKBREAK");
            spr->ApplyLookupGeometry("LEVEL_ROCKBREAK", 0);

            CDDrawSubMgrLeafScan* set = m_world->m_soundRegistry;
            if (set->m_emitGate == 0) {

                void* e_ob = 0;
                set->m_cues.Lookup("LEVEL_ROCKBREAK", e_ob);
                LeafCue* e = static_cast<LeafCue*>(e_ob);
                if (e != 0 && g_sndEnabled != 0) {
                    u32 now = g_killCueClock;
                    if (now - e->m_lastPlayTime >= e->m_replayDelay) {
                        e->m_lastPlayTime = now;
                        e->m_sound->ConfigureItem(g_sndCueTag, 0, 0, 0);
                    }
                }
            }
        }
    }
    return 1;
}

// @early-stop
RVA(0x0007b930, 0x3e0)
i32 CTriggerMgr::CombatCue(i32 x, i32 y, i32 radius, i32 tier, i32 flag) {
    i32 r = radius << 5;
    i32 xLo = x - r - 7;
    i32 yLo = y - r - 7;
    i32 xHi = x + r + 7;
    i32 yHi = y + r + 7;
    i32 rangeA = m_world->m_level->m_mainPlane->m_gridW - 2;
    i32 rangeB = m_world->m_level->m_mainPlane->m_gridH - 2;

    CGrunt** p = m_grid;
    for (i32 i = 0; i < 4; i++) {
        for (i32 j = 0; j < 15; j++, p++) {
            CGrunt* g = *p;
            if (g == 0) {
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
                    case 1:
                        if (g->m_gruntKind != 0x38) {
                            CellDispatch(i, j, 0, flag);
                        }
                        break;
                    case 6:
                        if (g->m_gruntKind != 0x38) {
                            CellDispatch(i, j, 0xb, flag);
                        }
                        break;
                    case 7:
                        if (g->m_gruntKind != 0x38) {
                            CellDispatch(i, j, 2, flag);
                        }
                        break;
                    case 2: {
                        if (gx == x && gy == y) {
                            break;
                        }
                        i32 done = 0;
                        do {
                            i32 dx = rangeA ? rand() % rangeA + 1 : rand() & 1;
                            i32 dy = rangeB ? rand() % rangeB + 1 : rand() & 1;
                            if (g->TryTeleportToCell(dx, dy, 0, 1)) {
                                CGameObject* spr =
                                    g_gameReg->m_world->m_childGroup
                                        ->CreateSprite(0, gx, gy, 0xf4240, s_LightFx, 0x40003);
                                done = 1;
                                spr->m_animWorker->m_notify(spr);
                                (static_cast<CLightFx*>(spr->m_animWorker->m_logic))
                                    ->Activate(s_GAME_LIGHTING_FLASH, s_GAME_FLASH, 3, 1);
                            }
                        } while (done == 0);
                        break;
                    }
                    case 3: {
                        if (gx == x && gy == y) {
                            break;
                        }
                        g->m_health = 0x64;
                        g->CreateHealthSprite();
                        g->m_combatTimeoutLo =
                            g_buteMgr.GetIntDef(s_Grunt, s_CombatTimeout, 0x1388);
                        g->m_combatTimeoutHi = 0;
                        g->m_combatClockLo = g_frameTime;
                        g->m_combatClockHi = 0;
                        CGameObject* spr =
                            g_gameReg->m_world->m_childGroup
                                ->CreateSprite(0, gx, gy, 0xf4240, s_LightFx, 0x40003);
                        spr->m_animWorker->m_notify(spr);
                        (static_cast<CLightFx*>(spr->m_animWorker->m_logic))
                            ->Activate(s_GAME_LIGHTING_FLASH, s_GAME_FLASH, 2, 1);
                        break;
                    }
                    case 5: {
                        if (gx == x && gy == y) {
                            break;
                        }
                        i32 toy = rand() % 9 + 0x17;
                        if (toy == 0x1e) {
                            toy = 0x20;
                        }
                        g->LoadGruntTypeTable(toy, 1, 0, 0);
                        CGameObject* spr =
                            g_gameReg->m_world->m_childGroup
                                ->CreateSprite(0, gx, gy, 0xf4240, s_LightFx, 0x40003);
                        spr->m_animWorker->m_notify(spr);
                        (static_cast<CLightFx*>(spr->m_animWorker->m_logic))
                            ->Activate(s_GAME_LIGHTING_FLASH, s_GAME_FLASH, 7, 1);
                        break;
                    }
                    case 4: {
                        if (gx == x && gy == y) {
                            break;
                        }
                        CGameObject* h = g->m_object;
                        CWwdGameObjectA* spr = g_gameReg->m_world->m_childGroup->CreateSprite(
                            0,
                            h->m_screenX,
                            h->m_screenY,
                            0xf4240,
                            s_LightFx,
                            0x40003
                        );
                        spr->m_animWorker->m_notify(spr);
                        (static_cast<CLightFx*>(spr->m_animWorker->m_logic))
                            ->Activate(s_GAME_LIGHTING_FLASH, s_GAME_FLASH, 9, 1);
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
    i32 hx = cx >> 5;
    i32 hy = cy >> 5;
    rect.left = hx - r;
    rect.right = hx + r;
    rect.top = hy - r;
    rect.bottom = hy + r;

    POSITION pos = m_baseList.GetHeadPosition();
    while (pos != 0) {
        POSITION cur = pos;
        CGruntPuddle* g = static_cast<CGruntPuddle*>(m_baseList.GetNext(pos));
        if (g->m_pending != 0) {
            continue;
        }
        POINT pt;
        pt.x = g->m_tileX;
        pt.y = g->m_tileY;
        if (!PtInRect(&rect, pt)) {
            continue;
        }

        i32 type = g->m_gruntType;
        GruntzPlayer* cfg = &g_gameReg->m_options[type];
        i32 aiType = 0;
        i32 ok = 0;
        i32 px = (g->m_tileX << 5) + 0x10;
        i32 py = (g->m_tileY << 5) + 0x10;

        if (g_gameReg->m_gameMode == 1) {
            i32 radius = 0;
            if (cfg->m_humanControlled == 0) {
                aiType = g_buteMgr.GetInt("Grunt", "RessurectAIType");
                radius = g_buteMgr.GetInt("Grunt", "RessurectAIRadius");
            }
            if (PlaceObject(
                    type,
                    px,
                    py,
                    0x186a0,
                    3,
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
                if (PlaceObject(type, px, py, 0x186a0, 3, g->m_placeIndex, 0, 0, 0, 0, 0, 0, 0)
                    != -1) {
                    ok = 1;
                }
            } else if (cfg->m_battlezConfig.TrySeedSpawnAt(g->m_tileX, g->m_tileY) != 0) {
                ok = 1;
            }
        }

        if (ok) {
            g->m_wwdObject->m_flags |= 0x10000;

            m_baseList.RemoveAt(cur);
            CGameObject* spr = g_gameReg->m_world->m_childGroup
                                   ->CreateSprite(0, px, py, 0xf4240, "LightFx", 0x40003);
            spr->m_animWorker->m_notify(spr);
            (static_cast<CLightFx*>(spr->m_animWorker->m_logic))
                ->Activate("GAME_LIGHTING_FLASH", "GAME_FLASH", 8, 1);
        }
    }
    return 1;
}

// @early-stop
RVA(0x0007c110, 0x166)
i32 CTriggerMgr::SpawnGrunt(i32 col, i32 row, i32 a18, i32 a1c) {
    CGrunt* src = m_grid[col * TM_GRID_COLS + a1c];
    i32 free = 0;
    CGrunt** rowBase = &m_grid[row * TM_GRID_COLS];
    if (*rowBase != 0) {
        CGrunt** p = rowBase;
        while (free < 15 && *p != 0) {
            p++;
            free++;
        }
    }
    if (free >= 15) {
        return 0;
    }
    CGameObject* o = src->m_object;
    i32 sx = (o->m_screenX & ~0x1f) + 0x10;
    i32 sy = (o->m_screenY & ~0x1f) + 0x10;
    i32 k = src->m_entranceReason;
    if (k > 0x16) {
        k = src->m_toolId;
    }
    i32 vis = src->m_vehiclePickupType;
    this->CellDispatch(col, row, 0, a18);
    CDDrawChildGroup* fac = m_world->m_childGroup;
    CWwdGameObjectA* sprite = fac->CreateSprite(0, sx, sy, 0x186a0, "Grunt", 0x40003);
    if (sprite == 0) {
        return 0;
    }
    sprite->m_animWorker->m_notify(sprite);

    CGrunt* logic = static_cast<CGrunt*>(sprite->m_animWorker->m_logic);

    if (logic->Place(this, row, free, vis, k, 0, 0, 0, 0, 0, 0, 0) == 0) {
        logic->m_wwdObject->m_flags |= 0x10000;
        return 0;
    }
    m_grid[row * TM_GRID_COLS + free] = logic;
    m_rowCount[row] += 1;
    m_cellFlag[(row * TM_GRID_COLS + free)] = 0;
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
                if (g != 0) {
                    if (enable != 0) {
                        i32 t = rand() % 0x11;
                        if (g->m_savedMoveIcon == -1) {
                            g->m_savedMoveIcon = g->m_moveIcon;
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
void CTriggerMgr::LoadFinishLevelSprite(i32 state) {
    switch (state) {
        case 1:
            if (m_phase != 2) {
                LeafCue* p = 0;
                MapLookup(m_world->m_soundRegistry->m_cues, "GAME\\FINISHLEVEL", p);
                m_timerWindow = static_cast<u32>((p->m_sound->m_durationMs + 500));
                m_timerBase = g_frameTime;
                CDDrawSubMgrLeafScan* h28 = m_world->m_soundRegistry;
                if (h28->m_emitGate == 0) {
                    p = 0;
                    MapLookup(h28->m_cues, "GAME\\FINISHLEVEL", p);
                    if (p != 0 && g_sndEnabled != 0
                        && static_cast<u32>((g_killCueClock - p->m_lastPlayTime))
                               >= static_cast<u32>(p->m_replayDelay)) {
                        p->m_lastPlayTime = g_killCueClock;
                        p->m_sound->ConfigureItem(g_sndCueTag, 0, 0, 0);
                    }
                }
                m_phase = 1;
                m_groupFlag = 0;
                m_finishReasonFrame = state;
                return;
            }
            goto Lab_56b;
        case 2:
            m_phase = 1;
            break;
        case 3:
            if (m_phase == 0) {
                m_phase = 2;
                if (m_pendingFx != 0) {
                    m_pendingFx->ResolveDeathAnimation();
                }
            }
            goto Lab_522;
        case 4:
            m_phase = 2;
            m_timerWindow = 3000;
            m_timerBase = g_frameTime;
            goto Lab_56b;
        case 5:
            m_phase = 2;
            break;
        case 6:
            m_phase = 2;
        Lab_522:
            m_timerWindow = 3000;
            m_timerBase = g_frameTime;
            goto Lab_56b;
        default:
            return;
    }
    m_timerWindow = 3000;
    m_timerBase = g_frameTime;
Lab_56b:
    m_groupFlag = 0;
    m_finishReasonFrame = state;
}

RVA(0x0007c620, 0x4f7)
i32 CTriggerMgr::LoadPowerupIconSprites(
    i32 type,
    i32 geoB,
    i32 geoA,
    i32 m130,
    i32 warpIdx,
    i32 m120
) {
    if (type == 0) {
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
            if (g_gameReg->m_gameMode == 1) {

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
        case PICKUP_REACTIVEARMOR:
            name = "GAME_INGAMEICONZ_POWERUPZ_REACTIVEARMOR";
            break;
        case PICKUP_ROIDZ:
            name = "GAME_INGAMEICONZ_POWERUPZ_ROIDZ";
            break;
        case PICKUP_INVULNERABILITY:
            name = "GAME_INGAMEICONZ_POWERUPZ_INVULNERABILITY";
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
            return tb != 0;
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
    CPtrList* sel = &m_selLists[idx];
    POSITION pos = m_selLists[idx].GetHeadPosition();
    if (pos != 0) {
        void* head = g_coordPool.m_freeHead;
        do {
            i32* payload = static_cast<i32*>(m_selLists[idx].GetNext(pos));
            if (payload != 0) {
                CoordPoolNode* slot = g_coordPool.NodeOf(payload);
                slot->m_next = static_cast<CoordPoolNode*>(head);
                head = slot;
                g_coordPool.m_freeHead = static_cast<CoordPoolNode*>(head);
            }
        } while (pos != 0);
    }
    sel->RemoveAll();
    pos = m_recList.GetHeadPosition();
    while (pos != 0) {
        Coord* src = static_cast<Coord*>(m_recList.GetNext(pos));
        CoordPoolNode* fhNode = g_coordPool.m_freeHead;
        Coord* dst = 0;
        if (fhNode->m_next != 0) {
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
    if (ov != 0 && ov->m_active != 0) {
        OverlayTick();
    }
    POSITION pos = m_selLists[slot].GetHeadPosition();
    if (pos == 0) {
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
        if (cell != 0) {
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
    } while (pos != 0);
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
    if (pos == 0) {
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
        if (cell != 0) {
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
    } while (pos != 0);
    i32 cy = bbox.top + (bbox.bottom - bbox.top) / 2;
    i32 cx = bbox.left + (bbox.right - bbox.left) / 2;
    (static_cast<CPlay*>(g_gameReg->m_curState))->ResetGoals(cx, cy);
    if (doSelect != 0 && count == 1) {
        CGrunt* cell2;
        if (m_recList.GetCount() != 1) {
            cell2 = 0;
        } else {
            i32* head = static_cast<i32*>(m_recList.GetHead());
            cell2 = m_grid[head[0] * TM_GRID_COLS + head[1]];
        }
        if (cell2 != 0) {
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
        if (pos != 0) {
            do {
                i32* payload = static_cast<i32*>(list->GetNext(pos));
                if (payload != 0) {
                    CoordPoolNode* slot = g_coordPool.NodeOf(payload);
                    slot->m_next = g_coordPool.m_freeHead;
                    g_coordPool.m_freeHead = slot;
                }
            } while (pos != 0);
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
        if (c != 0 && c->m_deathAnimStarted == 0) {
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
    i32 tx = px >> 5;
    i32 ty = py >> 5;
    i32 best = 0x7fffffff;
    i32 r = 0;
    CGrunt** row = m_grid;
    do {
        if (r != skipRow) {
            i32 i = 15;
            CGrunt** cell = row;
            do {
                CGrunt* g = *cell;
                if (g != 0 && g->m_entranceCommitted != 0) {
                    CGameObject* o = g->m_object;
                    i32 dx = (o->m_screenX >> 5) - tx;
                    i32 dy = (o->m_screenY >> 5) - ty;
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

// @early-stop
RVA(0x0007d2a0, 0x64)
i32 CTriggerMgr::SelectionListFind(i32 key, i32 y) {
    if (key != g_curPlayer) {
        return 0;
    }
    i32 result = 0;
    i32 i = 0;
    CPtrList* list = m_selLists;
    do {
        POSITION pos = list->GetHeadPosition();
        while (pos != 0) {
            i32* payload = static_cast<i32*>(list->GetNext(pos));
            if (payload[0] == key && payload[1] == y) {
                if (result != 0) {
                    return 10;
                }
                result = i;
            }
        }
        i++;
        list++;
    } while (i < 10);
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
            if (g != 0) {
                (static_cast<CGrunt*>(g))->DestroyAnims();
            }
            cell++;
            i--;
        } while (i != 0);
        r--;
    } while (r != 0);

    CObList& chain = m_world->m_childGroup->m_list;
    POSITION pos = chain.GetHeadPosition();
    while (pos != 0) {
        CGameObject* obj = static_cast<CGameObject*>(chain.GetNext(pos));
        if (obj != 0) {
            AnimWorkerObj* desc = obj->m_animWorker;

            NotifyWord slot;
            NotifyWord want;
            slot.m_fn = desc->m_notify;
            want.m_method = &CGrunt::ReadConfigFromButeMgr;
            if (slot.m_bits == want.m_bits) {
                (static_cast<CGrunt*>(desc->m_logic))->m_neighborCell.m_x = 0;
            }
        }
    }

    DirectSoundMgr* ch0 = m_rollingballLoop;
    if (ch0 != 0) {
        ch0->StopAndRewind();
        m_rollingballLoop = 0;
    }
    DirectSoundMgr* ch1 = m_teleportLoop;
    if (ch1 != 0) {
        ch1->StopAndRewind();
        m_teleportLoop = 0;
    }
    CState* state = g_gameReg->PickPausedThenPlayState();
    if (state != 0) {
        CStatusBarMgr* sub = (static_cast<CPlay*>(state))->m_guts;
        if (sub != 0) {
            DirectSoundMgr* ch2 = sub->m_destructButton;
            if (ch2 != 0) {
                ch2->StopAndRewind();
                sub->m_destructButton = 0;
            }
        }
    }
}

// @early-stop
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
        cell = 0;
    } else {
        i32* rec = static_cast<i32*>(m_recList.GetHead());
        cell = m_grid[rec[0] * TM_GRID_COLS + rec[1]];
    }
    if (cell == 0) {
        return 1;
    }
    if (cell->m_tileOwnerHi != g_curPlayer) {
        return 1;
    }
    if ((static_cast<CGrunt*>(cell))->CanShowStamina() == 0) {
        OverlayTick();
        return 1;
    }
    i32 v = cell->m_entranceReason;
    if (v > 0x16) {
        v = cell->m_toolId;
    }
    if (v == 0x13) {
        Coord pt;
        pt.m_x = cell->m_lastTilePx.m_x;
        pt.m_y = cell->m_lastTilePx.m_y;
        g_gameReg->m_cmdGrid->ResetGroup(pt.m_x, pt.m_y, 0, 0, 0, 2, 1);
        OverlayTick();
        return 1;
    }
    m_pendingFxKind = v + kPendingFxIdBase;
    (static_cast<CPlay*>(g_gameReg->m_curState))->LoadCursorSprites(v + kPendingFxIdBase, 0);
    OverlayTick();
    return 1;
}

// @early-stop
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
        cell = 0;
    } else {
        i32* rec = static_cast<i32*>(m_recList.GetHead());
        cell = m_grid[rec[0] * TM_GRID_COLS + rec[1]];
    }
    if (cell == 0) {
        return 1;
    }
    if (cell->m_tileOwnerHi != g_curPlayer) {
        return 1;
    }
    if (cell->m_entranceReason >= 0x17) {
        OverlayTick();
        return 1;
    }
    i32 kind = cell->m_vehiclePickupType;
    if (kind == 0x1e) {
        CGameObject* o = cell->m_object;
        g_gameReg->m_cmdGrid->ResetGroup(o->m_screenX, o->m_screenY, 0, 0, 0, 3, 1);
        OverlayTick();
        return 1;
    }
    if (kind == 0) {
        OverlayTick();
        return 1;
    }
    m_pendingFxKind = kind + kPendingFxIdBase;
    (static_cast<CPlay*>(g_gameReg->m_curState))->LoadCursorSprites(kind + kPendingFxIdBase, 0);
    OverlayTick();
    return 1;
}

// @early-stop
RVA(0x0007d6e0, 0xea)
i32 CTriggerMgr::EnqueueGroupCells() {
    if (m_groupFlag == 0) {
        return 0;
    }

    u8 buf[0x80];
    u8 count = 0;
    char x;
    POSITION pos = m_recList.GetHeadPosition();
    if (pos != 0) {
        i32 magic = g_curPlayer;
        do {
            Coord* p = static_cast<Coord*>(m_recList.GetNext(pos));

            CGrunt* cell = m_grid[p->m_x * TM_GRID_COLS + p->m_y];
            x = static_cast<char>(p->m_x);
            if (cell->m_tileOwnerHi == magic && cell->m_entranceActive == 0) {
                buf[count] = static_cast<u8>(p->m_y);
                count++;
            }
        } while (pos != 0);
    }
    if (count == 1) {
        g_gameReg->m_cmdSubMgr->EnqueueSingle(1, x, static_cast<char>(buf[0]), 5, 0, 0, 0, 0);
    } else {
        g_gameReg->m_cmdSubMgr->EnqueueMulti(1, x, count, buf, 5, 0, 0, 0);
    }
    return 1;
}

RVA(0x00085c50, 0x83)
CTriggerMgr::~CTriggerMgr() {
    Cleanup();
}
