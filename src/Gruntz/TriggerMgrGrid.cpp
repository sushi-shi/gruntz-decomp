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
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Play.h>  // canonical CPlay (m_curState real class: ArmSnapshot et al.)
#include <Gruntz/Timer.h> // CTimer - CPlay::m_frameMarker (the ex-CTmScoreSub @+0x3f4)

#include <Gruntz/ActionOptionsMenuBar.h>
#include <Gruntz/GruntzCmdMgr.h>
#include <Gruntz/StatusBarMgr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/BattlezData.h>          // CBattlezData - the REAL +0x7c HUD/score board
#include <DDrawMgr/DDrawChildGroup.h>    // the ONE CDDrawChildGroup (CreateSprite @0x1597b0)
#include <Gruntz/UserLogic.h>            // canonical CUserLogic (switch/trigger logic virtuals)
#include <Gruntz/TileTriggerContainer.h> // CTileTriggerContainer (CPlay::m_beginMarker; FindChild)
#include <Gruntz/TileTriggerSwitchLogic.h> // the 0x8c switch element (SwitchDown/m_key1)
#include <Gruntz/TileTriggerLogic.h>       // the 0x9c logic child (FindIndexByKey/RecordMove)
#include <Gruntz/TileGrid.h>               // canonical CTileGrid (the registry's +0x70 tile grid)
#include <Bute/ButeMgr.h>                  // canonical CButeMgr (one shape)
#include <Wwd/WwdFile.h>                   // CPlaneRender - the canonical plane (dims here)
#include <Gruntz/Grunt.h>                  // real CGrunt (the grid cells)
#include <Globals.h>

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
// big factory/jump-table driver (0x3f4 B): the kind jump table + the two CreateSprite stanzas
// pin ebp(this)/esi/edi differently than retail and the attribute-mask test ladder spills.
// Structurally faithful; regalloc diverges across the table. topic:wall.
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
    CTmCell** rowBase = &m_grid[row * TM_GRID_COLS];
    i32 free = 0;
    if (*rowBase != 0) {
        CTmCell** p = rowBase;
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
i32 CTriggerMgr::DispatchCellForObject(CTmCell* obj, i32 startRow, i32 kind, i32 arg) {
    i32 last;
    if (startRow == 5) {
        startRow = 0;
        last = 3;
    } else {
        last = startRow;
    }
    for (i32 row = startRow; row <= last; row++) {
        CTmCell** cell = &m_grid[row * TM_GRID_COLS];
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
    CTmCell* cell = m_grid[row * TM_GRID_COLS + col];
    if (cell == 0) {
        return 0;
    }
    if (cell->m_deathAnimStarted != 0) {
        NotifyCell(row, col, 0);
        return 0;
    }
    // The grid cell is a real CGrunt (m_grid holds "Grunt" sprites); route it via the
    // real CGrunt methods so the calls bind (ExitGrid==BuildGruntExitAnimation @0x641b0,
    // Route==LoadGruntDeathAnimations @0x60150). The m_grid CTmCell*->CGrunt* retype is
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
// 60->65: the "5 = all" decode is now the if/else form (jmp-merge). Residual is a
// byte-or-vs-dword-RMW regalloc cascade: our cl pins the zero constant in ebp and peepholes
// `m_8 |= 0x10000` to `or byte [mem+2],1` (frees a reg -> keeps `this` in ebx); retail pins
// zero in ebx and emits the full `mov r,[m_8];or r,0x10000;mov [m_8],r` RMW (the reg temp
// forces `this` to spill to [esp+0x10] + one extra `push ecx` frame slot). Same expr matches
// as dword-RMW in the lower-pressure ResetAll; not source-steerable here. topic:wall topic:regalloc.
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
        CTmCell** cell = &m_grid[row * TM_GRID_COLS];
        i32* perRow = m_rowStateB + row;
        i32 g2 = row * TM_GRID_COLS;
        do {
            i32 col = 0;
            do {
                CTmCell* c = *cell;
                if (c != 0) {
                    c->m_38->m_flags |= 0x10000;
                    *cell = 0;
                    m_cellFlag[g2 + col] = 0;
                }
                col++;
                cell++;
            } while (col < 15);
            *reinterpret_cast<i32*>((reinterpret_cast<char*>(perRow) - 0x100)) = 0;
            perRow[0] = 0;
            perRow[4] = 0;
            perRow++;
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
// reassociation/scheduling residual (~85%): the scroll/view loads + the CellHitTest arg
// pushes are byte-exact; retail loads scroll[0]/[4] together up front and accumulates px
// from scroll[0] (`(scroll0-view10)+sx`), our cl reloads sx and accumulates from it
// (`(sx-view10)+scroll0`) - same value, swapped operand order. topic:wall topic:scheduling.
RVA(0x0006be30, 0x47)
void* CTriggerMgr::ScreenToCell(i32 sx, i32 sy, i32* outRow, i32* outCol, i32 startRow) {
    CGameLevel* view = m_world->m_level;
    i32 px = view->m_mainPlane->m_originX - view->m_planeCtx.left + sx;
    i32 py = view->m_mainPlane->m_originY - view->m_planeCtx.top + sy;
    return CellHitTest(px, py, outRow, outCol, startRow);
}

// 0x6bea0: CellHitTest - scan the grid for the cell whose 30x30 object bounds contain
// (px,py). startRow==5 means "rows 0..3"; otherwise just that one row. Writes the hit
// (row,col) through the out-ptrs and returns the cell pointer (0 when none).
// @early-stop
// 70.9->71.2: the "5 = all" decode is now the if/else form (row/last kept in eax/edx).
// Residual regalloc wall: retail spills px to [esp+0x1c] and reloads it for each box-edge
// compare (freeing esi to precompute y0+30), and tail-merges the loop exit; our cl pins px
// in ebx. High register pressure (5 args + this + nested loop) -> different spill picks.
// Logic + offsets byte-exact. topic:wall topic:regalloc.
RVA(0x0006bea0, 0xe2)
void* CTriggerMgr::CellHitTest(i32 px, i32 py, i32* outRow, i32* outCol, i32 startRow) {
    i32 row, last;
    if (startRow == 5) {
        row = 0;
        last = 3;
    } else {
        last = startRow;
        row = startRow;
    }
    while (row <= last) {
        CTmCell** cell = &m_grid[row * TM_GRID_COLS];
        for (i32 col = 0; col < 15; col++) {
            CTmCell* g = cell[col];
            if (g != 0 && g->m_entranceCommitted != 0) {
                CWwdGameObjectA* o = g->m_10;
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
    }
    return 0;
}

// 0x6bfd0: ResetCell(col, row, force, ...) - if grid[row*15+col] (+0x1c) is live: for a
// non-magic row, run its three sub-state resetters then re-seed its CombatTimeout config
// fields (+0x880..+0x88c); for the magic row (== g_curPlayer), when not forced recycle the
// (row,col) record node onto the free list, AddTail it to +0x240, and run ResetMagic. ret 1
// only when a magic cell was recycled, else 0. (__stdcall: ret 0x10.)
// @early-stop
// regalloc + free-list-recycle scheduling wall: the node-bias recycle and the GetInt arg
// push order pin ebx/edi differently than retail. Logic + offsets byte-exact. topic:wall.
RVA(0x0006bfd0, 0x106)
i32 CTriggerMgr::ResetCell(i32 col, i32 row, i32 force, i32 keep) {
    i32 idx = col * TM_GRID_COLS + row;
    CTmCell* cell = m_grid[idx];
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
        cell->m_888 = g_buteMgr.GetDwordDef("Grunt", "CombatTimeout", 0x1388);
        cell->m_88c = 0;
        cell->m_880 = g_frameTime;
        cell->m_884 = 0;
        return 0;
    }
    if (force != 0) {
        if (keep == 0) {
            if (this->Reset3(col, row, 0) != 0) {
                return 1;
            }
        }
    } else {
        this->RefreshC(); // self-call 0x6c068 (reloc-masked)
    }
    CoordPoolNode* node = g_coordPool.m_freeHead;
    i32* slot = 0;
    if (node->m_next != 0) {
        slot = reinterpret_cast<i32*>(&node->m_coord);
        slot[0] = col;
        slot[1] = row;
        g_coordPool.m_freeHead = g_coordPool.m_freeHead->m_next;
    }
    m_recList.AddTail(slot);
    // RECOVERED: the 0-arg i32 call in this function's set is the ILT thunk 0x24c8 ->
    // 0x4b130 = ?CommitArrival@CGrunt@@QAEHXZ. `ResetMagic` was a phantom name for it.
    return cell->CommitArrival();
}

// @early-stop
// /GX branchy nested-jump-table megafunction wall (~10%): validated top reconstructed
// (prologue, grid clamp, cell-tag resolve, primary switch + first arm: FindChild(key,7),
// SwitchDown, the m_list2/m_list1 claim walks and both diagnostic stanzas); the 20-way +
// nested 7-way dispatch, the twelve near-identical list-walk/CString-format arms
// and the /GX EH-state thread across 3426 B are the documented wall. Final-sweep.
// docs/patterns/jumptable-data-overlap.md; big-seh-fuzzy-desync.md; eh-state-numbering-base.md.
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
    CPlaneRender* plane = level->m_mainPlane;
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
    TtcNode* n;
    trig = state->m_beginMarker;
    for (n = TtcHead(trig->m_list2); n != 0; n = n->m_next) {
        CTileTriggerLogic* el = static_cast<CTileTriggerLogic*>(n->m_data);
        if (el->FindIndexByKey(sw->m_key1) != 0) {
            anyHit = 1; // retail branches into the shared success tail (0x6cc7e)
            break;
        }
    }
    trig = state->m_beginMarker;
    for (n = TtcHead(trig->m_list1); n != 0; n = n->m_next) {
        CTileTriggerLogic* el = static_cast<CTileTriggerLogic*>(n->m_data);
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

// 0x6d300: ApplySwitch(sx, sy) - the /GX switch-logic driver. Clamp (sx,sy) to the plane,
// sample the tile attribute, decode the logic class, switch over the kind dispatching the
// matching switch/trigger logic object's Apply; on a miss Format an error CString ("No switch
// logic found for switch at: x=%d, y=%d" / "No trigger logic ...") into a stack temp and
// ReportError. Reconstructed to plateau.
// ARITY FIXED (2026-07-14): retail ends `ret 0xc` = THREE dwords - every caller
// (GruntSteps/Grunt.cpp/GruntCombat) pushes (grunt, x, y); the old 2-arg spelling
// emitted `ret 8`. The grunt arg is unread by this driver (the callers pass it for
// the logic objects that need the actor; this body dispatches by tile only).
// @early-stop
// big /GX switch driver (0x5b2 B): the dense jump table + the six CString-error stanzas
// (ctor/Format/ReportError/dtor under the EH frame) diverge wholesale in regalloc and the
// __ehfuncinfo state numbering; the validated head + the error-Format shape are faithful.
// topic:wall topic:eh.
RVA(0x0006d300, 0x5b2)
i32 CTriggerMgr::ApplySwitch(CGrunt* g, i32 sx, i32 sy) {
    static_cast<void>(g);
    char* plane = reinterpret_cast<char*>(g_gameReg->m_curState);
    char* view = *reinterpret_cast<char**>((reinterpret_cast<char*>(m_world) + 0x24));
    i32 x = sx;
    i32 y = sy;
    if (x < 0) {
        x = 0;
    } else {
        i32 w = *reinterpret_cast<i32*>((*reinterpret_cast<char**>(view + 0x5c) + 0x30));
        if (x >= w) {
            x = w - 1;
        }
    }
    if (y < 0) {
        y = 0;
    } else {
        i32 h = *reinterpret_cast<i32*>((*reinterpret_cast<char**>(view + 0x5c) + 0x34));
        if (y >= h) {
            y = h - 1;
        }
    }
    char* scroll = *reinterpret_cast<char**>((view + 0x5c));
    i32 sh = *reinterpret_cast<i32*>((scroll + 0x8c));
    i32 sw = *reinterpret_cast<i32*>((scroll + 0x90));
    i32 cell = *reinterpret_cast<i32*>(*reinterpret_cast<char**>(scroll + 0x24) + (y >> sw) * 4) + (x >> sh);
    i32 attr = *reinterpret_cast<i32*>((*reinterpret_cast<char**>(scroll + 0x20) + cell * 4));
    i32 kind;
    if (attr == static_cast<i32>(0xeeeeeeee) || attr == -1) {
        kind = 0;
    } else {
        CUserLogic* logic = static_cast<CUserLogic*>(*reinterpret_cast<void**>((*reinterpret_cast<char**>(view + 0x4c) + (attr & 0xffff) * 4)));
        kind = logic->UserLogicVfunc6(); // Apply = vtbl slot 8 (+0x20)
    }
    i32 op = kind - 0x34;
    if (static_cast<u32>(op) > 0xe) {
        return 0;
    }
    i32 cx = x;
    i32 cy = y;
    CTileTriggerSwitchLogic* obj =
        static_cast<CTileTriggerSwitchLogic*>(*reinterpret_cast<void**>((*reinterpret_cast<char**>(plane + 0x2e4) + 0)));
    if (obj == 0) {
        CString msg;
        msg.Format("No switch logic found for switch at: x=%d, y=%d", cx >> 5, cy >> 5);
        g_gameReg->ReportError(TRIGERR_LOOKUP_MISS, 0x3f7);
        return 0;
    }
    obj->SwitchUp(); // vtbl slot 3 (+0xc) - retail calls it NO-ARG here (the SwitchLogic
                     // hierarchy's own slot 3; the old CUserLogic spelling conflated the two trees)
    return 1;
}

// retail 0x6da60/0x6daa0 end at the void EnqueueSingle call with NO eax write, so void
// (a `return 0` adds a spurious xor eax,eax). Orphan copies - no caller in the tree.
RVA(0x0006da60, 0x27)
void __stdcall GridAction6(i32 a, i32 b) {
    g_gameReg->m_cmdSubMgr->EnqueueSingle(1, a, b, 6, 0, 0, 0, 0);
}

RVA(0x0006daa0, 0x27)
void __stdcall GridAction7(i32 a, i32 b) {
    g_gameReg->m_cmdSubMgr->EnqueueSingle(1, a, b, 7, 0, 0, 0, 0);
}

// 0x6dae0: ApplyTriggerA(a18, col, row, a24, a28, a2c) - look up grid[a18*15+col]; if live,
// un-pending and matching the snapped source pos, dispatch the cell's trigger logic by its
// kind (the 0x13/0xf branch families); update its state and return the applier result. Else
// -1 / 0. (__stdcall: ret 0x10.) Reconstructed to plateau.
// @early-stop
// big branchy trigger-applier (0x4b7 B): the kind-dispatch ladder + the snapped-pos compares
// pin esi(cell)/edi/ebp differently than retail; the body is structurally faithful but its
// regalloc diverges across the many branches. topic:wall.
RVA(0x0006dae0, 0x4b7)
i32 CTriggerMgr::ApplyTriggerA(i32 col, i32 row, i32 a24, i32 a28) {
    CTmCell* cell = m_grid[col * TM_GRID_COLS + row];
    if (cell == 0 || cell->m_entranceCommitted == 0) {
        return 0;
    }
    CGameObject* o = cell->m_10;
    if (o->m_screenX != cell->m_lastTilePxX) {
        if (o->m_screenY != cell->m_lastTilePxY) {
            return -1;
        }
    }
    i32 k = cell->m_entranceReason;
    if (k > 0x16) {
        k = cell->m_19c;
    }
    if (k == 0x13) {
        CTmCell* tc = cell;
        if (tc->Type13Check() != 0) {
            tc->Apply13(row, a28 + 1);
            return 1;
        }
    }
    if (k == 0xf) {
        cell->Dispatch(k, row);
    }
    return 0;
}

// 0x6e120: ApplyTriggerB(a1c, col, row, a28, a2c, a30) - the exit variant of ApplyTriggerA:
// same cell lookup + validation, then snap (a28,a2c) to a tile and route the cell's exit
// logic; updates +0x384 and returns the applier's boolean. (__stdcall: ret 0x10.)
// Reconstructed to plateau.
// @early-stop
// big branchy trigger-applier (0x552 B): mirrors ApplyTriggerA's wall - kind-dispatch ladder
// + snapped-box arithmetic diverge in regalloc across the branches. topic:wall.
RVA(0x0006e120, 0x552)
i32 CTriggerMgr::ApplyTriggerB(i32 col, i32 row, i32 a28, i32 a2c) {
    CTmCell* cell = m_grid[col * TM_GRID_COLS + row];
    if (cell == 0 || cell->m_entranceCommitted == 0 || cell->m_entranceActive != 0) {
        return 0;
    }
    CGameObject* o = cell->m_10;
    if (o->m_screenX != cell->m_lastTilePxX) {
        if (o->m_screenY != cell->m_lastTilePxY) {
            return -1;
        }
    }
    if (o->m_screenX == cell->m_lastTilePxX && o->m_screenY == cell->m_lastTilePxY
        && cell->m_198 != 0x1e && g_traitorMode == 0) {
        return 0;
    }
    i32 by = (a2c & ~0x1f) + 0x10;
    i32 bx = (a28 & ~0x1f) + 0x10;
    cell->m_coordRetryCount = 0;
    i32 r = cell->ApplyBox(bx, by, row, -1, 1, 0);
    return r != 0 ? 1 : 0;
}

RVA(0x0006e7e0, 0x5)
CGrunt* CTriggerMgr::FindAtPixel(i32 x, i32 y) {
    return 0;
}

// 0x6e800: ClearCell(col, row, a18, a1c, a20) - if grid[col*15+row] is live, reset its
// trigger/anim sub-state (unless already cleared via +0x420), bail if it has a pending
// flag (+0x1e4), look up its config name; when it equals "I" run the manager's fx with the
// cell's pose; then ApplyBox the snapped (a18..a20) bounds and return its boolean result.
// (__stdcall: ret 0x14.)
// @early-stop
// regalloc + inline-strcmp wall: the "I" compare inlines as a byte loop pinning ah/bl
// differently than retail and the box arithmetic spills. Logic + offsets byte-exact.
RVA(0x0006e800, 0x189)
i32 CTriggerMgr::ClearCell(i32 col, i32 row, i32 a18, i32 a1c, i32 a20) {
    i32 idx = col * TM_GRID_COLS + row;
    CTmCell* cell = m_grid[idx];
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
        cell->Disarm(1, 1);
    }
    if (cell->m_entranceActive != 0) {
        return 0;
    }
    char* name = *static_cast<CActReg&>(g_typeColl).ResolveSlot_46e0c0(reinterpret_cast<i32>(cell->m_14->m_1c)); // g_typeColl IS the 0x6bf650 registry (TypeKeyColl.cpp's own verdict)
    if (strcmp(name, "I") == 0) {
        i32 px = cell->m_moveTileX;
        i32 py = cell->m_moveTileY;
        this->Fx(px, py, py, cell->m_entranceReason, -1, py);
    }
    i32 by = (a20 & ~0x1f) + 0x10;
    i32 bx = (a1c & ~0x1f) + 0x10;
    cell->m_coordRetryCount = 0;
    i32 r = cell->ApplyBox(bx, by, a18, -1, 1, 0);
    return r != 0 ? 1 : 0;
}

// 0x6ea00: HitTestApply(x, y, kind) - hit-test the cell at (x,y); only for the magic group
// (out-col == g_curPlayer) and a cell whose config name is NOT "B" and kind 0x14, add the world's
// score delta, zero the status fields, SetStat(0,0xbb7), re-arm the status item (SetMode 1)
// and ClearMagic(g_curPlayer). void - no path materialises a return value. (__stdcall: ret 0xc.)
// @early-stop
// inline-strcmp result-register coloring wall (~80%): void return + strcmp `!= 0` bool steer +
// i64 score sub are byte-exact and size now matches retail (0x125). The residual is the inline
// strcmp landing its sbb result in ecx (retail eax) with the `differ` bool in al vs retail's cl,
// so the `setne`+null-test colors as `cmpb bl,al` vs retail `testb cl,cl`. Not source-steerable
// (the `bool` local is required for the setne form but shifts the result register). topic:wall.
RVA(0x0006ea00, 0x125)
void CTriggerMgr::HitTestApply(i32 x, i32 y, i32 kind) {
    i32 outRow = 0;
    i32 outCol = 0;
    CTmCell* cell = this->Hit(kind, y, y, &outRow, &outCol);
    if (cell == 0 || outCol != g_curPlayer) {
        return;
    }
    char* name = *static_cast<CActReg&>(g_typeColl).ResolveSlot_46e0c0(reinterpret_cast<i32>(cell->m_14->m_1c)); // g_typeColl IS the 0x6bf650 registry (TypeKeyColl.cpp's own verdict)
    bool differ = strcmp(name, "B") != 0;
    if (!differ) {
        return;
    }
    i32 k = cell->m_entranceReason;
    if (k > 0x16) {
        k = cell->m_19c;
    }
    if (k != 0x14) {
        return;
    }
    CPlay* world = static_cast<CPlay*>(g_gameReg->m_curState);
    // world->m_3f4 IS CPlay::m_frameMarker (the CTimer): read its i64 start stamp
    // (m_38:m_3c) as the elapsed accumulator, credit the HUD score, then zero the
    // timer's accum/lap/running/current block.
    CTimer* sub = world->m_frameMarker;
    i64 diff = static_cast<i64>(static_cast<u32>(g_frameTime)) - *reinterpret_cast<i64*>(&sub->m_38);
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
    this->ClearMagic(g_curPlayer);
}
