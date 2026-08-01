// TriggerMgrGrid.cpp - the CTriggerMgr grid/placement/trigger-apply obj:
// retail .text [0x6b640..0x6eb25], the FIRST of the three objs our old
// `triggermgr` unit conflated (SPLIT verdict, docs/exe-map/TU_MIGRATION.md +
// the 0x077f80 dossier). Original TU: filename unknown (@identity-TODO).
//
// Oracle evidence for the carve (own original obj, not an adjacent TU's tail):
//   * its OWN CRT init-fragment run - 10 frags @0x6b370 directly precede this
//     code block (the 0x77f80 obj has a separate 7-frag run @0x7d8f0; two
//     table runs = two objs);
//   * the block is contiguous [0x6b640..0x6eb25] and bounded by foreign objs
//     (grunt-region COMDAT pocket + frags before, goowellmgr @0x6eb80 after);
//   * in the init TABLE (link order) this obj sits right after the grunt
//     region - the retail source neighborhood of Grunt.cpp.
//
// (The former SEAM is CLOSED: ?WireTileSwitchLogic@CTriggerMgr@@ @0x6c130 lives
// below, between ResetCell (0x6bfd0) and ApplySwitch (0x6d300), as first-link
//
// Functions in retail-RVA order; shared views/externs in
// <Gruntz/TriggerMgrViews.h>. /GX unit (ApplySwitch owns a CString temp).
#include <Gruntz/ActReg.h> // CActReg + g_typeColl (the 0x6bf650 registry; ex the CTmNameReg view)
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Play.h>  // canonical CPlay (m_curState real class: ArmSnapshot et al.)
#include <Gruntz/Timer.h> // CTimer - CPlay::m_frameMarker (the ex-CTmScoreSub @+0x3f4)

#include <Gruntz/ActionOptionsMenuBar.h>
#include <Gruntz/GruntzCmdMgr.h>
#include <Gruntz/StatusBarMgr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/GruntzMapMgr.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/GruntSpawnConfig.h>
#include <Gruntz/BattlezData.h>          // CBattlezData - the REAL +0x7c HUD/score board
#include <DDrawMgr/DDrawChildGroup.h>    // the ONE CDDrawChildGroup (CreateSprite @0x1597b0)
#include <Gruntz/UserLogic.h>            // canonical CUserLogic (switch/trigger logic virtuals)
#include <Gruntz/TileTriggerContainer.h> // CTileTriggerContainer (CPlay::m_beginMarker; FindChild)
#include <Gruntz/TileTriggerSwitchLogic.h> // the 0x8c switch element (SwitchDown/m_key1)
#include <Gruntz/TileTriggerLogic.h>       // the 0x9c logic child (FindIndexByKey/RecordMove)
#include <Gruntz/TileGrid.h>               // canonical CMapMgr (the registry's +0x70 tile grid)
#include <Bute/ButeMgr.h>                  // canonical CButeMgr (one shape)
#include <Wwd/WwdFile.h>                   // CDDrawWorkerHost - the canonical plane (dims here)
#include <Gruntz/Grunt.h>                  // real CGrunt (the grid cells)
#include <Gruntz/GruntPuddle.h>            // the m_baseList element (ApplyTriggerA case 7)

#include <Gruntz/TriggerMgrViews.h> // the shared CTm* views + singleton externs

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

// 0x6b6d0: PlaceObject - the tile-object placer/factory. Validates the (col,row) against the
// plane bounds + the tile attribute mask (0x4000911), finds the first free grid column, then
// by the supplied kind (a dense jump table mapping kind->internal id, with Wormhole/Entrance
// special-cases that CreateSprite from the level factory + read the EntranceColor config),
// stashes the new cell into the grid (+0x1c) and bumps the per-row/level counters. ret the
// placed column (or -1). (__stdcall: ret 0x34.) Reconstructed to plateau.
// @early-stop
RVA(0x0006b6d0, 0x3f4)
i32 CTriggerMgr::PlaceObject(
    i32 a8,
    i32 ax,
    i32 ay,
    i32 col,
    i32 row,
    i32 kind,
    i32 a18,
    i32 a1c,
    i32 a20,
    i32 a24,
    i32 a28,
    i32 a2c,
    i32 a30
) {
    static_cast<void>(a8);
    static_cast<void>(a18);
    static_cast<void>(a24);
    static_cast<void>(a28);
    static_cast<void>(a2c);
    if (m_world == 0) {
        return -1;
    }
    i32 special = 0;
    i32 wantSlot = 0;
    if (a30 == 0x12) {
        special = 0x100;
        wantSlot = 1;
    }
    CGruntzMapMgr* plane = g_gameReg->m_tileGrid;
    i32 attr;
    if ((ax >> 5) >= plane->m_width || (ay >> 5) >= plane->m_height) {
        attr = 1;
    } else {
        attr = plane->m_rowInts[ay >> 5][(ax >> 5) * 7];
    }
    if ((attr & 0x4000911) != 0 && (special & attr) == 0) {
        return -1;
    }
    if ((attr & 0x82) != 0 || (attr & 0x400) != 0) {
        return -1;
    }
    if (wantSlot == 0 || (attr & 0x100) == 0) {
        return -1;
    }
    if (a20 != 0) {
        return -1;
    }
    // find the first free grid column of row `row`
    CGrunt** rowBase = &m_grid[row * TM_GRID_COLS];
    i32 free = 0;
    if (*rowBase != 0) {
        CGrunt** p = rowBase;
        while (free < 15 && *p != 0) {
            p++;
            free++;
        }
    }
    if (free >= 15) {
        return -1;
    }
    CDDrawChildGroup* fac = m_world->m_childGroup;
    CWwdGameObjectA* sprite = fac->CreateSprite(0, ax, ay, ay, "Grunt", 0x40003);
    if (sprite == 0) {
        return -1;
    }
    sprite->m_7c->m_notify(sprite);
    // Same shape as CTriggerMgr::SpawnGrunt (0x7c110), and the same correction: the grid
    // holds the sprite's LOGIC leaf, not the CreateSprite result (retail reassigns the
    // register to aux->m_logic before the `mov [grid],reg`). It stored the sprite here too.
    CGrunt* logic = static_cast<CGrunt*>(sprite->m_7c->m_logic);
    // (the dense kind jump table -> internal id + the Wormhole / Entrance sub-ctors elide
    // here; reconstructed to plateau)
    m_grid[row * TM_GRID_COLS + free] = logic;
    m_rowCount[row] += 1;
    m_cellFlag[(row * TM_GRID_COLS + free)] = 0;
    g_gameReg->m_scoreHud->m_counts[row] += 1;
    return free;
}

RVA(0x0006bc20, 0x6f)
i32 CTriggerMgr::DispatchCellForObject(CGrunt* obj, i32 startRow, i32 kind, i32 arg) {
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
i32 CTriggerMgr::CellDispatch(i32 row, i32 col, i32 kind, i32 arg) {
    CGrunt* cell = m_grid[row * TM_GRID_COLS + col];
    if (cell == 0) {
        return 0;
    }
    if (cell->m_deathAnimStarted != 0) {
        NotifyCell(row, col, 0);
        return 0;
    }
    // The grid cell is a real CGrunt (m_grid holds "Grunt" sprites); route it via the
    // real CGrunt methods so the calls bind (ExitGrid==BuildGruntExitAnimation @0x641b0,
    // Route==LoadGruntDeathAnimations @0x60150). The m_grid CGrunt*->CGrunt* retype is
    // deferred cross-lane (FindGruntAt's return type ripples into Play.cpp et al.).
    if (kind == 0xd) {
        (static_cast<CGrunt*>(cell))->BuildGruntExitAnimation();
    } else {
        (static_cast<CGrunt*>(cell))->LoadGruntDeathAnimations(kind, arg);
    }
    return 1;
}

// 0x6bd40: ClearGridRange(startRow) - ResetAll, then for rows startRow..3 (5 = all)
// flag each live cell's goal (+0x154) done and clear the cell, its parallel grid slot
// (+0x11c) and the per-row state words (+0x10c/+0x20c/+0x21c); then ClearSelections.
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
        // The three per-row bands the loop clears: m_rowCount (+0x10c), m_rowStateB
        // (+0x20c) and m_rowStateC (+0x21c). The old spelling reached the first two
        // through one cursor (`(char*)perRow - 0x100` and `perRow[4]`); they are
        // named arrays, so index them.
        i32 r = row;
        i32 g2 = row * TM_GRID_COLS;
        do {
            i32 col = 0;
            do {
                CGrunt* c = *cell;
                if (c != 0) {
                    c->m_38->m_flags |= 0x10000;
                    *cell = 0;
                    m_cellFlag[g2 + col] = 0;
                }
                col++;
                cell++;
            } while (col < 15);
            m_rowCount[r] = 0;
            m_rowStateB[r] = 0;
            m_rowStateC[r] = 0;
            r++;
            g2 += 15;
            n--;
        } while (n != 0);
    }
    ClearSelections();
    return 1;
}

// 0x6be30: ScreenToCell - bias the input (sx,sy) by the level view's scroll origin
// (view@m_24: scroll struct embedded at [m_5c]+0x40, origin @m_10/m_14) and forward to
// CellHitTest.
// @early-stop
RVA(0x0006be30, 0x47)
CGrunt* CTriggerMgr::ScreenToCell(i32 sx, i32 sy, i32* outRow, i32* outCol, i32 startRow) {
    CGameLevel* view = m_world->m_level;
    i32 px = view->m_mainPlane->m_viewRect.left - view->m_planeCtx.left + sx;
    i32 py = view->m_mainPlane->m_viewRect.top - view->m_planeCtx.top + sy;
    return CellHitTest(px, py, outRow, outCol, startRow);
}

// 0x6bea0: CellHitTest - scan the grid for the cell whose 30x30 object bounds contain
// (px,py). startRow==5 means "rows 0..3"; otherwise just that one row. Writes the hit
// (row,col) through the out-ptrs and returns the cell pointer (0 when none).
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
    // ONE miss exit (retail 0x6bf76): the row gate and the row loop's bottom test
    // both branch into it, so the outer back-edge is an unconditional jmp
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

// 0x6bfd0: ResetCell(col, row, force, ...) - if grid[row*15+col] (+0x1c) is live: for a
// non-magic row, run its three sub-state resetters then re-seed its CombatTimeout config
// fields (+0x880..+0x88c); for the magic row (== g_curPlayer), when not forced recycle the
// (row,col) record node onto the free list, AddTail it to +0x240, and run ResetMagic. ret 1
// only when a magic cell was recycled, else 0. (__stdcall: ret 0x10.)
RVA(0x0006bfd0, 0x106)
i32 CTriggerMgr::ResetCell(i32 col, i32 row, i32 force, i32 keep) {
    i32 idx = col * TM_GRID_COLS + row;
    CGrunt* cell = m_grid[idx];
    if (cell == 0 || cell->m_entranceCommitted == 0) {
        return 0;
    }
    if (col != g_curPlayer) {
        // RECOVERED (the view's RVAs for these were bogus - see <Gruntz/Grunt.h>). Retail
        // calls three ILT thunks here (0x243c / 0x22de / 0x4246), which jump to 0x4d130 /
        // 0x4d2f0 / 0x4d220: CGrunt's own CreateHealthSprite / CreateStaminaSprite /
        // CreateToySprite, all already-claimed bodies. They were ResetA/B/C - phantoms.
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
        // RECOVERED 2026-08-01: retail 0x6c063 `call 0x36ed` is the ILT thunk to
        // 0x78430 = ?ResetAll@CTriggerMgr@@QAEXXZ, NOT ResetSpawnState (0x79d90).
        // assert_relocs flagged the old binding the moment this fn reached 100%.
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
    // RECOVERED: the 0-arg i32 call in this function's set is the ILT thunk 0x24c8 ->
    // 0x4b130 = ?CommitArrival@CGrunt@@QAEHXZ. `ResetMagic` was a phantom name for it.
    return cell->CommitArrival();
}

// @early-stop
RVA(0x0006c130, 0xd62)
i32 CTriggerMgr::WireTileSwitchLogic(CGrunt* g, i32 x, i32 y) {
    // retail loads the play state early and spills it ([esp+0x10]); the switch
    // container walks below read it back per arm.
    CPlay* state = static_cast<CPlay*>(g_gameReg->m_curState);

    if (g != 0) {
        g->m_358 = 1;
    }

    // Inlined LookupTileType(m_level->m_level, x, y): clamp (x,y) to the main plane,
    // resolve the tile cell, ask its image set the collision kind at (subX, subY).
    CGameLevel* level = m_world->m_level;
    CDDrawWorkerHost* plane = level->m_mainPlane;
    i32 cx = x;
    i32 cy = y;
    if (cx < 0) {
        cx = 0;
    } else if (cx >= plane->m_wrapW) {
        cx = plane->m_wrapW - 1;
    }
    if (cy < 0) {
        cy = 0;
    } else if (cy >= plane->m_wrapH) {
        cy = plane->m_wrapH - 1;
    }
    i32 tx = cx >> plane->m_shiftX;
    i32 ty = cy >> plane->m_shiftY;
    i32 subX = cx - (tx << plane->m_shiftX);
    i32 subY = cy - (ty << plane->m_shiftY);
    i32 raw = plane->m_tileGrid[plane->m_colOffsets[ty] + tx];
    i32 tag = 0;
    if (raw != static_cast<i32>(0xeeeeeeee) && raw != -1) {
        CTileImageSet* ts = static_cast<CTileImageSet*>(level->m_imageSets.GetAt(raw & 0xffff));
        tag = ts->GetCollisionAt(subX, subY);
    }

    if (static_cast<u32>((tag - 0xb)) > 0x65) {
        return 0;
    }

    // The 20-way per-kind switch (byte dispatch table on tag-0xb). First arm
    // reconstructed: resolve the (tile,kind-7) switch element in the play state's
    // trigger container, fire it, then run every 0x9c logic child claiming its key.
    CTileTriggerContainer* trig = state->m_beginMarker;
    CTileTriggerSwitchLogic* sw = trig->FindChild((y >> 5) + ((x >> 5) << 8), TRIGID_TIME_SWITCH_7);
    if (sw == 0) {
        CString msg; // [esp+0x30] diagnostic temp
        msg.Format("No switch logic found for switch at: x=%d, y=%d", x, y);
        g_gameReg->EnterModalUI(static_cast<const char*>(msg));
        g_gameReg->ReportError(TRIGERR_LOOKUP_MISS, 0x3eb);
        return 0;
    }
    sw->SwitchDown(); // virtual slot 2 on the found switch element

    // Run every m_list2 then m_list1 logic child that claims the switch's key.
    i32 anyHit = 0;
    POSITION pos;
    trig = state->m_beginMarker;
    pos = trig->m_list2.GetHeadPosition();
    while (pos != 0) {
        CTileTriggerLogic* el = static_cast<CTileTriggerLogic*>(trig->m_list2.GetNext(pos));
        if (el->FindIndexByKey(sw->m_key1) != 0) {
            anyHit = 1; // retail branches into the shared success tail (0x6cc7e)
            break;
        }
    }
    trig = state->m_beginMarker;
    pos = trig->m_list1.GetHeadPosition();
    while (pos != 0) {
        CTileTriggerLogic* el = static_cast<CTileTriggerLogic*>(trig->m_list1.GetNext(pos));
        if (el->FindIndexByKey(sw->m_key1) != 0) {
            el->RecordMove();
            anyHit = 1;
        }
    }
    if (anyHit == 0) {
        CString msg;
        msg.Format("No trigger logic found for switch at: x=%d, y=%d", x, y);
        g_gameReg->EnterModalUI(static_cast<const char*>(msg));
        g_gameReg->ReportError(TRIGERR_LINK_BROKEN, 0x3ec);
        return 0;
    }
    return 1;
}

// 0x6d300: ApplySwitch(g, sx, sy) - the /GX switch-logic driver. Clamp (sx,sy) to the
// plane, sample the tile attribute, decode the collision kind, then switch over that kind
// (retail jump table @0x46d8b4, value table @0x46d8cc indexed by kind-0x34 over 0..0xe;
// live arms 0x34/0x36/0x38/0x40/0x42, everything else falls through to `return 0`).
// Each arm resolves the switch object via m_beginMarker->FindChild(((sx>>5)<<8)+(sy>>5), K)
// with its OWN K (0x40->7, 0x34->0, 0x36->0, 0x38->3, 0x42->8) and, on a miss, Formats
// "No switch logic found for switch at: x=%d, y=%d" (or "No trigger logic ...") with the
// RAW sx/sy into a stack CString, EnterModalUI's it and ReportErrors a per-arm site id
// 0x3f7..0x3fd. The CString scopes are the seven /GX EH states 0..6, in that source order.
// ARITY FIXED (2026-07-14): retail ends `ret 0xc` = THREE dwords - every caller
// (GruntSteps/Grunt.cpp/GruntCombat) pushes (grunt, x, y); the old 2-arg spelling
// emitted `ret 8`. The grunt arg IS read - by the 0x42 arm only (it gates on
// g->m_tileOwnerHi == g_curPlayer).
// The arms read the RAW args from their incoming stack slots, NOT the clamped x/y: ebx/edi
// hold subX/subY by the time the jump table fires, so every arm re-loads [esp+0x30]/[esp+0x34].
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
    i32 kind;
    if (attr == static_cast<i32>(0xeeeeeeee) || attr == -1) {
        kind = 0;
    } else {
        CTileImageSet* ts = static_cast<CTileImageSet*>(view->m_imageSets.GetAt(attr & 0xffff));
        kind = ts->GetCollisionAt(subX, subY); // slot 8 (+0x20)
    }
    switch (kind) {
        case 0x40: {
            CTileTriggerSwitchLogic* obj =
                state->m_beginMarker->FindChild(((sx >> 5) * 0x100) + (sy >> 5), 7);
            if (obj == 0) {
                CString msg;
                msg.Format("No switch logic found for switch at: x=%d, y=%d", sx, sy);
                g_gameReg->EnterModalUI(msg);
                g_gameReg->ReportError(TRIGERR_LOOKUP_MISS, TRIGSITE_APPLY_SWITCH_40);
                return 0;
            }
            obj->SwitchUp(); // vtbl slot 3 (+0xc)
            return 1;
        }
        case 0x34: {
            CTileTriggerSwitchLogic* obj =
                state->m_beginMarker->FindChild(((sx >> 5) * 0x100) + (sy >> 5), 0);
            if (obj == 0) {
                CString msg;
                msg.Format("No switch logic found for switch at: x=%d, y=%d", sx, sy);
                g_gameReg->EnterModalUI(msg);
                g_gameReg->ReportError(TRIGERR_LOOKUP_MISS, TRIGSITE_APPLY_SWITCH_34);
                return 0;
            }
            obj->SwitchUp();
            return 1;
        }
        case 0x36: {
            CTileTriggerSwitchLogic* obj =
                state->m_beginMarker->FindChild(((sx >> 5) * 0x100) + (sy >> 5), 0);
            if (obj == 0) {
                CString msg;
                msg.Format("No switch logic found for switch at: x=%d, y=%d", sx, sy);
                g_gameReg->EnterModalUI(msg);
                g_gameReg->ReportError(TRIGERR_LOOKUP_MISS, TRIGSITE_APPLY_SWITCH_36);
                return 0;
            }
            obj->SwitchUp();
            // every m_list1 child that claims this switch's key gets its slot-0 Tick;
            // a child returning 0 stops the walk.
            POSITION pos = state->m_beginMarker->m_list1.GetHeadPosition();
            i32 found = 0;
            i32 stop = 0;
            while (pos != 0) {
                if (stop != 0) {
                    break;
                }
                CTileTriggerLogic* child =
                    static_cast<CTileTriggerLogic*>(state->m_beginMarker->m_list1.GetNext(pos));
                if (child->FindIndexByKey(obj->m_key1) != 0) {
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
                g_gameReg->ReportError(TRIGERR_LINK_BROKEN, TRIGSITE_APPLY_TRIGGER_36);
                return 0;
            }
            return 1;
        }
        case 0x38: {
            CTileTriggerSwitchLogic* obj =
                state->m_beginMarker->FindChild(((sx >> 5) * 0x100) + (sy >> 5), 3);
            if (obj == 0) {
                CString msg;
                msg.Format("No switch logic found for switch at: x=%d, y=%d", sx, sy);
                g_gameReg->EnterModalUI(msg);
                g_gameReg->ReportError(TRIGERR_LOOKUP_MISS, TRIGSITE_APPLY_SWITCH_38);
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
                    if (child->FindIndexByKey(obj->m_key1) != 0) {
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
                    g_gameReg->ReportError(TRIGERR_LINK_BROKEN, TRIGSITE_APPLY_TRIGGER_38);
                    return 0;
                }
            }
            obj->SwitchUp();
            return 1;
        }
        case 0x42: {
            // the battlez-only arm: only the local player may flip it, and it always
            // reports "not handled" (falls into the shared xor eax,eax exit).
            if (g_gameReg->m_134 != 1) {
                return 0;
            }
            if (g == 0) {
                return 0;
            }
            if (g->m_tileOwnerHi != g_curPlayer) {
                return 0;
            }
            CTileTriggerSwitchLogic* obj =
                state->m_beginMarker->FindChild(((sx >> 5) * 0x100) + (sy >> 5), 8);
            if (obj == 0) {
                CString msg;
                msg.Format("No switch logic found for switch at: x=%d, y=%d", sx, sy);
                g_gameReg->EnterModalUI(msg);
                g_gameReg->ReportError(TRIGERR_LOOKUP_MISS, TRIGSITE_APPLY_SWITCH_42);
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

// retail 0x6da60/0x6daa0 end at the void EnqueueSingle call with NO eax write, so void
// (a `return 0` adds a spurious xor eax,eax). The bodies never read `this`; the
// __thiscall linkage is proven by the receiver load at both call sites (see the
// declaration note in TriggerMgr.h).
RVA(0x0006da60, 0x27)
void CTriggerMgr::GridAction6(i32 a, i32 b) {
    g_gameReg->m_cmdSubMgr->EnqueueSingle(1, a, b, 6, 0, 0, 0, 0);
}

RVA(0x0006daa0, 0x27)
void CTriggerMgr::GridAction7(i32 a, i32 b) {
    g_gameReg->m_cmdSubMgr->EnqueueSingle(1, a, b, 7, 0, 0, 0, 0);
}

// 0x6dae0: ApplyTriggerA(col, row, worldX, worldY) - look up grid[col*15+row]; if live,
// un-pending and matching the snapped source pos, dispatch the cell's trigger logic by its
// kind (the 0x13/0xf branch families); update its state and return the applier result. Else
// -1 / 0. (__thiscall, 4 int args: ?ApplyTriggerA@CTriggerMgr@@QAEHHHHH@Z, ret 0x10 - the
// old "__stdcall: ret 0x1c / 6 args" note was wrong on both counts.)
// worldX/worldY named 2026-07-29 from retail: with the frame at entry-0x24, @0x6db0e reads
// [esp+0x30] and [esp+0x34] (args 3 and 4) and immediately `sar`s each by 5 - the /32 tile
// snap - exactly as the twin ApplyTriggerB @0x6e120 does with its already-named
// worldX/worldY. arg1*15+arg2 is the m_grid index, so arg1/arg2 are col/row.
// Switch tables read out of the image: value table @0x46dfc4 (0x15 bytes, indexed by
// kind-2 over 0..0x14), jump table @0x46df98 (11 slots). Retail spills all four tile
// scalars - cellTileX@[esp+0x10], cellTileY@[esp+0x30], argTileX@[esp+0x28],
// argTileY@[esp+0x2c] - keeps raw worldX in ebp, and carries two more stack homes for the
// raw m_17c/m_180 loads (frame 0x14). Search: config/axes/applytriggera-head.json.
// @early-stop
RVA(0x0006dae0, 0x4b7)
i32 CTriggerMgr::ApplyTriggerA(i32 col, i32 row, i32 worldX, i32 worldY) {
    CGrunt* cell = m_grid[col * TM_GRID_COLS + row];
    if (cell == 0 || cell->m_entranceCommitted == 0) {
        return 0;
    }
    i32 cellTileX = cell->m_lastTilePxX >> 5;
    i32 cellTileY = cell->m_lastTilePxY >> 5;
    i32 argTileX = worldX >> 5;
    i32 argTileY = worldY >> 5;
    CGameObject* o = cell->m_object;
    if (o->m_screenX != cell->m_lastTilePxX) {
        return -1;
    }
    if (o->m_screenY != cell->m_lastTilePxY) {
        return -1;
    }
    i32 k = cell->m_entranceReason;
    if (k > 0x16) {
        k = cell->m_toolId;
    }
    if (k == 0x13 && cell->CanShowStamina() != 0) {
        if (cellTileX != argTileX || cellTileY != argTileY) {
            return 0;
        }
        cell->RunMoveConfig(cellTileX, cellTileY + 1);
        return 1;
    }
    if (cellTileX == argTileX && cellTileY == argTileY) {
        i32 kSame = cell->m_entranceReason;
        if (kSame > 0x16) {
            kSame = cell->m_toolId;
        }
        if (kSame != 0xf) {
            return 0;
        }
        if (cell->CanShowStamina() == 0) {
            return 0;
        }
        cell->RunMoveConfig(argTileX, argTileY);
        return 1;
    }
    i32 kDiag = cell->m_entranceReason;
    if (kDiag > 0x16) {
        kDiag = cell->m_toolId;
    }
    if (kDiag == 1) {
        // reason 1 = the diagonal walk: an off-axis step is only legal on the 45 deg line
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
    i32 bute = map->m_rows[by >> 5][bx >> 5].m_10; // target tile's bute type code
    i32 kind = cell->m_entranceReason;
    if (kind > 0x16) {
        kind = cell->m_toolId;
    }
    // Case ORDER is retail's source order, proven by the physical arm layout in the
    // image (5 @0x6dd8e, 13 @0x6ddd9, 7 @0x6ddea, 15 @0x6de5a, 3 @0x6de7a + the shared
    // RunMoveConfig tail @0x6de96 that 13 jumps into, the merged BeginAttack body
    // @0x6deb6, 20 @0x6decf). The jump-table SLOT numbering is by case value, so the
    // four separate BeginAttack groups {2} {9,10,11} {17} {21,22} must stay separate
    // groups - one merged group would collapse them to a single slot.
    switch (kind) {
        case 5:
            if (bute == 0x1e || bute == 0x1f || bute == 0x21 || bute == 0x97 || bute == 0x98
                || bute == 0x99) {
                cell->RunMoveConfig(argTileX, argTileY);
                return 1;
            }
            return 0;
        case 13:
            if (bute == 0x22 || bute == 0x23) {
                cell->RunMoveConfig(argTileX, argTileY);
                return 1;
            }
            return 0;
        case 7: {
            POSITION pos = m_baseList.GetHeadPosition();
            while (pos != 0) {
                CGruntPuddle* cand = static_cast<CGruntPuddle*>(m_baseList.GetNext(pos));
                if (cand->m_pending == 0 && cand->m_tileX == argTileX
                    && cand->m_tileY == argTileY) {
                    cell->RunMoveConfig(argTileX, argTileY);
                    cand->m_value = cand->m_38->m_1a0.m_14;
                    cand->m_38->ApplyLookupGeometry("GRUNTZ_GRUNTPUDDLE_GRUNTPUDDLE3", 0);
                    cand->m_pending = 1;
                    return 1;
                }
            }
            return 0;
        }
        case 15:
            cell->RunMoveConfig(cellTileX, cellTileY);
            return 1;
        case 3:
            if (bute == 0x96 || bute == 0x97 || bute == 0x98) {
                cell->RunMoveConfig(argTileX, argTileY);
                return 1;
            }
            return 0;
        case 2:
            return cell->BeginAttack(bx, by) != 0;
        case 9:
        case 10:
        case 11:
            return cell->BeginAttack(bx, by) != 0;
        case 17:
            return cell->BeginAttack(bx, by) != 0;
        case 21:
        case 22:
            return cell->BeginAttack(bx, by) != 0;
        case 20: {
            if (g_gameReg->m_134 == 1) {
                return 0;
            }
            i32 flags = 1;
            if (static_cast<u32>(argTileX) < map->m_width
                && static_cast<u32>(argTileY) < map->m_height) {
                flags = map->m_rows[argTileY][argTileX].m_0;
            }
            if ((flags & 0x40939) != 0 || (flags & 2) != 0) {
                return 0;
            }
            LoadPowerupIconSprites(0x14, bx, by, 0, cell->m_38c, 0);
            cell->PlayMoveSound(bx, by);
            if (cell->m_poweredUp != 0 && cell->m_neighborValid == 0) {
                cell->m_entranceActive = 0;
                cell->m_combatActive = 0;
                cell->m_neighborValid = 0;
                cell->m_poweredUp = 0;
                cell->ResetEntranceAnimation(1, 0, 0);
            }
            cell->LoadGruntTypeTable(0, 1, 0, 0);
            return 1;
        }
    }
    return 0;
}

// 0x6e120: ApplyTriggerB(a1c, col, row, a28, a2c, a30) - the exit variant of ApplyTriggerA:
// same cell lookup + validation, then snap (a28,a2c) to a tile and route the cell's exit
// logic; updates the arrival phase and returns the applier's boolean. (__stdcall: ret 0x10.)
// Reconstructed to plateau.
// @early-stop
RVA(0x0006e120, 0x552)
i32 CTriggerMgr::ApplyTriggerB(i32 col, i32 row, i32 worldX, i32 worldY) {
    CGrunt* cell = m_grid[col * TM_GRID_COLS + row];
    if (cell == 0 || cell->m_entranceCommitted == 0 || cell->m_entranceActive != 0) {
        return 0;
    }
    i32 cellTileX = cell->m_lastTilePxX >> 5;
    i32 cellTileY = cell->m_lastTilePxY >> 5;
    i32 argTileX = worldX >> 5;
    i32 argTileY = worldY >> 5;
    CGameObject* o = cell->m_object;
    if (o->m_screenX != cell->m_lastTilePxX) {
        return -1;
    }
    if (o->m_screenY != cell->m_lastTilePxY) {
        return -1;
    }
    // retail compares the SNAPPED TILE coords here, not the pixel pair
    if (cellTileX == argTileX && cellTileY == argTileY && cell->m_198 != 0x1e
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
            flags = map->m_rows[argTileY][argTileX].m_0;
        }
        if ((flags & 0x40939) != 0 || (flags & 0x82) != 0) {
            return 0;
        }

        i32 kind = cell->m_198;
        i32 moveKind = kind == 0x1e ? cell->m_moveKind : 0;
        if (LoadToyBoxIcon(bx, by, col, kind, moveKind) == 0) {
            return 0;
        }

        char* name = *g_typeColl.GetNameRecord(cell->m_objAux->m_1c);
        if (strcmp(name, "I") == 0) {
            LoadTileArrivalFx(
                col,
                row,
                cell->m_moveTileX,
                cell->m_moveTileY,
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
        cell->LoadVehicleGruntSprites(0);
        return 1;
    }

    if ((hit->m_lastTilePxX != bx || hit->m_lastTilePxY != by)
        && (hit->m_commitPxX != bx || hit->m_commitPxY != by)) {
        return 0;
    }

    char* hitName = *g_typeColl.GetNameRecord(hit->m_objAux->m_1c);
    if (strcmp(hitName, "G") == 0 || strcmp(hitName, "L") == 0 || strcmp(hitName, "P") == 0) {
        return 0;
    }

    i32 kind = cell->m_198;
    i32 moveKind = kind == 0x1e ? cell->m_moveKind : 0;
    cell->PlayMoveSound(bx, by);
    cell->m_neighborValid = 0;
    if (cell->m_poweredUp != 0) {
        cell->m_entranceActive = 0;
        cell->m_combatActive = 0;
        cell->m_neighborValid = 0;
        cell->m_poweredUp = 0;
        cell->ResetEntranceAnimation(1, 0, 0);
    }

    char* name = *g_typeColl.GetNameRecord(cell->m_objAux->m_1c);
    if (strcmp(name, "I") == 0) {
        LoadTileArrivalFx(
            col,
            row,
            cell->m_moveTileX,
            cell->m_moveTileY,
            cell->m_entranceReason,
            -1
        );
    }
    if (hit->LoadGruntTypeTable(kind, 1, moveKind, 0) == 0) {
        return 0;
    }
    cell->LoadVehicleGruntSprites(0);

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

// 0x6e800: ClearCell(col, row, arrivalPhase, worldX, worldY) - if grid[col*15+row] is live, reset its
// trigger/anim sub-state (unless already cleared via +0x420), bail if it has a pending
// flag (+0x1e4), look up its config name; when it equals "I" run the manager's fx with the
// cell's pose; then StepArrivalDrop on the snapped (worldX, worldY) bounds and return its
// boolean result.
// (__stdcall: ret 0x14.)
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
    const char* name = *g_typeColl.ScratchResolve(cell->m_objAux->m_1c);
    if (strcmp(name, "I") == 0) {
        i32 px = cell->m_moveTileX;
        i32 py = cell->m_moveTileY;
        this->LoadTileArrivalFx(px, py, py, cell->m_entranceReason, -1, py);
    }
    i32 by = (worldY & ~0x1f) + 0x10;
    i32 bx = (worldX & ~0x1f) + 0x10;
    cell->m_coordRetryCount = 0;
    i32 r = cell->StepArrivalDrop(bx, by, arrivalPhase, -1, 1, 0);
    return r != 0 ? 1 : 0;
}

// 0x6ea00: HitTestApply(x, y, kind) - hit-test the cell at (x,y); only for the magic group
// (out-col == g_curPlayer) and a cell whose config name is NOT "B" and kind 0x14, add the world's
// score delta, zero the status fields, SetStat(0,0xbb7), re-arm the status item (SetMode 1)
// and ClearRow(g_curPlayer). void - no path materialises a return value. (__stdcall: ret 0xc.)
// @early-stop
RVA(0x0006ea00, 0x125)
void CTriggerMgr::HitTestApply(i32 x, i32 y, HitSpanArg span) {
    // retail 0x6ea00 reads arg3 BOTH by value (the span rect) and by address (the
    // out-column): `mov edx,[esp+0xc]` and `lea ecx,[esp+0x20]` resolve to the same
    // E+0xc slot. HitSpanArg names both readings (see <Gruntz/TriggerMgr.h>).
    CGrunt* cell = FindGruntAt(x, y, span.m_span, &span.m_outCol, &y, 0);
    if (cell == 0 || span.m_outCol != g_curPlayer) {
        return;
    }
    const char* name = *g_typeColl.ScratchResolve(cell->m_objAux->m_1c);
    bool differ = strcmp(name, "B") != 0;
    if (!differ) {
        return;
    }
    i32 k = cell->m_entranceReason;
    if (k > 0x16) {
        k = cell->m_toolId;
    }
    if (k != 0x14) {
        return;
    }
    CPlay* world = static_cast<CPlay*>(g_gameReg->m_curState);
    // world->m_3f4 IS CPlay::m_frameMarker (the CTimer): read its i64 start stamp
    // (m_38:m_3c) as the elapsed accumulator, credit the HUD score, then zero the
    // timer's accum/lap/running/current block.
    CTimer* sub = world->m_frameMarker;
    i64 diff = static_cast<i64>(static_cast<u32>(g_frameTime)) - sub->m_startStamp.m_v;
    if (diff < 0) {
        diff = 0;
    }
    g_gameReg->m_scoreHud->m_score += static_cast<i32>(diff);
    sub->m_40 = 0;
    sub->m_44 = 0;
    sub->m_accumLo = 0;           // +0x30
    sub->m_accumHi = 0;           // +0x34
    sub->m_running = 0;           // +0x48
    sub->m_currentMs = 0;         // +0x4c
    world->ArmSnapshot(0, 0xbb7); // 0xd9240
    world->m_guts->SetMode(1);
    this->ClearRow(g_curPlayer);
}
