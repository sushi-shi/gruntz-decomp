#include <Bute/ButeMgr.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawSubMgrLeafScan.h>
#include <Enums.h>
#include <Gruntz/ActionOptionsMenuBar.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/BattlezData.h>
#include <Gruntz/BattlezUnitKind.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/EnemyAiType.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameModeId.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntDeathType.h>
#include <Gruntz/GruntDirection.h>
#include <Gruntz/GruntPickupInline.h>
#include <Gruntz/GruntPoweredStateMacros.h>
#include <Gruntz/GruntPuddle.h>
#include <Gruntz/GruntSpawnConfig.h>
#include <Gruntz/GruntzCmdMgr.h>
#include <Gruntz/GruntzCommandId.h>
#include <Gruntz/GruntzMapMgr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/HealthPct.h>
#include <Gruntz/LeafCue.h>
#include <Gruntz/LevelArea.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/Play.h>
#include <Gruntz/PlayerCommandKind.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/StatusBarMgr.h>
#include <Gruntz/TileCollisionKind.h>
#include <Gruntz/TileGrid.h>
#include <Gruntz/TileTriggerContainer.h>
#include <Gruntz/TileTriggerLogic.h>
#include <Gruntz/TileTriggerSwitchLogic.h>
#include <Gruntz/Timer.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TriggerMgrRecords.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/UserLogic.h>
#include <Utils/MapTyped.h>
#include <Wap32/CoordUnset.h>
#include <Wap32/TileGeometry.h>
#include <Wwd/WwdFile.h>

#include <stddef.h>

RVA(0x0006b640, 0x2f)
i32 CTriggerMgr::SetLevel(CDDrawSurfaceMgr* lvl) {
    if (lvl == NULL) {
        return 0;
    }
    m_world = lvl;
    m_armed = 0;
    m_pendingFx = NULL;
    m_countdownActive = 1;
    return 1;
}

RVA(0x0006b680, 0x39)
void CTriggerMgr::Cleanup() {
    CActionOptionsMenuBar* ov = m_overlay;
    if (ov != NULL) {
        ov->Clear();
        operator delete(ov);
        m_overlay = NULL;
    }
    ClearRecords();
    ClearSelections();
}

// @early-stop
RVA(0x0006b6d0, 0x434)
i32 CTriggerMgr::PlaceObject(
    i32 row,
    i32 x,
    i32 y,
    i32 z,
    GruntEntranceMode mode,
    i32 kindDefault,
    i32 typeKind,
    i32 vehicleKind,
    i32 aiType,
    i32 defenderRadiusMinusOne,
    i32 defenderQueuePosition,
    i32 defenderPickupType,
    RECT* span
) {

    {
        if (m_world == NULL) {
            goto fail;
        }
        i32 wantSlot = 0;
        i32 special = 0;
        if (static_cast<PickupType>(typeKind) == PICKUP_TOOB) {
            special = 0x100;
            wantSlot = 1;
        }
        CGruntzMapMgr* plane = g_gameReg->m_tileGrid;
        i32 tx = x >> TILE_SHIFT_PX;
        i32 ty = y >> TILE_SHIFT_PX;
        i32 attr = plane->CellFlagsAt(tx, ty);
        if ((attr & 0x4000911) != 0 && (special & attr) == 0) {
            goto fail;
        }
        if ((attr & 0x82) != 0) {
            goto fail;
        }
        if ((attr & 0x400) != 0) {
            goto fail;
        }

        i32 col = 0;
        i32 onSpecialTile;
        if (wantSlot != col && (attr & 0x100) != 0) {
            onSpecialTile = 1;
            if (mode != GRUNT_ENTRANCE_NONE) {
                goto fail;
            }
        } else {
            onSpecialTile = 0;
        }

        i32 base = row * TM_GRID_COLS;
        while (m_grid[base + col] != NULL) {
            if (col >= TM_GRID_COLS) {
                goto fail;
            }
            col++;
        }
        if (col >= TM_GRID_COLS) {
            goto fail;
        }

        CWwdGameObjectA* sprite = m_world->m_childGroup->CreateSprite(0, x, y, z, "Grunt", 0x40003);
        if (sprite == NULL) {
            goto fail;
        }
        sprite->m_animWorker->m_notify(sprite);
        CGrunt* logic = static_cast<CGrunt*>(sprite->m_animWorker->m_logic);
        CGruntzMgr* game = g_gameReg;

        // NOT a PickupType local: the AI-type switch fills it with tool ids,
        // but the player-slot path below overwrites it with m_colorIndex, so it
        // carries two domains. Converted explicitly where it enters Place().
        i32 kindId;
        if (game->m_gameMode == GAMEMODE_SINGLE) {
            switch (aiType) {
                case BZUNIT_BOMB:
                    kindId = IDX(PICKUP_BOMB);
                    break;
                case BZUNIT_GUNHAT:
                    kindId = IDX(PICKUP_GUNHAT);
                    break;
                case BZUNIT_GAUNTLETZ:
                    kindId = IDX(PICKUP_GAUNTLETZ);
                    break;
                case BZUNIT_CLUB:
                    kindId = IDX(PICKUP_CLUB);
                    break;
                case BZUNIT_SHIELD:
                    kindId = IDX(PICKUP_SHIELD);
                    break;
                case BZUNIT_GLOVEZ:
                    kindId = IDX(PICKUP_GLOVEZ);
                    break;
                case BZUNIT_BRICK:
                    kindId = IDX(PICKUP_BRICK);
                    typeKind = 1;
                    break;
                case BZUNIT_GRAVITYBOOTZ:
                    kindId = IDX(PICKUP_GRAVITYBOOTZ);
                    typeKind = 3;
                    break;
                case BZUNIT_SPY:
                    kindId = IDX(PICKUP_SPY);
                    typeKind = 7;
                    break;
                case BZUNIT_NERFGUN:
                    kindId = IDX(PICKUP_NERFGUN);
                    typeKind = 13;
                    break;
                case BZUNIT_BOOMERANG:
                    kindId = IDX(PICKUP_BOOMERANG);
                    typeKind = 5;
                    break;
                case BZUNIT_GOOBER:
                    kindId = IDX(PICKUP_GOOBER);
                    break;
                case BZUNIT_SWORD:
                    kindId = IDX(PICKUP_SWORD);
                    break;
                case BZUNIT_ROCK:
                    kindId = IDX(PICKUP_ROCK);
                    typeKind = 17;
                    break;
                case BZUNIT_SHOVEL:
                    kindId = IDX(PICKUP_SHOVEL);
                    typeKind = 19;
                    break;
                case BZUNIT_SHOVEL_MOUNTED:
                    kindId = IDX(PICKUP_SHOVEL);
                    vehicleKind = IDX(PICKUP_SCROLL);
                    break;
                default:
                    kindId = kindDefault;
                    break;
            }
        } else {
            kindId = kindDefault;
        }

        if (m_rowCount[row] < game->m_options[row].m_comboSel) {
            if (game->m_options[row].m_liveGate != 0
                || (row != g_curPlayer
                    && kindId == IDX(game->m_options[g_curPlayer].m_colorIndex))) {
                kindId = IDX(game->m_options[row].m_colorIndex);
            }
            if (row == g_curPlayer && aiType != 0) {
                aiType = 0;
            }
            if (logic->Place(
                    this,
                    row,
                    col,
                    static_cast<PickupType>(kindId),
                    static_cast<PickupType>(typeKind),
                    vehicleKind,
                    static_cast<EnemyAiType>(aiType),
                    defenderRadiusMinusOne,
                    defenderQueuePosition,
                    defenderPickupType,
                    span,
                    mode
                )
                == 0) {
                logic->SetObjectFlags(0x10000);
                return -1;
            }

            if (mode == GRUNT_ENTRANCE_WORMHOLE) {
                CWwdGameObjectA* hole =
                    m_world->m_childGroup->CreateSprite(0, x, y, 0, "Wormhole", 0x40003);
                if (hole == NULL) {
                    logic->SetObjectFlags(0x10000);
                    return -1;
                }
                hole->m_smarts = g_buteMgr.GetIntDef("Wormhole", "EntranceColor", 0xe);
            } else if (mode == GRUNT_ENTRANCE_RESURRECT || mode == GRUNT_ENTRANCE_DROP) {

                if (mode == GRUNT_ENTRANCE_RESURRECT) {
                    logic->m_health = HEALTH_RESPAWN;
                }
            } else {
                if (onSpecialTile != 0) {
                    logic->SetupTubeAnim(1);
                }
                WireTileSwitchLogic(logic, x, y);
            }

            m_grid[base + col] = logic;
            m_rowCount[row] += 1;
            m_cellFlag[base + col] = 0;
            game->m_scoreHud->m_counts[row] += 1;
            return col;
        }
    }
fail:
    return -1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0006bc20, 0x6f)
i32 CTriggerMgr::DispatchCellForObject(CGrunt* obj, i32 startRow, GruntDeathType kind, i32 arg) {
    i32 last;
    if (startRow == TM_GRID_ROW_ALL) {
        startRow = 0;
        last = 3;
    } else {
        last = startRow;
    }
    for (i32 row = startRow; row <= last; row++) {
        CGrunt** cell = &m_grid[row * TM_GRID_COLS];
        for (i32 col = 0; col < TM_GRID_COLS; col++) {
            if (cell[col] == obj) {
                return CellDispatch(row, col, kind, arg);
            }
        }
    }
    return 0;
}

RVA(0x0006bcb0, 0x6a)
i32 CTriggerMgr::CellDispatch(i32 row, i32 col, GruntDeathType kind, i32 arg) {
    CGrunt* cell = m_grid[row * TM_GRID_COLS + col];
    if (cell == NULL) {
        return 0;
    }
    if (cell->m_deathAnimStarted != 0) {
        NotifyCell(row, col, 0);
        return 0;
    }

    if (kind == DEATH_EXIT) {
        (static_cast<CGrunt*>(cell))->BuildGruntExitAnimation();
    } else {
        (static_cast<CGrunt*>(cell))->LoadGruntDeathAnimations(kind, arg);
    }
    return 1;
}

RVA(0x0006bd40, 0xb3)
i32 CTriggerMgr::ClearGridRange(i32 startRow) {
    i32 row, last;
    if (startRow == TM_GRID_ROW_ALL) {
        row = 0;
        last = 3;
    } else {
        last = startRow;
        row = startRow;
    }
    ResetAll();
    for (i32 r = row; r <= last; r++) {
        CGrunt** cell = &m_grid[r * TM_GRID_COLS];
        for (i32 col = 0; col < TM_GRID_COLS; col++) {
            CGrunt* c = cell[col];
            if (c != NULL) {
                c->SetObjectFlags(0x10000);
                cell[col] = NULL;
                m_cellFlag[r * TM_GRID_COLS + col] = 0;
            }
        }
        m_rowCount[r] = 0;
        m_gruntzExitedByPlayer[r] = 0;
        m_gruntzLostByPlayer[r] = 0;
    }
    ClearSelections();
    return 1;
}

// @early-stop
// The call, size, 28-instruction skeleton, and all loads agree. Retail only
// schedules the view-rectangle left load across the completed y expression.
RVA(0x0006be30, 0x47)
CGrunt* CTriggerMgr::ScreenToCell(i32 sx, i32 sy, i32* outRow, i32* outCol, i32 startRow) {
    CGameLevel* view = m_world->m_level;
    RECT* r = &view->m_mainPlane->m_viewRect;
    i32 px = r->left - view->m_planeCtx.left + sx;
    i32 py = r->top - view->m_planeCtx.top + sy;
    return CellHitTest(px, py, outRow, outCol, startRow);
}

// @early-stop
RVA(0x0006bea0, 0xe2)
CGrunt* CTriggerMgr::CellHitTest(i32 px, i32 py, i32* outRow, i32* outCol, i32 startRow) {
    i32 last;
    if (startRow == TM_GRID_ROW_ALL) {
        startRow = 0;
        last = 3;
    } else {
        last = startRow;
    }

    if (startRow <= last) {
        do {
            CGrunt** cell = &m_grid[startRow * TM_GRID_COLS];
            for (i32 col = 0; col < TM_GRID_COLS; col++) {
                CGrunt* g = cell[col];
                if (g != NULL && g->m_entranceCommitted != 0) {
                    CWwdGameObjectA* o = g->m_object;
                    if (o->m_layer != NULL) {
                        i32 x0 = o->m_screenX - 15;
                        i32 y0 = o->m_screenY - 15;
                        i32 x1 = x0 + 30;
                        i32 y1 = y0 + 30;
                        if (px < x1 && px >= x0 && py < y1 && py >= y0) {
                            if (outRow != NULL) {
                                *outRow = startRow;
                            }
                            if (outCol != NULL) {
                                *outCol = col;
                            }
                            return m_grid[startRow * TM_GRID_COLS + col];
                        }
                    }
                }
            }
            startRow++;
        } while (startRow <= last);
    }
    return NULL;
}

RVA(0x0006bfd0, 0x106)
i32 CTriggerMgr::ResetCell(i32 col, i32 row, i32 force, i32 keep) {
    i32 idx = col * TM_GRID_COLS + row;
    CGrunt* cell = m_grid[idx];
    if (cell == NULL || cell->m_entranceCommitted == 0) {
        return 0;
    }
    if (col != g_curPlayer) {

        cell->CreateHealthSprite();
        cell->CreateStaminaSprite();
        cell->CreateToySprite();
        cell->m_hudRetireWindowLo = g_buteMgr.GetDwordDef("Grunt", "CombatTimeout", 0x1388);
        cell->m_hudRetireWindowHi = 0;
        cell->m_hudRetireClockLo = g_frameTime;
        cell->m_hudRetireClockHi = 0;
        return 0;
    }
    if (force == 0) {

        ResetAll();
    } else if (keep == 0) {
        if (RemoveCellRecord(col, row, 0) != 0) {
            return 1;
        }
    }
    CoordPoolNode* node = g_coordPool.m_freeHead;
    Coord* slot = NULL;
    if (node->m_next != NULL) {
        slot = &node->m_coord;
        slot->m_x = col;
        slot->m_y = row;
        g_coordPool.m_freeHead = g_coordPool.m_freeHead->m_next;
    }
    m_recList.AddTail(slot);

    return cell->CommitArrival();
}

// @early-stop
// The fixed RIGHT arm and CURRENT/EAST arm are source-distinct. Depending on
// unrelated header state, cl either emits both calls or cross-jumps RIGHT into
// CURRENT/EAST as retail does; local source-shape probes do not steer it.
RVA(0x0006c130, 0xe38)
i32 CTriggerMgr::WireTileSwitchLogic(CGrunt* g, i32 x, i32 y) {

    CPlay* state = static_cast<CPlay*>(g_gameReg->m_curState);

    if (g != NULL) {
        g->m_neighborScanEnabled = 1;
    }

    CGameLevel* level = m_world->m_level;
    i32 cx = x;
    i32 cy = y;
    if (cx < 0) {
        cx = 0;
    } else if (cx >= level->m_mainPlane->m_wrapW) {
        cx = level->m_mainPlane->m_wrapW - 1;
    }
    if (cy < 0) {
        cy = 0;
    } else if (cy >= level->m_mainPlane->m_wrapH) {
        cy = level->m_mainPlane->m_wrapH - 1;
    }
    i32 tx = cx >> level->m_mainPlane->m_shiftX;
    i32 ty = cy >> level->m_mainPlane->m_shiftY;
    i32 subX = cx - (tx << level->m_mainPlane->m_shiftX);
    i32 subY = cy - (ty << level->m_mainPlane->m_shiftY);
    i32 raw = level->m_mainPlane->m_tileGrid[level->m_mainPlane->m_colOffsets[ty] + tx];
    TileCollisionKind tag;
    if (raw == UNINIT_FILL || raw == -1) {
        tag = TILEKIND_PASSABLE;
    } else {
        CTileImageSet* ts = static_cast<CTileImageSet*>(level->m_imageSets.GetAt(raw & 0xffff));
        // Ingest: the raw WWD attribute byte for this cell.
        tag = ts->GetCollisionAt(subX, subY);
    }

    if (static_cast<u32>((IDX(tag) - 0xb)) > 0x65) {
        return 0;
    }

    CTileTriggerSwitchLogic* sw;
    POSITION pos;
    i32 anyHit;
    i32 stop;

    switch (tag) {
        case TILEKIND_TIME_SWITCH:
            sw = state->m_beginMarker->FindChild(
                ((x >> TILE_SHIFT_PX) * 0x100) + (y >> TILE_SHIFT_PX),
                TRIGID_TIME_SWITCH_7
            );
            if (sw == NULL) {
                CString msg;
                msg.Format("No switch logic found for switch at: x=%d, y=%d", x, y);
                g_gameReg->EnterModalUI(static_cast<const char*>(msg));
                g_gameReg->ReportError(IDX(TRIGERR_LOOKUP_MISS), IDX(TRIGSITE_WIRE_TIME_SWITCH));
                return 0;
            }
            sw->SwitchDown();
            pos = state->m_beginMarker->m_list2.GetHeadPosition();
            while (pos != NULL) {
                CTileTriggerLogic* el =
                    static_cast<CTileTriggerLogic*>(state->m_beginMarker->m_list2.GetNext(pos));
                if (el->FindIndexByKey(sw->m_cellKey) != 0) {
                    return 1;
                }
            }
            anyHit = 0;
            pos = state->m_beginMarker->m_list1.GetHeadPosition();
            while (pos != NULL) {
                CTileTriggerLogic* el =
                    static_cast<CTileTriggerLogic*>(state->m_beginMarker->m_list1.GetNext(pos));
                if (el->FindIndexByKey(sw->m_cellKey) != 0) {
                    el->RecordMove();
                    anyHit = 1;
                }
            }
            if (anyHit == 0) {
                CString msg;
                msg.Format("No trigger logic found for switch at: x=%d, y=%d", x, y);
                g_gameReg->EnterModalUI(static_cast<const char*>(msg));
                g_gameReg->ReportError(IDX(TRIGERR_LINK_BROKEN), IDX(TRIGSITE_WIRE_TIME_TRIGGER));
                return 0;
            }
            return 1;

        case TILEKIND_SECRET_SWITCH:
            sw = state->m_beginMarker->FindChild(
                ((x >> TILE_SHIFT_PX) * 0x100) + (y >> TILE_SHIFT_PX),
                TRIGID_SECRET_SWITCH_6
            );
            if (sw == NULL) {
                CString msg;
                msg.Format("No switch logic found for switch at: x=%d, y=%d", x, y);
                g_gameReg->EnterModalUI(static_cast<const char*>(msg));
                g_gameReg->ReportError(IDX(TRIGERR_LOOKUP_MISS), IDX(TRIGSITE_WIRE_SECRET_SWITCH));
                return 0;
            }
            sw->SwitchDown();
            anyHit = 0;
            pos = state->m_beginMarker->m_list1.GetHeadPosition();
            while (pos != NULL) {
                CTileTriggerLogic* el =
                    static_cast<CTileTriggerLogic*>(state->m_beginMarker->m_list1.GetNext(pos));
                if (el->FindIndexByKey(sw->m_cellKey) != 0) {
                    el->RecordMove();
                    anyHit = 1;
                }
            }
            if (anyHit == 0) {
                CString msg;
                msg.Format("No trigger logic found for switch at: x=%d, y=%d", x, y);
                g_gameReg->EnterModalUI(static_cast<const char*>(msg));
                g_gameReg->ReportError(IDX(TRIGERR_LINK_BROKEN), IDX(TRIGSITE_WIRE_SECRET_TRIGGER));
                return 0;
            }
            {
                g_gameReg->m_scoreHud->m_secretsFound++;
                {
                    CDDrawSubMgrLeafScan* set = m_world->m_soundRegistry;
                    if (set->m_emitGate == 0) {
                        LeafCue* found = NULL;
                        MapLookup(set->m_cues, "GAME_SECRETSWITCH", found);
                        if (found != NULL) {
                            found->PlayIfElapsed(g_sndCueTag, 0, 0, 0);
                        }
                    }
                }
                if (g != NULL) {
                    i32 cueX = g->m_object->m_screenX;
                    i32 cueY = g->m_object->m_screenY;
                    if (CGameLevel::PointInRect(&g_gameReg->m_viewBounds, cueX, cueY)) {
                        g_gameReg->m_cueSink->SpawnVoiceDriver(g, 0x3f2, -1, 0, -1, -1);
                    }
                } else if (CGameLevel::PointInRect(&g_gameReg->m_viewBounds, x, y)) {
                    g_gameReg->m_cueSink->SpawnVoiceDriver(NULL, 0x3f2, -1, 1, -1, -1);
                }
            }
            return 1;

        case TILEKIND_SWITCH_A:
        case TILEKIND_SWITCH_B:
        case TILEKIND_SWITCH_C:
            sw = state->m_beginMarker->FindChild(
                ((x >> TILE_SHIFT_PX) * 0x100) + (y >> TILE_SHIFT_PX),
                TRIGID_ANY
            );
            if (sw == NULL) {
                CString msg;
                msg.Format("No switch logic found for switch at: x=%d, y=%d", x, y);
                g_gameReg->EnterModalUI(static_cast<const char*>(msg));
                g_gameReg->ReportError(IDX(TRIGERR_LOOKUP_MISS), IDX(TRIGSITE_WIRE_SWITCH));
                return 0;
            }
            sw->SwitchDown();
            anyHit = 0;
            stop = 0;
            pos = state->m_beginMarker->m_list1.GetHeadPosition();
            while (pos != NULL && stop == 0) {
                CTileTriggerLogic* el =
                    static_cast<CTileTriggerLogic*>(state->m_beginMarker->m_list1.GetNext(pos));
                if (el->FindIndexByKey(sw->m_cellKey) != 0) {
                    if (el->Tick() == 0) {
                        stop = 1;
                    }
                    anyHit = 1;
                }
            }
            if (anyHit == 0) {
                CString msg;
                msg.Format("No trigger logic found for switch at: x=%d, y=%d", x, y);
                g_gameReg->EnterModalUI(static_cast<const char*>(msg));
                g_gameReg->ReportError(IDX(TRIGERR_LINK_BROKEN), IDX(TRIGSITE_WIRE_TRIGGER));
                return 0;
            }
            return 1;

        case TILEKIND_MULTI_SWITCH:
            sw = state->m_beginMarker->FindChild(
                ((x >> TILE_SHIFT_PX) * 0x100) + (y >> TILE_SHIFT_PX),
                TRIGID_MULTI_SWITCH_3
            );
            if (sw == NULL) {
                CString msg;
                msg.Format("No switch logic found for switch at: x=%d, y=%d", x, y);
                g_gameReg->EnterModalUI(static_cast<const char*>(msg));
                g_gameReg->ReportError(IDX(TRIGERR_LOOKUP_MISS), IDX(TRIGSITE_WIRE_MULTI_SWITCH));
                return 0;
            }
            sw->SwitchDown();
            if (sw->VerifyBlockLinksB() == 0) {
                return 1;
            }
            anyHit = 0;
            stop = 0;
            pos = state->m_beginMarker->m_list1.GetHeadPosition();
            while (pos != NULL && stop == 0) {
                CTileTriggerLogic* el =
                    static_cast<CTileTriggerLogic*>(state->m_beginMarker->m_list1.GetNext(pos));
                if (el->FindIndexByKey(sw->m_cellKey) != 0) {
                    if (el->Tick() == 0) {
                        stop = 1;
                    }
                    anyHit = 1;
                }
            }
            if (anyHit == 0) {
                CString msg;
                msg.Format("No trigger logic found for switch at: x=%d, y=%d", x, y);
                g_gameReg->EnterModalUI(static_cast<const char*>(msg));
                g_gameReg->ReportError(IDX(TRIGERR_LINK_BROKEN), IDX(TRIGSITE_WIRE_MULTI_TRIGGER));
                return 0;
            }
            return 1;

        case TILEKIND_EXCLUSIVE_SWITCH:
            sw = state->m_beginMarker->FindChild(
                ((x >> TILE_SHIFT_PX) * 0x100) + (y >> TILE_SHIFT_PX),
                TRIGID_EXCLUSIVE_SWITCH_4
            );
            if (sw == NULL) {
                CString msg;
                msg.Format("No switch logic found for switch at: x=%d, y=%d", x, y);
                g_gameReg->EnterModalUI(static_cast<const char*>(msg));
                g_gameReg->ReportError(
                    IDX(TRIGERR_LOOKUP_MISS),
                    IDX(TRIGSITE_WIRE_EXCLUSIVE_SWITCH)
                );
                return 0;
            }
            if (sw->SwitchDown() == 0) {
                return 1;
            }
            anyHit = 0;
            stop = 0;
            pos = state->m_beginMarker->m_list1.GetHeadPosition();
            while (pos != NULL && stop == 0) {
                CTileTriggerLogic* el =
                    static_cast<CTileTriggerLogic*>(state->m_beginMarker->m_list1.GetNext(pos));
                if (el->FindIndexByKey(sw->m_cellKey) != 0) {
                    if (el->Tick() == 0) {
                        stop = 1;
                    }
                    anyHit = 1;
                }
            }
            if (anyHit == 0) {
                CString msg;
                msg.Format("No trigger logic found for switch at: x=%d, y=%d", x, y);
                g_gameReg->EnterModalUI(static_cast<const char*>(msg));
                g_gameReg->ReportError(
                    IDX(TRIGERR_LINK_BROKEN),
                    IDX(TRIGSITE_WIRE_EXCLUSIVE_TRIGGER)
                );
                return 0;
            }
            return 1;

        case TILEKIND_ARROW_UP_A:
        case TILEKIND_ARROW_UP_B:
            if (g == NULL || g->m_deathAnimStarted != 0) {
                return 1;
            }
            g->m_entranceActive = 1;
            g->StepArrivalDrop(x, y - 32, 0, -1, 1, 0);
            return 1;

        case TILEKIND_ARROW_RIGHT_A:
        case TILEKIND_ARROW_RIGHT_B:
            if (g == NULL || g->m_deathAnimStarted != 0) {
                return 1;
            }
            g->m_entranceActive = 1;
            g->StepArrivalDrop(x + 32, y, 0, -1, 1, 0);
            return 1;

        case TILEKIND_ARROW_DOWN_A:
        case TILEKIND_ARROW_DOWN_B:
            if (g == NULL || g->m_deathAnimStarted != 0) {
                return 1;
            }
            g->m_entranceActive = 1;
            g->StepArrivalDrop(x, y + 32, 0, -1, 1, 0);
            return 1;

        case TILEKIND_ARROW_LEFT_A:
        case TILEKIND_ARROW_LEFT_B:
            if (g == NULL || g->m_deathAnimStarted != 0) {
                return 1;
            }
            g->m_entranceActive = 1;
            g->StepArrivalDrop(x - 32, y, 0, -1, 1, 0);
            return 1;

        case TILEKIND_ARROW_CURRENT:
            if (g != NULL && g->m_deathAnimStarted == 0) {
                g->m_entranceActive = 1;
                switch (static_cast<GruntDirection>(g->m_entranceCell.direction)) {
                    case DIR_NORTH:
                        g->StepArrivalDrop(x, y - 32, 0, -1, 1, 0);
                        break;
                    case DIR_EAST:
                        g->StepArrivalDrop(x + 32, y, 0, -1, 1, 0);
                        break;
                    case DIR_SOUTH:
                        g->StepArrivalDrop(x, y + 32, 0, -1, 1, 0);
                        break;
                    case DIR_WEST:
                        g->StepArrivalDrop(x - 32, y, 0, -1, 1, 0);
                        break;
                }
                return 1;
            }

        case TILEKIND_CRUMBLEWATERBRIDGE: {
            CTileTriggerLogic* logic = state->m_beginMarker->AddLogicDefaults(
                tag,
                TRIGID_TILE_TRIGGER_24,
                x >> TILE_SHIFT_PX,
                y >> TILE_SHIFT_PX,
                0,
                0x9d,
                0,
                g_buteMgr.GetDword("Hazardz", "CrumbleTileDelay"),
                0
            );
            if (logic != NULL) {
                logic->RecordMove();
            }
            return 1;
        }

        case TILEKIND_CRUMBLEDEATHBRIDGE: {
            i32 token = 0x75;
            if (state->m_levelType > AREA_TILESET_A_LAST) {
                token = 0x72;
            }
            CTileTriggerLogic* logic = state->m_beginMarker->AddLogicDefaults(
                tag,
                TRIGID_TILE_TRIGGER_24,
                x >> TILE_SHIFT_PX,
                y >> TILE_SHIFT_PX,
                0,
                token,
                0,
                g_buteMgr.GetDword("Hazardz", "CrumbleTileDelay"),
                0
            );
            if (logic != NULL) {
                logic->RecordMove();
            }
            return 1;
        }

        case TILEKIND_CHECKPOINT:
            if (g_gameReg->m_gameMode != GAMEMODE_SINGLE || g == NULL
                || g->m_tileOwnerHi != g_curPlayer) {
                return 0;
            }
            sw = state->m_beginMarker->FindChild(
                ((x >> TILE_SHIFT_PX) * 0x100) + (y >> TILE_SHIFT_PX),
                TRIGID_CHECKPOINT_SWITCH_8
            );
            if (sw == NULL) {
                CString msg;
                msg.Format("No switch logic found for plate at: x=%d, y=%d", x, y);
                g_gameReg->EnterModalUI(static_cast<const char*>(msg));
                g_gameReg->ReportError(IDX(TRIGERR_LOOKUP_MISS), IDX(TRIGSITE_WIRE_CHECKPOINT));
                return 0;
            }
            if (sw->m_checkpointType == 0) {
                sw->SwitchDown();
            } else {
                PickupType gruntKind = ArrivalPickup(g);
                // m_checkpointType is a PickupType stored as i32 (declared in
                // TileTriggerSwitchLogic.h, fed from LevelTileValidation).
                if (IDX(gruntKind) == sw->m_checkpointType
                    || sw->m_checkpointType == IDX(g->m_vehiclePickupType)) {
                    sw->SwitchDown();
                } else {
                    RECT* view = &g_gameReg->m_world->m_level->m_mainPlane->m_viewRect;
                    i32 gx = g->m_object->m_screenX;
                    i32 gy = g->m_object->m_screenY;
                    if (CGameLevel::PointInRect(view, gx, gy)) {
                        g_gameReg->m_cueSink->SpawnVoiceDriver(g, 0x335, -1, 0, -1, -1);
                    }
                }
            }
            if (sw->VerifyBlockLinks() == 0) {
                return 0;
            }
            anyHit = 0;
            stop = 0;
            pos = state->m_beginMarker->m_list1.GetHeadPosition();
            while (pos != NULL && stop == 0) {
                CTileTriggerLogic* el =
                    static_cast<CTileTriggerLogic*>(state->m_beginMarker->m_list1.GetNext(pos));
                if (el->FindIndexByKey(sw->m_cellKey) != 0) {
                    if (el->Tick() == 0) {
                        stop = 1;
                    }
                    anyHit = 1;
                }
            }
            if (anyHit == 0) {
                CString msg;
                msg.Format("No trigger logic found for plate at: x=%d, y=%d", x, y);
                g_gameReg->EnterModalUI(static_cast<const char*>(msg));
                g_gameReg->ReportError(
                    IDX(TRIGERR_LINK_BROKEN),
                    IDX(TRIGSITE_WIRE_CHECKPOINT_TRIGGER)
                );
            }
            return 0;
    }
    return 0;
}

// @early-stop
RVA(0x0006d300, 0x5db)
i32 CTriggerMgr::ApplySwitch(CGrunt* g, i32 sx, i32 sy) {
    CPlay* state = static_cast<CPlay*>(g_gameReg->m_curState);
    CGameLevel* view = m_world->m_level;
    i32 x = sx;
    i32 y = sy;
    if (x < 0) {
        x = 0;
    } else {
        i32 w = view->m_mainPlane->m_wrapW;
        if (x >= w) {
            x = w - 1;
        }
    }
    if (y < 0) {
        y = 0;
    } else {
        i32 h = view->m_mainPlane->m_wrapH;
        if (y >= h) {
            y = h - 1;
        }
    }
    CDDrawWorkerHost* scroll = view->m_mainPlane;
    i32 sh = scroll->m_shiftX;
    i32 sw = scroll->m_shiftY;
    i32 tx = x >> sh;
    i32 ty = y >> sw;
    i32 subX = x - (tx << sh);
    i32 subY = y - (ty << sw);
    i32 attr = scroll->m_tileGrid[scroll->m_colOffsets[ty] + tx];
    TileCollisionKind kind;
    if (attr == UNINIT_FILL || attr == -1) {
        kind = TILEKIND_PASSABLE;
    } else {
        CTileImageSet* ts = static_cast<CTileImageSet*>(view->m_imageSets.GetAt(attr & 0xffff));
        // Ingest: the raw WWD attribute byte for this cell.
        kind = ts->GetCollisionAt(subX, subY);
    }
    switch (kind) {
        case TILEKIND_TIME_SWITCH_UP: {
            CTileTriggerSwitchLogic* obj = state->m_beginMarker->FindChild(
                ((sx >> TILE_SHIFT_PX) * 0x100) + (sy >> TILE_SHIFT_PX),
                TRIGID_TIME_SWITCH_7
            );
            if (obj == NULL) {
                CString msg;
                msg.Format("No switch logic found for switch at: x=%d, y=%d", sx, sy);
                g_gameReg->EnterModalUI(msg);
                g_gameReg->ReportError(IDX(TRIGERR_LOOKUP_MISS), IDX(TRIGSITE_APPLY_SWITCH_40));
                return 0;
            }
            obj->SwitchUp();
            return 1;
        }
        case TILEKIND_SWITCH_A_UP: {
            CTileTriggerSwitchLogic* obj = state->m_beginMarker->FindChild(
                ((sx >> TILE_SHIFT_PX) * 0x100) + (sy >> TILE_SHIFT_PX),
                TRIGID_ANY
            );
            if (obj == NULL) {
                CString msg;
                msg.Format("No switch logic found for switch at: x=%d, y=%d", sx, sy);
                g_gameReg->EnterModalUI(msg);
                g_gameReg->ReportError(IDX(TRIGERR_LOOKUP_MISS), IDX(TRIGSITE_APPLY_SWITCH_34));
                return 0;
            }
            obj->SwitchUp();
            return 1;
        }
        case TILEKIND_SWITCH_B_UP: {
            CTileTriggerSwitchLogic* obj = state->m_beginMarker->FindChild(
                ((sx >> TILE_SHIFT_PX) * 0x100) + (sy >> TILE_SHIFT_PX),
                TRIGID_ANY
            );
            if (obj == NULL) {
                CString msg;
                msg.Format("No switch logic found for switch at: x=%d, y=%d", sx, sy);
                g_gameReg->EnterModalUI(msg);
                g_gameReg->ReportError(IDX(TRIGERR_LOOKUP_MISS), IDX(TRIGSITE_APPLY_SWITCH_36));
                return 0;
            }
            obj->SwitchUp();

            POSITION pos = state->m_beginMarker->m_list1.GetHeadPosition();
            i32 found = 0;
            i32 stop = 0;
            while (pos != NULL) {
                if (stop != 0) {
                    break;
                }
                CTileTriggerLogic* child =
                    static_cast<CTileTriggerLogic*>(state->m_beginMarker->m_list1.GetNext(pos));
                if (child->FindIndexByKey(obj->m_cellKey) != 0) {
                    if (child->Tick() == 0) {
                        stop = 1;
                    }
                    found = 1;
                }
            }
            if (found == 0) {
                CString msg;
                msg.Format("No trigger logic found for switch at: x=%d, y=%d", sx, sy);
                g_gameReg->EnterModalUI(msg);
                g_gameReg->ReportError(IDX(TRIGERR_LINK_BROKEN), IDX(TRIGSITE_APPLY_TRIGGER_36));
                return 0;
            }
            return 1;
        }
        case TILEKIND_MULTI_SWITCH_UP: {
            CTileTriggerSwitchLogic* obj = state->m_beginMarker->FindChild(
                ((sx >> TILE_SHIFT_PX) * 0x100) + (sy >> TILE_SHIFT_PX),
                TRIGID_MULTI_SWITCH_3
            );
            if (obj == NULL) {
                CString msg;
                msg.Format("No switch logic found for switch at: x=%d, y=%d", sx, sy);
                g_gameReg->EnterModalUI(msg);
                g_gameReg->ReportError(IDX(TRIGERR_LOOKUP_MISS), IDX(TRIGSITE_APPLY_SWITCH_38));
                return 0;
            }
            i32 found = 0;
            if (obj->VerifyBlockLinksB() != 0) {
                POSITION pos = state->m_beginMarker->m_list1.GetHeadPosition();
                i32 stop = 0;
                while (pos != NULL) {
                    if (stop != 0) {
                        break;
                    }
                    CTileTriggerLogic* child =
                        static_cast<CTileTriggerLogic*>(state->m_beginMarker->m_list1.GetNext(pos));
                    if (child->FindIndexByKey(obj->m_cellKey) != 0) {
                        if (child->Tick() == 0) {
                            stop = 1;
                        }
                        found = 1;
                    }
                }
                if (found == 0) {
                    CString msg;
                    msg.Format("No trigger logic found for switch at: x=%d, y=%d", sx, sy);
                    g_gameReg->EnterModalUI(msg);
                    g_gameReg->ReportError(
                        IDX(TRIGERR_LINK_BROKEN),
                        IDX(TRIGSITE_APPLY_TRIGGER_38)
                    );
                    return 0;
                }
            }
            obj->SwitchUp();
            return 1;
        }
        case TILEKIND_CHECKPOINT_UP: {

            if (g_gameReg->m_gameMode != GAMEMODE_SINGLE) {
                return 0;
            }
            if (g == NULL) {
                return 0;
            }
            if (g->m_tileOwnerHi != g_curPlayer) {
                return 0;
            }
            CTileTriggerSwitchLogic* obj = state->m_beginMarker->FindChild(
                ((sx >> TILE_SHIFT_PX) * 0x100) + (sy >> TILE_SHIFT_PX),
                TRIGID_CHECKPOINT_SWITCH_8
            );
            if (obj == NULL) {
                CString msg;
                msg.Format("No switch logic found for switch at: x=%d, y=%d", sx, sy);
                g_gameReg->EnterModalUI(msg);
                g_gameReg->ReportError(IDX(TRIGERR_LOOKUP_MISS), IDX(TRIGSITE_APPLY_SWITCH_42));
                return 0;
            }
            if (obj->VerifyBlockLinks() != 0) {
                return 0;
            }
            obj->SwitchUp();
            return 0;
        }
    }
    return 0;
}

RVA(0x0006da60, 0x27)
void CTriggerMgr::GridAction6(i32 a, i32 b) {
    g_gameReg->m_cmdSubMgr
        ->EnqueueSingle(1, a, b, static_cast<char>(IDX(PLAYERCMD_GUARD_BEGIN)), 0, 0, 0, 0);
}

RVA(0x0006daa0, 0x27)
void CTriggerMgr::GridAction7(i32 a, i32 b) {
    g_gameReg->m_cmdSubMgr
        ->EnqueueSingle(1, a, b, static_cast<char>(IDX(PLAYERCMD_GUARD_END)), 0, 0, 0, 0);
}

// @early-stop
RVA(0x0006dae0, 0x4f9)
i32 CTriggerMgr::ApplyTriggerA(i32 col, i32 row, i32 worldX, i32 worldY) {
    CGrunt* cell = m_grid[col * TM_GRID_COLS + row];
    if (cell == NULL || cell->m_entranceCommitted == 0) {
        return 0;
    }
    i32 cellTileX = cell->LastTilePx().m_x >> TILE_SHIFT_PX;
    i32 cellTileY = cell->LastTilePx().m_y >> TILE_SHIFT_PX;
    i32 argTileX = worldX >> TILE_SHIFT_PX;
    i32 argTileY = worldY >> TILE_SHIFT_PX;
    CGameObject* o = cell->m_object;
    if (o->m_screenX != cell->m_lastTilePx.m_x) {
        return -1;
    }
    if (o->m_screenY != cell->m_lastTilePx.m_y) {
        return -1;
    }
    PickupType k = ArrivalPickup(cell);
    if (k == PICKUP_WAND && cell->CanShowStamina() != 0) {
        if (cellTileX != argTileX || cellTileY != argTileY) {
            return 0;
        }
        cell->RunMoveConfig(cellTileX, cellTileY + 1);
        return 1;
    }
    if (cellTileX == argTileX && cellTileY == argTileY) {
        PickupType kSame = ArrivalPickup(cell);
        if (kSame != PICKUP_SPY) {
            return 0;
        }
        if (cell->CanShowStamina() == 0) {
            return 0;
        }
        cell->RunMoveConfig(cellTileX, cellTileY);
        return 1;
    }
    PickupType kDiag = ArrivalPickup(cell);
    if (kDiag == PICKUP_BOMB) {

        if (cellTileY != argTileY && cellTileX != argTileX) {
            if (abs(argTileY - cellTileY) != abs(argTileX - cellTileX)) {
                return -1;
            }
        }
        if (cell->CanShowStamina() == 0) {
            return 0;
        }
        cell->RunMoveConfig(argTileX, argTileY);
        return 1;
    }
    i32 by = (worldY & ~TILE_MASK_PX) + TILE_HALF_PX;
    i32 bx = (worldX & ~TILE_MASK_PX) + TILE_HALF_PX;
    if (cell->RectContains(bx, by) == 0) {
        return -1;
    }
    cell->m_arrivalPhase = 0;
    i32 hitRow;
    i32 hitCol;
    CGrunt* hit = CellHitTest(worldX, worldY, &hitRow, &hitCol, TM_GRID_ROW_ALL);
    if (hit != NULL) {
        if (hit->m_tileOwnerHi == cell->m_tileOwnerHi && g_traitorMode == 0) {
            return 0;
        }
        return cell->CommitNeighbor(hitRow, hitCol, bx, by) != 0;
    }
    if (cell->CanShowStamina() == 0) {
        return 0;
    }
    CGruntzMapMgr* map = g_gameReg->m_tileGrid;
    TileCollisionKind bute = map->m_rows[by >> TILE_SHIFT_PX][bx >> TILE_SHIFT_PX].m_typeCode;
    PickupType kind = ArrivalPickup(cell);

    switch (kind) {
        case PICKUP_GAUNTLETZ:
            if (bute == TILEKIND_GAUNTLET_ROCK_A || bute == TILEKIND_GAUNTLET_ROCK_B
                || bute == TILEKIND_GIANT_ROCK || bute == TILEKIND_GAUNTLET_BRICK_A
                || bute == TILEKIND_GAUNTLET_BRICK_B || bute == TILEKIND_GAUNTLET_BRICK_C) {
                cell->RunMoveConfig(argTileX, argTileY);
                return 1;
            }
            return 0;
        case PICKUP_SHOVEL:
            if (bute == TILEKIND_COVERED_POWERUP || bute == TILEKIND_REVEALED_POWERUP) {
                cell->RunMoveConfig(argTileX, argTileY);
                return 1;
            }
            return 0;
        case PICKUP_GOOBER: {
            POSITION pos = m_baseList.GetHeadPosition();
            while (pos != NULL) {
                CGruntPuddle* cand = static_cast<CGruntPuddle*>(m_baseList.GetNext(pos));
                if (cand->m_pending == 0 && cand->m_tileX == argTileX
                    && cand->m_tileY == argTileY) {
                    cell->RunMoveConfig(argTileX, argTileY);
                    cand->m_value = cand->m_wwdObject->m_animCursor.m_animation;
                    cand->m_wwdObject->ApplyLookupGeometry("GRUNTZ_GRUNTPUDDLE_GRUNTPUDDLE3", 0);
                    cand->m_pending = 1;
                    return 1;
                }
            }
            return 0;
        }
        case PICKUP_SPY:
            cell->RunMoveConfig(cellTileX, cellTileY);
            return 1;
        case PICKUP_BRICK:
            if (bute == TILEKIND_HIDDEN_POWERUP || bute == TILEKIND_GAUNTLET_BRICK_A
                || bute == TILEKIND_GAUNTLET_BRICK_B) {
                cell->RunMoveConfig(argTileX, argTileY);
                return 1;
            }
            return 0;
        case PICKUP_BOOMERANG:
            return cell->BeginAttack(bx, by) != 0;
        case PICKUP_GUNHAT:
        case PICKUP_NERFGUN:
        case PICKUP_ROCK:
            return cell->BeginAttack(bx, by) != 0;
        case PICKUP_TIMEBOMB:
            return cell->BeginAttack(bx, by) != 0;
        case PICKUP_WELDER:
        case PICKUP_WINGZ:
            return cell->BeginAttack(bx, by) != 0;
        case PICKUP_WARPSTONE: {
            if (g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
                return 0;
            }
            i32 flags = map->CellFlagsAt(argTileX, argTileY);
            if ((flags & 0x40939) != 0 || (flags & 2) != 0) {
                return 0;
            }
            LoadPowerupIconSprites(PICKUP_WARPSTONE, bx, by, 0, cell->m_warpstoneAnchorIndex, 0);
            cell->PlayMoveSound(bx, by);
            if (cell->m_poweredUp != 0 && cell->m_neighborValid == 0) {
                RESET_GRUNT_POWERED_STATE(cell)
            }
            cell->LoadGruntTypeTable(PICKUP_NONE, 1, 0, 0);
            return 1;
        }
    }
    return 0;
}

// @early-stop
RVA(0x0006e120, 0x552)
i32 CTriggerMgr::ApplyTriggerB(i32 col, i32 row, i32 worldX, i32 worldY) {
    i32 bx;
    i32 by;
    CGrunt* hit;
    i32 moveKind;
    CString* typeRec;
    CString* slot;
    i32 grown;
    bool isG;
    bool isL;
    bool isP;
    bool isI2;
    CGrunt* cell = m_grid[col * TM_GRID_COLS + row];
    if (cell == NULL || cell->m_entranceCommitted == 0 || cell->m_entranceActive != 0) {
        return 0;
    }
    i32 cellTileY = cell->LastTilePx().m_y >> TILE_SHIFT_PX;
    i32 cellTileX = cell->LastTilePx().m_x >> TILE_SHIFT_PX;
    i32 argTileX = worldX >> TILE_SHIFT_PX;
    i32 argTileY = worldY >> TILE_SHIFT_PX;
    CGameObject* o = cell->m_object;
    if (o->m_screenX != cell->m_lastTilePx.m_x) {
        goto bad;
    }
    if (o->m_screenY != cell->m_lastTilePx.m_y) {
        return -1;
    }

    if (cellTileX == argTileX && cellTileY == argTileY && cell->m_vehiclePickupType != PICKUP_SCROLL
        && g_traitorMode == 0) {
        return 0;
    }
    by = (worldY & ~TILE_MASK_PX) + TILE_HALF_PX;
    bx = (worldX & ~TILE_MASK_PX) + TILE_HALF_PX;
    if (cell->RectContainsGated(bx, by) == 0) {
        goto bad;
    }

    cell->m_arrivalPhase = 0;
    i32 hitRow;
    i32 hitCol;
    hit = CellHitTest(worldX, worldY, &hitRow, &hitCol, TM_GRID_ROW_ALL);
    if (hit == NULL) {
        CGruntzMapMgr* map = g_gameReg->m_tileGrid;
        i32 flags = map->CellFlagsAt(argTileX, argTileY);
        if ((flags & 0x40939) != 0 || (flags & 0x82) != 0) {
            return 0;
        }

        PickupType kind = cell->m_vehiclePickupType;
        i32 moveKind = 0;
        if (kind == PICKUP_SCROLL) {
            moveKind = cell->m_moveKind;
        }
        if (LoadToyBoxIcon(bx, by, col, kind, moveKind) == 0) {
            return 0;
        }

        char* name = *g_typeColl.GetNameRecord(cell->m_objAux->m_actKey);
        bool isI = (strcmp(name, "I") == 0);
        if (isI) {
            LoadTileArrivalFx(
                col,
                row,
                cell->m_moveTile.m_x,
                cell->m_moveTile.m_y,
                cell->m_entranceReason,
                WWDDRAW_NO_ANIMATION
            );
        }
        cell->PlayMoveSound(bx, by);
        if (cell->m_poweredUp != 0 && cell->m_neighborValid == 0) {
            RESET_GRUNT_POWERED_STATE(cell)
        }
        cell->LoadVehicleGruntSprites(PICKUP_NONE);
        return 1;
    }

    if ((hit->m_lastTilePx.m_x != bx || hit->m_lastTilePx.m_y != by)
        && (hit->m_commitPx.m_x != bx || hit->m_commitPx.m_y != by)) {
        return 0;
    }

    // Retail holds each strcmp result in a `bool` before testing it (`sete cl /
    // test cl,cl`), five times over this function.
    isG = (ANIMATION_ACT_EQUALS_FOR(hit, "G"));
    if (isG) {
        return 0;
    }
    isL = (ANIMATION_ACT_EQUALS_FOR(hit, "L"));
    if (isL) {
        return 0;
    }
    isP = (ANIMATION_ACT_EQUALS_FOR(hit, "P"));
    if (isP) {
        return 0;
    }

    moveKind = 0;
    if (cell->m_vehiclePickupType == PICKUP_SCROLL) {
        moveKind = cell->m_moveKind;
    }
    cell->PlayMoveSound(bx, by);
    cell->m_neighborValid = 0;
    if (cell->m_poweredUp != 0) {
        RESET_GRUNT_POWERED_STATE(cell)
    }

    typeRec = g_typeColl.ScratchResolve(cell->m_objAux->m_actKey);
    slot = g_typeColl.Slots();
    grown = g_typeColl.m_grown;
    while (grown--) {
        if (slot != NULL) {
            slot->CString::CString();
        }
        slot++;
    }
    isI2 = (strcmp(*typeRec, "I") == 0);
    if (isI2) {
        LoadTileArrivalFx(
            col,
            row,
            cell->m_moveTile.m_x,
            cell->m_moveTile.m_y,
            cell->m_entranceReason,
            WWDDRAW_NO_ANIMATION
        );
    }
    if (hit->LoadGruntTypeTable(cell->m_vehiclePickupType, 1, moveKind, 0) != 0) {
        cell->LoadVehicleGruntSprites(PICKUP_NONE);

        if (hit->m_tileOwnerHi != col) {
            CGameObject* obj = cell->m_object;
            i32 sy = obj->m_screenY;
            i32 sx = obj->m_screenX;
            RECT* vr = &g_gameReg->m_world->m_level->m_mainPlane->m_viewRect;
            if (sx < vr->right && sx >= vr->left && sy < vr->bottom && sy >= vr->top) {
                g_gameReg->m_cueSink->SpawnVoiceDriver(cell, 0x38e, -1, 0, -1, -1);
            }
        }
        return 1;
    }
    return 0;
bad:
    return -1;
}

RVA(0x0006e7e0, 0x5)
CGrunt* CTriggerMgr::FindAtPixel(i32 x, i32 y) {
    return NULL;
}

// @early-stop
RVA(0x0006e800, 0x189)
i32 CTriggerMgr::ClearCell(i32 col, i32 row, i32 worldX, i32 worldY, i32 arrivalPhase) {
    i32 idx = col * TM_GRID_COLS + row;
    CGrunt* cell = m_grid[idx];
    if (cell == NULL || cell->m_entranceCommitted == 0) {
        return 0;
    }
    if (cell->m_tileClaimed != 0) {
        cell->m_arrivalRerollLo = 0;
        cell->m_arrivalRerollWindowLo = 0;
        cell->m_arrivalRerollHi = 0;
        cell->m_arrivalRerollWindowHi = 0;
        cell->m_arrivalFlags &= 0xe7fbfbfd;
        cell->m_tileClaimed = 0;
        cell->m_arrivalState = AI_NONE;
        cell->SetEntrancePos(1, 1);
    }
    if (cell->m_entranceActive != 0) {
        return 0;
    }
    CString* typeRec = g_typeColl.ScratchResolve(cell->m_objAux->m_actKey);
    CString* p = g_typeColl.Slots();
    i32 n = g_typeColl.m_grown;
    while (n--) {
        if (p != NULL) {
            p->CString::CString();
        }
        p++;
    }
    bool isI = (strcmp(*typeRec, "I") == 0);
    if (isI) {
        Coord t = cell->m_moveTile;
        this->LoadTileArrivalFx(
            col,
            row,
            t.m_x,
            t.m_y,
            cell->m_entranceReason,
            WWDDRAW_NO_ANIMATION
        );
    }
    i32 by = (worldY & ~TILE_MASK_PX) + TILE_HALF_PX;
    i32 bx = (worldX & ~TILE_MASK_PX) + TILE_HALF_PX;
    cell->m_coordRetryCount = 0;
    return cell->StepArrivalDrop(bx, by, arrivalPhase, -1, 1, 0) != 0;
}

// @early-stop
// residue: cl colours the inline-strcmp result into ecx (retail eax) and closes the
// bool with `cmp al,bl` against the zero register where retail writes `test cl,cl`;
// and it interleaves the two i64 zero stores (0x40,0x30,0x44,0x34) where retail keeps
// each pair contiguous. Every store ordering scores the same or lower.
RVA(0x0006ea00, 0x125)
void CTriggerMgr::HitTestApply(i32 x, i32 y, HitSpanArg span) {

    CGrunt* cell = FindGruntAt(x, y, span.m_span, &span.m_outCol, &y, NULL);
    if (cell == NULL || span.m_outCol != g_curPlayer) {
        return;
    }
    const char* name = *g_typeColl.GetNameRecord(cell->m_objAux->m_actKey);
    bool differ = strcmp(name, "B") != 0;
    if (!differ) {
        return;
    }
    PickupType k = ArrivalPickup(cell);
    if (k != PICKUP_WARPSTONE) {
        return;
    }
    CPlay* world = static_cast<CPlay*>(g_gameReg->m_curState);

    i64 diff = static_cast<i64>(g_frameTime) - world->m_frameMarker->m_startStamp.m_v;
    g_gameReg->m_scoreHud->m_elapsedTimeMs += (diff < 0) ? 0 : static_cast<i32>(diff);
    CTimer* sub = world->m_frameMarker;
    sub->m_unusedStamp.m_v = 0;
    sub->m_accum.m_v = 0;
    sub->m_running = 0;
    sub->m_currentMs = 0;
    world->ArmSnapshot(0, 0xbb7);
    world->m_guts->SetMode(1);
    this->ClearRow(g_curPlayer);
}
