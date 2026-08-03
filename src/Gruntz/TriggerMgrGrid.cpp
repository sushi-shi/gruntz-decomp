#include <AddrWord.h>
#include <Bute/ButeMgr.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawSubMgrLeafScan.h>
#include <Enums.h>
#include <Gruntz/ActionOptionsMenuBar.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/BattlezData.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntDeathType.h>
#include <Gruntz/GruntPuddle.h>
#include <Gruntz/GruntSpawnConfig.h>
#include <Gruntz/GruntzCmdMgr.h>
#include <Gruntz/GruntzCommandId.h>
#include <Gruntz/GruntzMapMgr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LeafCue.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/Play.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/StatusBarMgr.h>
#include <Gruntz/TileGrid.h>
#include <Gruntz/TileTriggerContainer.h>
#include <Gruntz/TileTriggerLogic.h>
#include <Gruntz/TileTriggerSwitchLogic.h>
#include <Gruntz/Timer.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TriggerMgrRecords.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/UserLogic.h>
#include <Wwd/WwdFile.h>

RVA(0x0006b640, 0x2f)
i32 CTriggerMgr::SetLevel(CDDrawSurfaceMgr* lvl) {
    if (lvl == 0) {
        return 0;
    }
    m_world = lvl;
    m_armed = 0;
    m_pendingFx = 0;
    m_countdownActive = 1;
    return 1;
}

RVA(0x0006b680, 0x39)
void CTriggerMgr::Cleanup() {
    CActionOptionsMenuBar* ov = m_overlay;
    if (ov != 0) {
        ov->Clear();
        operator delete(ov);
        m_overlay = 0;
    }
    ClearRecords();
    ClearSelections();
}

// @early-stop
RVA(0x0006b6d0, 0x3f4)
i32 CTriggerMgr::PlaceObject(
    i32 row,
    i32 x,
    i32 y,
    i32 z,
    i32 mode,
    i32 kindDefault,
    i32 typeKind,
    i32 vehicleKind,
    i32 aiType,
    i32 aiRadius,
    i32 placeArg9,
    i32 placeArg10,
    i32 spanWord
) {

    {
        CDDrawSurfaceMgr* world = m_world;
        if (world == 0) {
            goto fail;
        }
        i32 wantSlot = 0;
        i32 special = 0;
        if (typeKind == 0x12) {
            special = 0x100;
            wantSlot = 1;
        }
        CGruntzMapMgr* plane = g_gameReg->m_tileGrid;
        i32 tx = x >> 5;
        i32 ty = y >> 5;
        i32 attr;
        if (static_cast<u32>(tx) >= static_cast<u32>(plane->m_width)
            || static_cast<u32>(ty) >= static_cast<u32>(plane->m_height)) {
            attr = 1;
        } else {
            attr = plane->m_rowInts[ty][tx * 7];
        }
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
            if (mode != col) {
                goto fail;
            }
        } else {
            onSpecialTile = 0;
        }

        i32 base = row * TM_GRID_COLS;
        if (m_grid[base] != 0) {

            CGrunt** cells = &m_grid[row * TM_GRID_COLS];
            do {
                if (col >= TM_GRID_COLS) {
                    goto fail;
                }
                cells++;
                col++;
            } while (*cells != 0);
        }
        if (col >= TM_GRID_COLS) {
            goto fail;
        }

        CWwdGameObjectA* sprite = m_world->m_childGroup->CreateSprite(0, x, y, z, "Grunt", 0x40003);
        if (sprite == 0) {
            goto fail;
        }
        sprite->m_animWorker->m_notify(sprite);
        CGrunt* logic = static_cast<CGrunt*>(sprite->m_animWorker->m_logic);

        // NOT a PickupType local: the AI-type switch fills it with tool ids,
        // but the player-slot path below overwrites it with m_colorIndex, so it
        // carries two domains. Converted explicitly where it enters Place().
        i32 kindId;
        if (g_gameReg->m_gameMode == 1) {
            switch (aiType) {
                case 1:
                    kindId = IDX(PICKUP_BOMB);
                    break;
                case 2:
                    kindId = IDX(PICKUP_GUNHAT);
                    break;
                case 3:
                    kindId = IDX(PICKUP_GAUNTLETZ);
                    break;
                case 4:
                    kindId = IDX(PICKUP_CLUB);
                    break;
                case 5:
                    kindId = IDX(PICKUP_SHIELD);
                    break;
                case 6:
                    kindId = IDX(PICKUP_GLOVEZ);
                    break;
                case 7:
                    kindId = IDX(PICKUP_BRICK);
                    typeKind = 1;
                    break;
                case 8:
                    kindId = IDX(PICKUP_GRAVITYBOOTZ);
                    typeKind = 3;
                    break;
                case 10:
                    kindId = IDX(PICKUP_SPY);
                    typeKind = 7;
                    break;
                case 11:
                    kindId = IDX(PICKUP_NERFGUN);
                    typeKind = 13;
                    break;
                case 9:
                    kindId = IDX(PICKUP_BOOMERANG);
                    typeKind = 5;
                    break;
                case 13:
                    kindId = IDX(PICKUP_GOOBER);
                    break;
                case 14:
                    kindId = IDX(PICKUP_SWORD);
                    break;
                case 12:
                    kindId = IDX(PICKUP_ROCK);
                    typeKind = 17;
                    break;
                case 15:
                    kindId = IDX(PICKUP_SHOVEL);
                    typeKind = 19;
                    break;
                case 16:
                    kindId = IDX(PICKUP_SHOVEL);
                    vehicleKind = 30;
                    break;
                default:
                    kindId = kindDefault;
                    break;
            }
        } else {
            kindId = kindDefault;
        }

        GruntzPlayer* slot = &g_gameReg->m_options[row];
        if (m_rowCount[row] >= slot->m_comboSel) {
            goto fail;
        }
        if (slot->m_liveGate != 0
            || (row != g_curPlayer && kindId == g_gameReg->m_options[g_curPlayer].m_colorIndex)) {
            kindId = slot->m_colorIndex;
        }
        if (row == g_curPlayer && aiType != 0) {
            aiType = 0;
        }

        AddrWord<RECT> span;
        span.m_word = spanWord;
        if (logic->Place(
                this,
                row,
                col,
                static_cast<PickupType>(kindId),
                static_cast<PickupType>(typeKind),
                vehicleKind,
                aiType,
                aiRadius,
                placeArg9,
                placeArg10,
                span.m_rect,
                mode
            )
            == 0) {
            logic->m_wwdObject->m_flags |= 0x10000;
            return -1;
        }

        if (mode == 1) {
            CWwdGameObjectA* hole =
                m_world->m_childGroup->CreateSprite(0, x, y, 0, "Wormhole", 0x40003);
            if (hole == 0) {
                logic->m_wwdObject->m_flags |= 0x10000;
                return -1;
            }
            hole->m_smarts = g_buteMgr.GetIntDef("Wormhole", "EntranceColor", 0xe);
        } else if (mode == 3 || mode == 2) {

            if (mode == 3) {
                logic->m_health = 0x19;
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
        g_gameReg->m_scoreHud->m_counts[row] += 1;
        return col;
    }
fail:
    return -1;
}

RVA(0x0006bc20, 0x6f)
i32 CTriggerMgr::DispatchCellForObject(CGrunt* obj, i32 startRow, GruntDeathType kind, i32 arg) {
    i32 last;
    if (startRow == 5) {
        startRow = 0;
        last = 3;
    } else {
        last = startRow;
    }
    for (i32 row = startRow; row <= last; row++) {
        CGrunt** cell = &m_grid[row * TM_GRID_COLS];
        for (i32 col = 0; col < 15; col++) {
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
    if (cell == 0) {
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

// @early-stop
RVA(0x0006bd40, 0xb3)
i32 CTriggerMgr::ClearGridRange(i32 startRow) {
    i32 row, last;
    if (startRow == 5) {
        row = 0;
        last = 3;
    } else {
        last = startRow;
        row = startRow;
    }
    ResetAll();
    if (row <= last) {
        i32 n = last - row + 1;
        CGrunt** cell = &m_grid[row * TM_GRID_COLS];

        i32 r = row;
        i32 g2 = row * TM_GRID_COLS;
        do {
            i32 col = 0;
            do {
                CGrunt* c = *cell;
                if (c != 0) {
                    c->m_wwdObject->m_flags |= 0x10000;
                    *cell = 0;
                    m_cellFlag[g2 + col] = 0;
                }
                col++;
                cell++;
            } while (col < 15);
            m_rowCount[r] = 0;
            m_gruntzExitedByPlayer[r] = 0;
            m_gruntzLostByPlayer[r] = 0;
            r++;
            g2 += 15;
            n--;
        } while (n != 0);
    }
    ClearSelections();
    return 1;
}

// @early-stop
RVA(0x0006be30, 0x47)
CGrunt* CTriggerMgr::ScreenToCell(i32 sx, i32 sy, i32* outRow, i32* outCol, i32 startRow) {
    CGameLevel* view = m_world->m_level;
    i32 px = view->m_mainPlane->m_viewRect.left - view->m_planeCtx.left + sx;
    i32 py = view->m_mainPlane->m_viewRect.top - view->m_planeCtx.top + sy;
    return CellHitTest(px, py, outRow, outCol, startRow);
}

// @early-stop
RVA(0x0006bea0, 0xe2)
CGrunt* CTriggerMgr::CellHitTest(i32 px, i32 py, i32* outRow, i32* outCol, i32 startRow) {
    i32 row, last;
    if (startRow == 5) {
        row = 0;
        last = 3;
    } else {
        last = startRow;
        row = startRow;
    }

    if (row <= last) {
        do {
            CGrunt** cell = &m_grid[row * TM_GRID_COLS];
            for (i32 col = 0; col < 15; col++) {
                CGrunt* g = cell[col];
                if (g != 0 && g->m_entranceCommitted != 0) {
                    CWwdGameObjectA* o = g->m_object;
                    if (o->m_layer != 0) {
                        i32 x0 = o->m_screenX - 15;
                        i32 y0 = o->m_screenY - 15;
                        if (px < x0 + 30 && px >= x0 && py < y0 + 30 && py >= y0) {
                            if (outRow != 0) {
                                *outRow = row;
                            }
                            if (outCol != 0) {
                                *outCol = col;
                            }
                            return m_grid[row * TM_GRID_COLS + col];
                        }
                    }
                }
            }
            row++;
        } while (row <= last);
    }
    return 0;
}

RVA(0x0006bfd0, 0x106)
i32 CTriggerMgr::ResetCell(i32 col, i32 row, i32 force, i32 keep) {
    i32 idx = col * TM_GRID_COLS + row;
    CGrunt* cell = m_grid[idx];
    if (cell == 0 || cell->m_entranceCommitted == 0) {
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
    Coord* slot = 0;
    if (node->m_next != 0) {
        slot = &node->m_coord;
        slot->m_x = col;
        slot->m_y = row;
        g_coordPool.m_freeHead = g_coordPool.m_freeHead->m_next;
    }
    m_recList.AddTail(slot);

    return cell->CommitArrival();
}

RVA(0x0006c130, 0xd62)
i32 CTriggerMgr::WireTileSwitchLogic(CGrunt* g, i32 x, i32 y) {

    CPlay* state = static_cast<CPlay*>(g_gameReg->m_curState);

    if (g != 0) {
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
    if (raw == static_cast<i32>(0xeeeeeeee) || raw == -1) {
        tag = TILEKIND_PASSABLE;
    } else {
        CTileImageSet* ts = static_cast<CTileImageSet*>(level->m_imageSets.GetAt(raw & 0xffff));
        // Ingest: the raw WWD attribute byte for this cell.
        tag = static_cast<TileCollisionKind>(ts->GetCollisionAt(subX, subY));
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
                ((x >> 5) * 0x100) + (y >> 5),
                TRIGID_TIME_SWITCH_7
            );
            if (sw == 0) {
                CString msg;
                msg.Format("No switch logic found for switch at: x=%d, y=%d", x, y);
                g_gameReg->EnterModalUI(static_cast<const char*>(msg));
                g_gameReg->ReportError(IDX(TRIGERR_LOOKUP_MISS), IDX(TRIGSITE_WIRE_TIME_SWITCH));
                return 0;
            }
            sw->SwitchDown();
            pos = state->m_beginMarker->m_list2.GetHeadPosition();
            while (pos != 0) {
                CTileTriggerLogic* el =
                    static_cast<CTileTriggerLogic*>(state->m_beginMarker->m_list2.GetNext(pos));
                if (el->FindIndexByKey(sw->m_cellKey) != 0) {
                    return 1;
                }
            }
            anyHit = 0;
            pos = state->m_beginMarker->m_list1.GetHeadPosition();
            while (pos != 0) {
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
                ((x >> 5) * 0x100) + (y >> 5),
                TRIGID_SECRET_SWITCH_6
            );
            if (sw == 0) {
                CString msg;
                msg.Format("No switch logic found for switch at: x=%d, y=%d", x, y);
                g_gameReg->EnterModalUI(static_cast<const char*>(msg));
                g_gameReg->ReportError(IDX(TRIGERR_LOOKUP_MISS), IDX(TRIGSITE_WIRE_SECRET_SWITCH));
                return 0;
            }
            sw->SwitchDown();
            anyHit = 0;
            pos = state->m_beginMarker->m_list1.GetHeadPosition();
            while (pos != 0) {
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
                        void* found = 0;
                        set->m_cues.Lookup("GAME_SECRETSWITCH", found);
                        if (found != 0) {
                            static_cast<LeafCue*>(found)->PlayIfElapsed(g_sndCueTag, 0, 0, 0);
                        }
                    }
                }
                if (g != 0) {
                    i32 cueX = g->m_object->m_screenX;
                    i32 cueY = g->m_object->m_screenY;
                    if (cueX < g_gameReg->m_viewBounds.right && cueX >= g_gameReg->m_viewBounds.left
                        && cueY < g_gameReg->m_viewBounds.bottom
                        && cueY >= g_gameReg->m_viewBounds.top) {
                        g_gameReg->m_cueSink->SpawnVoiceDriver(g, 0x3f2, -1, 0, -1, -1);
                    }
                } else if (x < g_gameReg->m_viewBounds.right && x >= g_gameReg->m_viewBounds.left
                           && y < g_gameReg->m_viewBounds.bottom
                           && y >= g_gameReg->m_viewBounds.top) {
                    g_gameReg->m_cueSink->SpawnVoiceDriver(0, 0x3f2, -1, 1, -1, -1);
                }
            }
            return 1;

        case TILEKIND_SWITCH_A:
        case TILEKIND_SWITCH_B:
        case TILEKIND_SWITCH_C:
            sw = state->m_beginMarker->FindChild(((x >> 5) * 0x100) + (y >> 5), TRIGID_ANY);
            if (sw == 0) {
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
            while (pos != 0 && stop == 0) {
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
                ((x >> 5) * 0x100) + (y >> 5),
                TRIGID_MULTI_SWITCH_3
            );
            if (sw == 0) {
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
            while (pos != 0 && stop == 0) {
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
                ((x >> 5) * 0x100) + (y >> 5),
                TRIGID_EXCLUSIVE_SWITCH_4
            );
            if (sw == 0) {
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
            while (pos != 0 && stop == 0) {
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
            if (g == 0 || g->m_deathAnimStarted != 0) {
                return 1;
            }
            g->m_entranceActive = 1;
            g->StepArrivalDrop(x, y - 32, 0, -1, 1, 0);
            return 1;

        case TILEKIND_ARROW_RIGHT_A:
        case TILEKIND_ARROW_RIGHT_B:
            if (g == 0 || g->m_deathAnimStarted != 0) {
                return 1;
            }
            g->m_entranceActive = 1;
            g->StepArrivalDrop(x + 32, y, 0, -1, 1, 0);
            return 1;

        case TILEKIND_ARROW_DOWN_A:
        case TILEKIND_ARROW_DOWN_B:
            if (g == 0 || g->m_deathAnimStarted != 0) {
                return 1;
            }
            g->m_entranceActive = 1;
            g->StepArrivalDrop(x, y + 32, 0, -1, 1, 0);
            return 1;

        case TILEKIND_ARROW_LEFT_A:
        case TILEKIND_ARROW_LEFT_B:
            if (g == 0 || g->m_deathAnimStarted != 0) {
                return 1;
            }
            g->m_entranceActive = 1;
            g->StepArrivalDrop(x - 32, y, 0, -1, 1, 0);
            return 1;

        case TILEKIND_ARROW_CURRENT:
            if (g != 0 && g->m_deathAnimStarted == 0) {
                g->m_entranceActive = 1;
                switch (g->m_entranceCell.direction) {
                    case 1:
                        g->StepArrivalDrop(x, y - 32, 0, -1, 1, 0);
                        break;
                    case 3:
                        g->StepArrivalDrop(x + 32, y, 0, -1, 1, 0);
                        break;
                    case 5:
                        g->StepArrivalDrop(x, y + 32, 0, -1, 1, 0);
                        break;
                    case 7:
                        g->StepArrivalDrop(x - 32, y, 0, -1, 1, 0);
                        break;
                    default:
                        g->StepArrivalDrop(x, y, 0, -1, 1, 0);
                        break;
                }
                return 1;
            }

        case TILEKIND_CRUMBLEWATERBRIDGE: {
            CTileTriggerLogic* logic = state->m_beginMarker->AddLogicDefaults(
                tag,
                TRIGID_TILE_TRIGGER_24,
                x >> 5,
                y >> 5,
                0,
                0x9d,
                0,
                g_buteMgr.GetDword("Hazardz", "CrumbleTileDelay"),
                0
            );
            if (logic != 0) {
                logic->RecordMove();
            }
            return 1;
        }

        case TILEKIND_CRUMBLEDEATHBRIDGE: {
            i32 token = state->m_levelType > 4 ? 0x72 : 0x75;
            CTileTriggerLogic* logic = state->m_beginMarker->AddLogicDefaults(
                tag,
                TRIGID_TILE_TRIGGER_24,
                x >> 5,
                y >> 5,
                0,
                token,
                0,
                g_buteMgr.GetDword("Hazardz", "CrumbleTileDelay"),
                0
            );
            if (logic != 0) {
                logic->RecordMove();
            }
            return 1;
        }

        case TILEKIND_CHECKPOINT:
            if (g_gameReg->m_gameMode != 1 || g == 0 || g->m_tileOwnerHi != g_curPlayer) {
                return 0;
            }
            sw = state->m_beginMarker->FindChild(
                ((x >> 5) * 0x100) + (y >> 5),
                TRIGID_CHECKPOINT_SWITCH_8
            );
            if (sw == 0) {
                CString msg;
                msg.Format("No switch logic found for plate at: x=%d, y=%d", x, y);
                g_gameReg->EnterModalUI(static_cast<const char*>(msg));
                g_gameReg->ReportError(IDX(TRIGERR_LOOKUP_MISS), IDX(TRIGSITE_WIRE_CHECKPOINT));
                return 0;
            }
            if (sw->m_checkpointType == 0) {
                sw->SwitchDown();
            } else {
                PickupType gruntKind = g->m_entranceReason;
                if (gruntKind > PICKUP_TOOLZ_LAST) {
                    gruntKind = g->m_toolId;
                }
                // m_checkpointType is a PickupType stored as i32 (declared in
                // TileTriggerSwitchLogic.h, fed from LevelTileValidation).
                if (IDX(gruntKind) == sw->m_checkpointType
                    || sw->m_checkpointType == IDX(g->m_vehiclePickupType)) {
                    sw->SwitchDown();
                } else {
                    RECT* view = &g_gameReg->m_world->m_level->m_mainPlane->m_viewRect;
                    i32 gx = g->m_object->m_screenX;
                    i32 gy = g->m_object->m_screenY;
                    if (gx < view->right && gx >= view->left && gy < view->bottom
                        && gy >= view->top) {
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
            while (pos != 0 && stop == 0) {
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
RVA(0x0006d300, 0x5b2)
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
    if (attr == static_cast<i32>(0xeeeeeeee) || attr == -1) {
        kind = TILEKIND_PASSABLE;
    } else {
        CTileImageSet* ts = static_cast<CTileImageSet*>(view->m_imageSets.GetAt(attr & 0xffff));
        // Ingest: the raw WWD attribute byte for this cell.
        kind = static_cast<TileCollisionKind>(ts->GetCollisionAt(subX, subY));
    }
    switch (kind) {
        case TILEKIND_TIME_SWITCH_UP: {
            CTileTriggerSwitchLogic* obj = state->m_beginMarker->FindChild(
                ((sx >> 5) * 0x100) + (sy >> 5),
                TRIGID_TIME_SWITCH_7
            );
            if (obj == 0) {
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
            CTileTriggerSwitchLogic* obj =
                state->m_beginMarker->FindChild(((sx >> 5) * 0x100) + (sy >> 5), TRIGID_ANY);
            if (obj == 0) {
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
            CTileTriggerSwitchLogic* obj =
                state->m_beginMarker->FindChild(((sx >> 5) * 0x100) + (sy >> 5), TRIGID_ANY);
            if (obj == 0) {
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
            while (pos != 0) {
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
                ((sx >> 5) * 0x100) + (sy >> 5),
                TRIGID_MULTI_SWITCH_3
            );
            if (obj == 0) {
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
                while (pos != 0) {
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

            if (g_gameReg->m_gameMode != 1) {
                return 0;
            }
            if (g == 0) {
                return 0;
            }
            if (g->m_tileOwnerHi != g_curPlayer) {
                return 0;
            }
            CTileTriggerSwitchLogic* obj = state->m_beginMarker->FindChild(
                ((sx >> 5) * 0x100) + (sy >> 5),
                TRIGID_CHECKPOINT_SWITCH_8
            );
            if (obj == 0) {
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
    g_gameReg->m_cmdSubMgr->EnqueueSingle(1, a, b, 6, 0, 0, 0, 0);
}

RVA(0x0006daa0, 0x27)
void CTriggerMgr::GridAction7(i32 a, i32 b) {
    g_gameReg->m_cmdSubMgr->EnqueueSingle(1, a, b, 7, 0, 0, 0, 0);
}

// @early-stop
RVA(0x0006dae0, 0x4b7)
i32 CTriggerMgr::ApplyTriggerA(i32 col, i32 row, i32 worldX, i32 worldY) {
    CGrunt* cell = m_grid[col * TM_GRID_COLS + row];
    if (cell == 0 || cell->m_entranceCommitted == 0) {
        return 0;
    }
    i32 cellTileX = cell->m_lastTilePx.m_x >> 5;
    i32 cellTileY = cell->m_lastTilePx.m_y >> 5;
    i32 argTileX = worldX >> 5;
    i32 argTileY = worldY >> 5;
    CGameObject* o = cell->m_object;
    if (o->m_screenX != cell->m_lastTilePx.m_x) {
        return -1;
    }
    if (o->m_screenY != cell->m_lastTilePx.m_y) {
        return -1;
    }
    PickupType k = cell->m_entranceReason;
    if (k > PICKUP_TOOLZ_LAST) {
        k = cell->m_toolId;
    }
    if (k == PICKUP_WAND && cell->CanShowStamina() != 0) {
        if (cellTileX != argTileX || cellTileY != argTileY) {
            return 0;
        }
        cell->RunMoveConfig(cellTileX, cellTileY + 1);
        return 1;
    }
    if (cellTileX == argTileX && cellTileY == argTileY) {
        PickupType kSame = cell->m_entranceReason;
        if (kSame > PICKUP_TOOLZ_LAST) {
            kSame = cell->m_toolId;
        }
        if (kSame != PICKUP_SPY) {
            return 0;
        }
        if (cell->CanShowStamina() == 0) {
            return 0;
        }
        cell->RunMoveConfig(argTileX, argTileY);
        return 1;
    }
    PickupType kDiag = cell->m_entranceReason;
    if (kDiag > PICKUP_TOOLZ_LAST) {
        kDiag = cell->m_toolId;
    }
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
    i32 by = (worldY & ~0x1f) + 0x10;
    i32 bx = (worldX & ~0x1f) + 0x10;
    if (cell->RectContains(bx, by) == 0) {
        return -1;
    }
    cell->m_arrivalPhase = 0;
    i32 hitRow;
    i32 hitCol;
    CGrunt* hit = CellHitTest(worldX, worldY, &hitRow, &hitCol, 5);
    if (hit != 0) {
        if (hit->m_tileOwnerHi == cell->m_tileOwnerHi && g_traitorMode == 0) {
            return 0;
        }
        return cell->CommitNeighbor(hitRow, hitCol, bx, by) != 0;
    }
    if (cell->CanShowStamina() == 0) {
        return 0;
    }
    CGruntzMapMgr* map = g_gameReg->m_tileGrid;
    i32 bute = map->m_rows[by >> 5][bx >> 5].m_typeCode;
    PickupType kind = cell->m_entranceReason;
    if (kind > PICKUP_TOOLZ_LAST) {
        kind = cell->m_toolId;
    }

    switch (kind) {
        case PICKUP_GAUNTLETZ:
            if (bute == 0x1e || bute == 0x1f || bute == 0x21 || bute == 0x97 || bute == 0x98
                || bute == 0x99) {
                cell->RunMoveConfig(argTileX, argTileY);
                return 1;
            }
            return 0;
        case PICKUP_SHOVEL:
            if (bute == 0x22 || bute == 0x23) {
                cell->RunMoveConfig(argTileX, argTileY);
                return 1;
            }
            return 0;
        case PICKUP_GOOBER: {
            POSITION pos = m_baseList.GetHeadPosition();
            while (pos != 0) {
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
            if (bute == 0x96 || bute == 0x97 || bute == 0x98) {
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
            if (g_gameReg->m_gameMode == 1) {
                return 0;
            }
            i32 flags = 1;
            if (static_cast<u32>(argTileX) < map->m_width
                && static_cast<u32>(argTileY) < map->m_height) {
                flags = map->m_rows[argTileY][argTileX].m_flags;
            }
            if ((flags & 0x40939) != 0 || (flags & 2) != 0) {
                return 0;
            }
            LoadPowerupIconSprites(PICKUP_WARPSTONE, bx, by, 0, cell->m_warpstoneAnchorIndex, 0);
            cell->PlayMoveSound(bx, by);
            if (cell->m_poweredUp != 0 && cell->m_neighborValid == 0) {
                cell->m_entranceActive = 0;
                cell->m_combatActive = 0;
                cell->m_neighborValid = 0;
                cell->m_poweredUp = 0;
                cell->ResetEntranceAnimation(1, 0, 0);
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
    CGrunt* cell = m_grid[col * TM_GRID_COLS + row];
    if (cell == 0 || cell->m_entranceCommitted == 0 || cell->m_entranceActive != 0) {
        return 0;
    }
    i32 cellTileX = cell->m_lastTilePx.m_x >> 5;
    i32 cellTileY = cell->m_lastTilePx.m_y >> 5;
    i32 argTileX = worldX >> 5;
    i32 argTileY = worldY >> 5;
    CGameObject* o = cell->m_object;
    if (o->m_screenX != cell->m_lastTilePx.m_x) {
        return -1;
    }
    if (o->m_screenY != cell->m_lastTilePx.m_y) {
        return -1;
    }

    if (cellTileX == argTileX && cellTileY == argTileY && cell->m_vehiclePickupType != PICKUP_SCROLL
        && g_traitorMode == 0) {
        return 0;
    }
    i32 by = (worldY & ~0x1f) + 0x10;
    i32 bx = (worldX & ~0x1f) + 0x10;
    if (cell->RectContainsGated(bx, by) == 0) {
        return -1;
    }

    cell->m_arrivalPhase = 0;
    i32 hitRow;
    i32 hitCol;
    CGrunt* hit = CellHitTest(worldX, worldY, &hitRow, &hitCol, 5);
    if (hit == 0) {
        CGruntzMapMgr* map = g_gameReg->m_tileGrid;
        i32 flags = 1;
        if (static_cast<u32>(argTileX) < map->m_width
            && static_cast<u32>(argTileY) < map->m_height) {
            flags = map->m_rows[argTileY][argTileX].m_flags;
        }
        if ((flags & 0x40939) != 0 || (flags & 0x82) != 0) {
            return 0;
        }

        PickupType kind = cell->m_vehiclePickupType;
        i32 moveKind = kind == PICKUP_SCROLL ? cell->m_moveKind : 0;
        if (LoadToyBoxIcon(bx, by, col, kind, moveKind) == 0) {
            return 0;
        }

        char* name = *g_typeColl.GetNameRecord(cell->m_objAux->m_actKey);
        if (strcmp(name, "I") == 0) {
            LoadTileArrivalFx(
                col,
                row,
                cell->m_moveTile.m_x,
                cell->m_moveTile.m_y,
                cell->m_entranceReason,
                -1
            );
        }
        cell->PlayMoveSound(bx, by);
        if (cell->m_poweredUp != 0 && cell->m_neighborValid == 0) {
            cell->m_entranceActive = 0;
            cell->m_combatActive = 0;
            cell->m_neighborValid = 0;
            cell->m_poweredUp = 0;
            cell->ResetEntranceAnimation(1, 0, 0);
        }
        cell->LoadVehicleGruntSprites(PICKUP_NONE);
        return 1;
    }

    if ((hit->m_lastTilePx.m_x != bx || hit->m_lastTilePx.m_y != by)
        && (hit->m_commitPx.m_x != bx || hit->m_commitPx.m_y != by)) {
        return 0;
    }

    char* hitName = *g_typeColl.GetNameRecord(hit->m_objAux->m_actKey);
    if (strcmp(hitName, "G") == 0 || strcmp(hitName, "L") == 0 || strcmp(hitName, "P") == 0) {
        return 0;
    }

    PickupType kind = cell->m_vehiclePickupType;
    i32 moveKind = kind == PICKUP_SCROLL ? cell->m_moveKind : 0;
    cell->PlayMoveSound(bx, by);
    cell->m_neighborValid = 0;
    if (cell->m_poweredUp != 0) {
        cell->m_entranceActive = 0;
        cell->m_combatActive = 0;
        cell->m_neighborValid = 0;
        cell->m_poweredUp = 0;
        cell->ResetEntranceAnimation(1, 0, 0);
    }

    char* name = *g_typeColl.GetNameRecord(cell->m_objAux->m_actKey);
    if (strcmp(name, "I") == 0) {
        LoadTileArrivalFx(
            col,
            row,
            cell->m_moveTile.m_x,
            cell->m_moveTile.m_y,
            cell->m_entranceReason,
            -1
        );
    }
    if (hit->LoadGruntTypeTable(kind, 1, moveKind, 0) == 0) {
        return 0;
    }
    cell->LoadVehicleGruntSprites(PICKUP_NONE);

    if (hit->m_tileOwnerHi != col) {
        CGameObject* obj = cell->m_object;
        CDDrawWorkerHost* plane = g_gameReg->m_world->m_level->m_mainPlane;
        if (obj->m_screenX >= plane->m_viewRect.left && obj->m_screenX < plane->m_viewRect.right
            && obj->m_screenY >= plane->m_viewRect.top
            && obj->m_screenY < plane->m_viewRect.bottom) {
            g_gameReg->m_cueSink->SpawnVoiceDriver(cell, 0x38e, -1, 0, -1, -1);
        }
    }
    return 1;
}

RVA(0x0006e7e0, 0x5)
CGrunt* CTriggerMgr::FindAtPixel(i32 x, i32 y) {
    return 0;
}

// @early-stop
RVA(0x0006e800, 0x189)
i32 CTriggerMgr::ClearCell(i32 col, i32 row, i32 arrivalPhase, i32 worldX, i32 worldY) {
    i32 idx = col * TM_GRID_COLS + row;
    CGrunt* cell = m_grid[idx];
    if (cell == 0 || cell->m_entranceCommitted == 0) {
        return 0;
    }
    if (cell->m_tileClaimed == 0) {
        cell->m_arrivalRerollLo = 0;
        cell->m_arrivalRerollWindowLo = 0;
        cell->m_arrivalRerollHi = 0;
        cell->m_arrivalRerollWindowHi = 0;
        cell->m_arrivalFlags &= 0xe7fbfbfd;
        cell->m_tileClaimed = 0;
        cell->m_arrivalState = 0;
        cell->SetEntrancePos(1, 1);
    }
    if (cell->m_entranceActive != 0) {
        return 0;
    }
    const char* name = *g_typeColl.ScratchResolve(cell->m_objAux->m_actKey);
    if (strcmp(name, "I") == 0) {
        i32 px = cell->m_moveTile.m_x;
        i32 py = cell->m_moveTile.m_y;
        // NOTE: argument shape differs from the other five LoadTileArrivalFx
        // call sites - m_entranceReason lands in `tileY`, not in `reason`.
        this->LoadTileArrivalFx(px, py, py, IDX(cell->m_entranceReason), PICKUP_INVALID, py);
    }
    i32 by = (worldY & ~0x1f) + 0x10;
    i32 bx = (worldX & ~0x1f) + 0x10;
    cell->m_coordRetryCount = 0;
    i32 r = cell->StepArrivalDrop(bx, by, arrivalPhase, -1, 1, 0);
    return r != 0 ? 1 : 0;
}

// @early-stop
RVA(0x0006ea00, 0x125)
void CTriggerMgr::HitTestApply(i32 x, i32 y, HitSpanArg span) {

    CGrunt* cell = FindGruntAt(x, y, span.m_span, &span.m_outCol, &y, 0);
    if (cell == 0 || span.m_outCol != g_curPlayer) {
        return;
    }
    const char* name = *g_typeColl.ScratchResolve(cell->m_objAux->m_actKey);
    bool differ = strcmp(name, "B") != 0;
    if (!differ) {
        return;
    }
    PickupType k = cell->m_entranceReason;
    if (k > PICKUP_TOOLZ_LAST) {
        k = cell->m_toolId;
    }
    if (k != PICKUP_WARPSTONE) {
        return;
    }
    CPlay* world = static_cast<CPlay*>(g_gameReg->m_curState);

    CTimer* sub = world->m_frameMarker;
    i64 diff = static_cast<i64>(static_cast<u32>(g_frameTime)) - sub->m_startStamp.m_v;
    if (diff < 0) {
        diff = 0;
    }
    g_gameReg->m_scoreHud->m_elapsedTimeMs += static_cast<i32>(diff);
    sub->m_unusedStamp.m_v = 0;
    sub->m_accum.m_v = 0;
    sub->m_running = 0;
    sub->m_currentMs = 0;
    world->ArmSnapshot(0, 0xbb7);
    world->m_guts->SetMode(1);
    this->ClearRow(g_curPlayer);
}
