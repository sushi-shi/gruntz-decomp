#include <rva.h>

#include <Mfc.h>

#include <AddrWord.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <Gruntz/ChatBoxOwner.h>
#include <Gruntz/FreeNodePool.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameModeId.h>
#include <Gruntz/GameObjectFactory.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzCmdMgr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/ImageSets.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/Play.h>
#include <Gruntz/StatusBarMgr.h>
#include <Gruntz/TileCollisionKind.h>
#include <Gruntz/TileTriggerContainer.h>
#include <Gruntz/TileTriggerLogic.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/UserLogic.h>
#include <Wwd/WwdFile.h>

static char s_BadSwitch[] = "Bad switch at: x=%d, y=%d\n";
static char s_BadMulti[] = "Bad multi switch at: x=%d, y=%d\n";

static char s_CouldNotAdd[] = "Could not add Grunt: Player=%d, x=%d, y=%d";

static inline CGameLevel* LevelOf(CDDrawSurfaceMgr* holder) {
    return holder->m_level;
}

static inline i32 LookupTileType(CGameLevel* level, i32 x, i32 y) {
    CDDrawWorkerHost* g = level->m_mainPlane;
    if (x < 0) {
        x = 0;
    } else if (x >= g->m_wrapW) {
        x = g->m_wrapW - 1;
    }
    if (y < 0) {
        y = 0;
    } else if (y >= g->m_wrapH) {
        y = g->m_wrapH - 1;
    }
    i32 tx = x >> g->m_shiftX;
    i32 ty = y >> g->m_shiftY;
    i32 subX = x - (tx << g->m_shiftX);
    i32 subY = y - (ty << g->m_shiftY);
    i32 cell = g->GetTileHandle(tx, ty);
    if (cell == static_cast<i32>(0xeeeeeeee) || cell == -1) {
        return 0;
    }

    CImageSet1* tc = static_cast<CImageSet1*>(level->m_imageSets.GetAt(cell & 0xffff));
    return tc->GetCollisionAt(subX, subY);
}

// @early-stop
RVA(0x000d2b20, 0x21f)
i32 CPlay::PlaceStartGruntz() {

    CObList* list = &m_world->m_childGroup->m_list;
    if (list == 0) {
        return 0;
    }
    CGruntzMgr* reg = m_mgr;
    POSITION pos = list->GetHeadPosition();
    i32 result = 1;
    i32 counter = 0;
    i32 flag14 = 0;
    if (reg->m_gameMode == GAMEMODE_SINGLE) {
        flag14 = 1;
    }
    if (pos == 0) {
        return result;
    }
    do {
        CGameObject* obj = static_cast<CGameObject*>(list->GetNext(pos));
        if (obj != 0) {
            AnimWorkerObj* aux = obj->m_animWorker;

            GameObjNotifyFn who = aux->m_notify;
            if (who == CreateGruntStartingPoint) {
                AddrWord<long> extentArg;
                extentArg.m_addr = &obj->m_extent.left;
                i32 idx = reg->m_cmdGrid->PlaceObject(
                    obj->m_smarts,
                    (obj->m_screenX & ~0x1f) + 0x10,
                    (obj->m_screenY & ~0x1f) + 0x10,
                    100000,
                    flag14,
                    obj->m_score,
                    obj->m_powerup,
                    obj->m_damage,
                    obj->m_points,
                    obj->m_direction,
                    aux->m_minX,
                    aux->m_maxX,

                    extentArg.m_word
                );
                if (idx == -1) {
                    CString s;
                    s.Format(
                        s_CouldNotAdd,
                        obj->m_smarts,
                        (obj->m_screenX & ~0x1f) + 0x10,
                        (obj->m_screenY & ~0x1f) + 0x10
                    );
                    g_gameReg->EnterModalUI(static_cast<const char*>(static_cast<LPCSTR>(s)));
                    return 0;
                }
                obj->m_flags |= 0x10000;
            } else if (g_gameReg->m_gameMode != GAMEMODE_SINGLE && who == CreateGruntCreationPoint
                       && obj->m_smarts == g_curPlayer) {

                GruntzPlayer* e = &g_gameReg->m_options[g_curPlayer];
                if (e != 0 && counter < e->m_comboSel) {
                    reg->m_cmdSubMgr->EnqueueSingle(
                        result,
                        static_cast<char>(obj->m_smarts),
                        0,
                        0,
                        (obj->m_screenX & ~0x1f) + 0x10,
                        (obj->m_screenY & ~0x1f) + 0x10,
                        0,
                        0
                    );
                    counter++;
                }
            }
        }
    } while (pos != 0);
    return result;
}

RVA(0x000d2dd0, 0x1de4)
i32 CPlay::ValidateLevelTiles() {
    i32 validCount = 0;
    i32 counts[4];
    counts[0] = 0;
    counts[1] = 0;
    counts[2] = 0;
    counts[3] = 0;

    CObList* list = &m_world->m_childGroup->m_list;
    if (list == 0) {
        return 0;
    }
    POSITION pos = list->GetHeadPosition();
    if (pos == 0) {
        return 1;
    }

    i32 ok = 1;
    do {
        CGameObject* obj = static_cast<CGameObject*>(list->GetNext(pos));
        if (obj == 0) {
            continue;
        }

        GameObjNotifyFn who = obj->m_animWorker->m_notify;

        if (who == CreateTileTriggerSwitch) {
            CGameLevel* grid = LevelOf(m_world);
            i32 type = LookupTileType(LevelOf(m_world), obj->m_screenX, obj->m_screenY);
            if (type == 0x21) {

                void* hit = 0;
                i32 col = obj->m_speedX - 1;
                i32 colOff = col << 8;
                i32 row = obj->m_speedY - 1;
                while (col < obj->m_speedX + 2) {
                    row = obj->m_speedY - 1;
                    if (hit != 0) {
                        break;
                    }
                    while (row < obj->m_speedY + 2) {
                        void* r = m_beginMarker->FindInLists12(row + colOff, TRIGID_GIANT_ROCK_22);
                        if (r != 0) {
                            hit = r;
                        }
                        if (hit != 0) {
                            break;
                        }
                        row++;
                    }
                    if (hit != 0) {
                        break;
                    }
                    col++;
                    colOff += 0x100;
                }
                if (hit == 0) {
                    return 0;
                }
                i32 rel = (obj->m_speedY - row) * 3 - col + obj->m_speedX;

                i32 tcidx = (static_cast<CGiantRockLogic*>(hit))->m_matrix[rel + 4];
                if (tcidx == 0) {
                    return 0;
                }
                type = (static_cast<CImageSet1*>(grid->m_imageSets.GetAt(tcidx)))
                           ->GetCollisionAt(0, 0);
            }
            if (type == 0x1e || type == 0x1f || type == 0x22 || type == 0x23) {

                CTileTriggerLogic* r =
                    m_beginMarker->FindInLists12(obj->m_id, TRIGID_COVERED_POWERUP_26);
                if (r == 0) {
                    return 0;
                }
                i32 tcidx = r->m_tileToken;
                if (tcidx == 0) {
                    return 0;
                }
                type = (static_cast<CImageSet1*>(grid->m_imageSets.GetAt(tcidx)))
                           ->GetCollisionAt(0, 0);
            }
            switch (type) {
                case TILEKIND_MULTI_SWITCH:
                case TILEKIND_MULTI_SWITCH_UP:
                    if (!m_beginMarker->AddSwitchLogic(
                            TRIGID_MULTI_SWITCH_3,
                            obj->m_speedX,
                            obj->m_speedY,
                            obj->m_id,
                            obj->m_extent,
                            obj->m_area,
                            obj->m_switchRect,
                            obj->m_clip,
                            obj->m_animWorker->m_userRect1,
                            obj->m_animWorker->m_userRect2,
                            type == TILEKIND_MULTI_SWITCH_UP,
                            obj->m_damage,
                            0
                        )) {
                        CString s;
                        s.Format(s_BadSwitch, obj->m_screenX, obj->m_screenY);
                        g_gameReg->EnterModalUI(static_cast<LPCSTR>(s));
                        return 0;
                    }
                    validCount++;
                    obj->m_flags |= 0x10000;
                    break;
                case TILEKIND_EXCLUSIVE_SWITCH:
                case TILEKIND_EXCLUSIVE_SWITCH_UP:
                    if (!m_beginMarker->AddSwitchLogic(
                            TRIGID_EXCLUSIVE_SWITCH_4,
                            obj->m_speedX,
                            obj->m_speedY,
                            obj->m_id,
                            obj->m_extent,
                            obj->m_area,
                            obj->m_switchRect,
                            obj->m_clip,
                            obj->m_animWorker->m_userRect1,
                            obj->m_animWorker->m_userRect2,
                            type == TILEKIND_EXCLUSIVE_SWITCH_UP,
                            obj->m_damage,
                            0
                        )) {
                        CString s;
                        s.Format(s_BadSwitch, obj->m_screenX, obj->m_screenY);
                        g_gameReg->EnterModalUI(static_cast<LPCSTR>(s));
                        return 0;
                    }
                    validCount++;
                    obj->m_flags |= 0x10000;
                    break;
                case TILEKIND_SECRET_SWITCH:
                case TILEKIND_SECRET_SWITCH_UP:
                    if (!m_beginMarker->AddSwitchLogic(
                            TRIGID_SECRET_SWITCH_6,
                            obj->m_speedX,
                            obj->m_speedY,
                            obj->m_id,
                            obj->m_extent,
                            obj->m_area,
                            obj->m_switchRect,
                            obj->m_clip,
                            obj->m_animWorker->m_userRect1,
                            obj->m_animWorker->m_userRect2,
                            type == TILEKIND_SECRET_SWITCH_UP,
                            obj->m_damage,
                            0
                        )) {
                        CString s;
                        s.Format(s_BadSwitch, obj->m_screenX, obj->m_screenY);
                        g_gameReg->EnterModalUI(static_cast<LPCSTR>(s));
                        return 0;
                    }
                    validCount++;
                    obj->m_flags |= 0x10000;
                    break;
                case TILEKIND_TIME_SWITCH:
                case TILEKIND_TIME_SWITCH_UP:
                    if (!m_beginMarker->AddSwitchLogic(
                            TRIGID_TIME_SWITCH_7,
                            obj->m_speedX,
                            obj->m_speedY,
                            obj->m_id,
                            obj->m_extent,
                            obj->m_area,
                            obj->m_switchRect,
                            obj->m_clip,
                            obj->m_animWorker->m_userRect1,
                            obj->m_animWorker->m_userRect2,
                            type == TILEKIND_TIME_SWITCH_UP,
                            obj->m_damage,
                            0
                        )) {
                        CString s;
                        s.Format(s_BadSwitch, obj->m_screenX, obj->m_screenY);
                        g_gameReg->EnterModalUI(static_cast<LPCSTR>(s));
                        return 0;
                    }
                    validCount++;
                    obj->m_flags |= 0x10000;
                    break;
                case TILEKIND_CHECKPOINT:
                case TILEKIND_CHECKPOINT_UP:
                    if (!m_beginMarker->AddSwitchLogic(
                            TRIGID_CHECKPOINT_SWITCH_8,
                            obj->m_speedX,
                            obj->m_speedY,
                            obj->m_id,
                            obj->m_extent,
                            obj->m_area,
                            obj->m_switchRect,
                            obj->m_clip,
                            obj->m_animWorker->m_userRect1,
                            obj->m_animWorker->m_userRect2,
                            type == TILEKIND_CHECKPOINT_UP,
                            obj->m_damage,
                            obj->m_smarts
                        )) {
                        CString s;
                        s.Format(s_BadSwitch, obj->m_screenX, obj->m_screenY);
                        g_gameReg->EnterModalUI(static_cast<LPCSTR>(s));
                        return 0;
                    }
                    validCount++;
                    obj->m_flags |= 0x10000;
                    break;
                case TILEKIND_SWITCH_A:
                case TILEKIND_SWITCH_A_UP:
                    if (!m_beginMarker->AddSwitchLogic(
                            TRIGID_SWITCH_1,
                            obj->m_speedX,
                            obj->m_speedY,
                            obj->m_id,
                            obj->m_extent,
                            obj->m_area,
                            obj->m_switchRect,
                            obj->m_clip,
                            obj->m_animWorker->m_userRect1,
                            obj->m_animWorker->m_userRect2,
                            type == TILEKIND_SWITCH_A_UP || type == TILEKIND_SWITCH_B_UP
                                || type == TILEKIND_SWITCH_C_UP
                                || type == TILEKIND_SECRET_SWITCH_UP,
                            obj->m_damage,
                            0
                        )) {
                        CString s;
                        s.Format(s_BadMulti, obj->m_screenX, obj->m_screenY);
                        g_gameReg->EnterModalUI(static_cast<LPCSTR>(s));
                        return 0;
                    }
                    validCount++;
                    obj->m_flags |= 0x10000;
                    break;
                case TILEKIND_SWITCH_B:
                case TILEKIND_SWITCH_B_UP:
                    if (!m_beginMarker->AddSwitchLogic(
                            TRIGID_SWITCH_2,
                            obj->m_speedX,
                            obj->m_speedY,
                            obj->m_id,
                            obj->m_extent,
                            obj->m_area,
                            obj->m_switchRect,
                            obj->m_clip,
                            obj->m_animWorker->m_userRect1,
                            obj->m_animWorker->m_userRect2,
                            type == TILEKIND_SWITCH_A_UP || type == TILEKIND_SWITCH_B_UP
                                || type == TILEKIND_SWITCH_C_UP
                                || type == TILEKIND_SECRET_SWITCH_UP,
                            obj->m_damage,
                            0
                        )) {
                        CString s;
                        s.Format(s_BadMulti, obj->m_screenX, obj->m_screenY);
                        g_gameReg->EnterModalUI(static_cast<LPCSTR>(s));
                        return 0;
                    }
                    validCount++;
                    obj->m_flags |= 0x10000;
                    break;
                case TILEKIND_SWITCH_C:
                case TILEKIND_SWITCH_C_UP:
                    if (!m_beginMarker->AddSwitchLogic(
                            TRIGID_SWITCH_5,
                            obj->m_speedX,
                            obj->m_speedY,
                            obj->m_id,
                            obj->m_extent,
                            obj->m_area,
                            obj->m_switchRect,
                            obj->m_clip,
                            obj->m_animWorker->m_userRect1,
                            obj->m_animWorker->m_userRect2,
                            type == TILEKIND_SWITCH_A_UP || type == TILEKIND_SWITCH_B_UP
                                || type == TILEKIND_SWITCH_C_UP
                                || type == TILEKIND_SECRET_SWITCH_UP,
                            obj->m_damage,
                            0
                        )) {
                        CString s;
                        s.Format(s_BadMulti, obj->m_screenX, obj->m_screenY);
                        g_gameReg->EnterModalUI(static_cast<LPCSTR>(s));
                        return 0;
                    }
                    validCount++;
                    obj->m_flags |= 0x10000;
                    break;
                default:
                    break;
            }
        } else if (who == CreateTileTrigger) {
            i32 type = LookupTileType(LevelOf(m_world), obj->m_screenX, obj->m_screenY);
            static_cast<void>(type);
            obj->m_flags |= 0x10000;
        } else if (who == CreateTileSecretTrigger) {
            i32 type = LookupTileType(LevelOf(m_world), obj->m_screenX, obj->m_screenY);
            static_cast<void>(type);
            obj->m_flags |= 0x10000;
        } else if (who == CreateLevelTime) {

            if (m_frameMarker != 0 && m_mgr->m_gameMode != GAMEMODE_MULTIPLAYER
                && g_gameReg->m_isEasyMode != 0 && g_gameReg->m_gameMode == ok) {
                i32 a = obj->m_points;
                i32 b = obj->m_score;
                a += a;
                b += b;
                if (a > 0x3b) {
                    b++;
                    a -= 0x3c;
                }
                m_frameMarker->SetTime(b, a);
            }
            obj->m_flags |= 0x10000;
        } else if (who == CreateInGameIcon) {
            if (obj->m_smarts == PICKUP_MEGAPHONE) {

                m_guts->InsertPtr(obj->m_points, obj->m_score);
            }
        } else if (who == CreateGruntCreationPoint) {
            if (obj->m_smarts == g_curPlayer) {
                CoordPoolNode* cell = g_coordPool.m_freeHead;
                void* slot = 0;
                if (cell->m_next != 0) {
                    slot = &cell->m_coord;
                    g_coordPool.m_freeHead = cell->m_next;
                }
                if (slot != 0) {
                    (static_cast<i32*>(slot))[0] = (obj->m_screenX & ~0x1f) + 0x10;
                    (static_cast<i32*>(slot))[1] = (obj->m_screenY & ~0x1f) + 0x10;
                }
            }
        } else if (who == CreateBrickz) {

            CDDrawWorkerHost* pl = m_world->m_level->m_mainPlane;
            i32 tile = pl->m_tileGrid[pl->m_colOffsets[obj->m_speedY] + obj->m_speedX];
            if (tile >= 0x12f && tile <= 0x149) {
                if (m_beginMarker->AddToList3(
                        tile,
                        obj->m_speedX,
                        obj->m_speedY,
                        obj->m_id,
                        obj->m_extent.left,
                        obj->m_extent.top,
                        obj->m_extent.right,
                        obj->m_extent.bottom
                    )
                    != 0) {
                    validCount++;
                    obj->m_flags |= 0x10000;
                }
            }
        } else if (who == CreateGruntPuddle) {

            m_mgr->m_cmdGrid->PlacePuddle(obj, 0);
        } else if (who == CreateGuardPoint) {

            i32 col = obj->m_screenX >> 5;
            i32 rowBase = obj->m_screenY >> 5;
            i32 stride = (col << 3) - col;

            i32 ebp = stride - 7;
            for (i32 dy = -1; dy < 2; dy++, ebp += 7) {
                i32 row = rowBase;
                i32 ofs = rowBase - 1;
                for (i32 k = 3; k != 0; k--, ofs++, row++) {
                    i32 gx = dy + col;
                    i32 gyy = row - 1;
                    CGruntzMapMgr* gg = g_gameReg->m_tileGrid;
                    if (static_cast<u32>(gx) >= gg->m_width
                        || static_cast<u32>(gyy) >= gg->m_height) {
                        continue;
                    }
                    i32 kind = obj->m_smarts;
                    i32 bit;
                    if (static_cast<u32>(kind) > 3) {
                        bit = 0;
                    } else {
                        switch (kind) {
                            case 0:
                                bit = 0x100000;
                                break;
                            case 1:
                                bit = 0x200000;
                                break;
                            case 2:
                                bit = 0x400000;
                                break;
                            default:
                                bit = 0x800000;
                                break;
                        }
                    }
                    counts[kind]++;
                    gg = g_gameReg->m_tileGrid;
                    if (static_cast<u32>(gx) >= gg->m_width
                        || static_cast<u32>(gyy) >= gg->m_height) {
                        continue;
                    }
                    i32* cellRow = gg->m_rowInts[0] + ofs;
                    cellRow[ebp] |= bit;
                }
            }
        } else if (who == CreateToobSpikez) {
            CGruntzMapMgr* gg = g_gameReg->m_tileGrid;
            i32 cy = obj->m_screenX >> 5;
            i32 cx = obj->m_screenY >> 5;
            if (static_cast<u32>(cy) < gg->m_width && static_cast<u32>(cx) < gg->m_height) {
            }
        } else if (who == CreateWarpStonePad) {
            if (g_gameReg->m_gameMode != ok) {
                CoordPoolNode* cell = g_coordPool.m_freeHead;
                if (cell->m_next != 0) {
                    g_coordPool.m_freeHead = cell->m_next;
                }
            }
        }
    } while (pos != 0);

    return ok;
}

// @early-stop
RVA(0x000d5b20, 0xbb)
i32 CPlay::PositionBridgeToggle(i32 mode, i32) {
    CGruntzMgr* w = m_mgr;
    i32 ex = w->m_modeW;
    i32 ey = w->m_modeH;
    CTimer* pt;
    if (mode == 1) {
        m_hitTest->Configure(2);
        pt = m_frameMarker;
        if (pt == 0) {
            goto done;
        }
        ex -= 0x37;
    } else if (mode == 0) {
        m_hitTest->Configure(1);
        pt = m_frameMarker;
        if (pt == 0) {
            goto done;
        }
        ex -= 0xd7;
    } else {
        m_hitTest->Configure(3);
        pt = m_frameMarker;
        if (pt == 0) {
            goto done;
        }
        ex -= 0x37;
    }
    ey -= 0x16;
    pt->m_baseX = ex;
    pt->m_baseY = ey;
done:

    CTriggerMgr* g = m_mgr->m_cmdGrid;
    CWwdGameObjectA* goal = g->m_goal;
    if (goal != 0) {
        if (goal != 0) {
            goal->m_flags |= 0x10000;
            g->m_goal = 0;
        }
        m_mgr->m_cmdGrid->LoadCameraSprite();
    }
    return 1;
}
