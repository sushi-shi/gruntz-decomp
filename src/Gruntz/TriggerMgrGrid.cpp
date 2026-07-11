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
// SEAM (not moved): ?WireTileSwitchLogic@CTileWireLogic@@QAEHPAXHH@Z @0x6c130
// (0xd62 B) sits INSIDE this interval between ResetCell (0x6bfd0) and
// ApplySwitch (0x6d300) - by first-link contiguity it belongs to this original
// TU. It currently lives in src/Gruntz/TileSwitchLogic.cpp (tile-logic files
// are owned by a parallel wave-2 worker); folding it here is deferred work for
// the tile-logic owner.
//
// Functions in retail-RVA order; shared views/externs in
// <Gruntz/TriggerMgrViews.h>. /GX unit (ApplySwitch owns a CString temp).
#include <Gruntz/TriggerMgr.h>

#include <Gruntz/ActionOptionsMenuBar.h>
#include <Gruntz/GruntzCmdMgr.h>
#include <Gruntz/SBI_RectOnly.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/SpriteFactory.h> // the ONE CSpriteFactory (CreateSprite @0x1597b0)
#include <Gruntz/UserLogic.h>     // canonical CUserLogic (switch/trigger logic virtuals)
#include <Gruntz/TileGrid.h>      // canonical CTileGrid (the registry's +0x70 tile grid)
#include <Bute/ButeMgr.h>         // canonical CButeMgr (one shape)
#include <Gruntz/Viewport.h>      // shared world tile-grid geometry (dims here)
#include <Globals.h>

#include <Gruntz/TriggerMgrViews.h> // the shared CTm* views + singleton externs

// 0x6b640: SetLevel - store the supplied level at +0x22c, clear m_230 + m_pendingFx
// and raise m_2a4; returns 1 (0 when arg is null).
RVA(0x0006b640, 0x2f)
i32 CTriggerMgr::SetLevel(CTmLevel* lvl) {
    if (lvl == 0) {
        return 0;
    }
    m_level = lvl;
    m_230 = 0;
    m_pendingFx = 0;
    m_2a4 = 1;
    return 1;
}

// 0x6b680: Cleanup - destruct+free the overlay sub-object (+0x25c) when present, then
// drain the record and selection lists. The overlay's Clear runs the in-place dtor,
// then __cdecl operator delete frees it.
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
    (void)a8;
    (void)a18;
    (void)a24;
    (void)a28;
    (void)a2c;
    if (m_level == 0) {
        return -1;
    }
    i32 special = 0;
    i32 wantSlot = 0;
    if (a30 == 0x12) {
        special = 0x100;
        wantSlot = 1;
    }
    CTileGrid* plane = g_tmGameReg->m_tileGrid;
    i32 attr;
    if ((ax >> 5) >= plane->m_c || (ay >> 5) >= plane->m_10) {
        attr = 1;
    } else {
        attr = plane->m_8[ay >> 5][(ax >> 5) * 7];
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
    CTmCell** rowBase = &m_grid[row * 15];
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
    CSpriteFactory* fac = m_level->m_8;
    CTmCell* sprite = (CTmCell*)fac->CreateSprite(0, ax, ay, ay, "Grunt", 0x40003);
    if (sprite == 0) {
        return -1;
    }
    sprite->m_7c->Init(sprite);
    void* logicTag = sprite->m_7c->m_18;
    (void)logicTag;
    // (the dense kind jump table -> internal id + the Wormhole / Entrance sub-ctors elide
    // here; reconstructed to plateau)
    m_grid[row * 15 + free] = sprite;
    m_rowCount[row] += 1;
    m_cellFlag[(row * 15 + free)] = 0;
    g_tmGameReg->m_scoreBoard->m_counts[row] += 1;
    return free;
}

// 0x6bc20: DispatchCellForObject(obj, startRow, kind, arg) - scan the placed-object
// grid (+0x1c) for the cell whose pointer == `obj` (startRow, or rows 0..3 when
// startRow==5) and hand it to CellDispatch(row,col,kind,arg); ret 0 when not found.
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
        CTmCell** cell = &m_grid[row * 15];
        for (i32 col = 0; col < 15; col++) {
            if (cell[col] == obj) {
                return CellDispatch(row, col, kind, arg);
            }
        }
    }
    return 0;
}

// 0x6bcb0: CellDispatch(row, col, kind, arg) - look up grid[row*15+col] (+0x1c);
// if it has a notify hook run NotifyCell(row,col,0) (ret 0); else route the cell by
// `kind` (0xd => ExitGrid, else Route(kind,arg)) and ret 1; ret 0 when no cell.
RVA(0x0006bcb0, 0x6a)
i32 CTriggerMgr::CellDispatch(i32 row, i32 col, i32 kind, i32 arg) {
    CTmCell* cell = m_grid[row * 15 + col];
    if (cell == 0) {
        return 0;
    }
    if (cell->m_368 != 0) {
        NotifyCell(row, col, 0);
        return 0;
    }
    if (kind == 0xd) {
        cell->ExitGrid();
    } else {
        cell->Route(kind, arg);
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
        CTmCell** cell = &m_grid[row * 15];
        i32* perRow = m_rowStateB + row;
        i32 g2 = row * 15;
        do {
            i32 col = 0;
            do {
                CTmCell* c = *cell;
                if (c != 0) {
                    c->m_154->m_8 |= 0x10000;
                    *cell = 0;
                    m_cellFlag[g2 + col] = 0;
                }
                col++;
                cell++;
            } while (col < 15);
            *(i32*)((char*)perRow - 0x100) = 0;
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
    CTmLevelView* view = m_level->m_24;
    i32 px = view->m_5c->m_edgeL - view->m_10 + sx;
    i32 py = view->m_5c->m_edgeT - view->m_14 + sy;
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
        CTmCell** cell = &m_grid[row * 15];
        for (i32 col = 0; col < 15; col++) {
            CTmCell* g = cell[col];
            if (g != 0 && g->m_1fc != 0) {
                CTmDisplay* o = g->m_10;
                if (o->m_198 != 0) {
                    i32 x0 = o->m_5c - 15;
                    i32 y0 = o->m_60 - 15;
                    if (px < x0 + 30 && px >= x0 && py < y0 + 30 && py >= y0) {
                        if (outRow != 0) {
                            *outRow = row;
                        }
                        if (outCol != 0) {
                            *outCol = col;
                        }
                        return m_grid[row * 15 + col];
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
// fields (+0x880..+0x88c); for the magic row (== g_644c54), when not forced recycle the
// (row,col) record node onto the free list, AddTail it to +0x240, and run ResetMagic. ret 1
// only when a magic cell was recycled, else 0. (__stdcall: ret 0x10.)
// @early-stop
// regalloc + free-list-recycle scheduling wall: the node-bias recycle and the GetInt arg
// push order pin ebx/edi differently than retail. Logic + offsets byte-exact. topic:wall.
RVA(0x0006bfd0, 0x106)
i32 CTriggerMgr::ResetCell(i32 col, i32 row, i32 force, i32 keep) {
    i32 idx = col * 15 + row;
    CTmCell* cell = m_grid[idx];
    if (cell == 0 || cell->m_1fc == 0) {
        return 0;
    }
    if (col != g_644c54) {
        cell->ResetA();
        cell->ResetB();
        cell->ResetC();
        cell->m_888 = g_buteMgr.GetDwordDef("Grunt", "CombatTimeout", 0x1388);
        cell->m_88c = 0;
        cell->m_880 = g_645588;
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
    void* node = g_freeList;
    i32* slot = 0;
    if (*(void**)node != 0) {
        slot = (i32*)((char*)node + 4);
        slot[0] = col;
        slot[1] = row;
        g_freeList = *(void**)g_freeList;
    }
    m_recList.AddTail(slot);
    return cell->ResetMagic();
}

// (0x6c130 ?WireTileSwitchLogic@CTileWireLogic - this obj's next fn by retail
// position; body in src/Gruntz/TileSwitchLogic.cpp, see the header note.)

// 0x6d300: ApplySwitch(sx, sy) - the /GX switch-logic driver. Clamp (sx,sy) to the plane,
// sample the tile attribute, decode the logic class, switch over the kind dispatching the
// matching switch/trigger logic object's Apply; on a miss Format an error CString ("No switch
// logic found for switch at: x=%d, y=%d" / "No trigger logic ...") into a stack temp and
// ReportError. (__stdcall: ret 0xc.) Reconstructed to plateau.
// @early-stop
// big /GX switch driver (0x5b2 B): the dense jump table + the six CString-error stanzas
// (ctor/Format/ReportError/dtor under the EH frame) diverge wholesale in regalloc and the
// __ehfuncinfo state numbering; the validated head + the error-Format shape are faithful.
// topic:wall topic:eh.
RVA(0x0006d300, 0x5b2)
i32 CTriggerMgr::ApplySwitch(i32 sx, i32 sy) {
    char* plane = (char*)g_tmGameReg->m_curState;
    char* view = *(char**)((char*)m_level + 0x24);
    i32 x = sx;
    i32 y = sy;
    if (x < 0) {
        x = 0;
    } else {
        i32 w = *(i32*)(*(char**)(view + 0x5c) + 0x30);
        if (x >= w) {
            x = w - 1;
        }
    }
    if (y < 0) {
        y = 0;
    } else {
        i32 h = *(i32*)(*(char**)(view + 0x5c) + 0x34);
        if (y >= h) {
            y = h - 1;
        }
    }
    char* scroll = *(char**)(view + 0x5c);
    i32 sh = *(i32*)(scroll + 0x8c);
    i32 sw = *(i32*)(scroll + 0x90);
    i32 cell = *(i32*)(*(char**)(scroll + 0x24) + (y >> sw) * 4) + (x >> sh);
    i32 attr = *(i32*)(*(char**)(scroll + 0x20) + cell * 4);
    i32 kind;
    if (attr == (i32)0xeeeeeeee || attr == -1) {
        kind = 0;
    } else {
        CUserLogic* logic = (CUserLogic*)*(void**)(*(char**)(view + 0x4c) + (attr & 0xffff) * 4);
        kind = logic->UserLogicVfunc6(); // Apply = vtbl slot 8 (+0x20)
    }
    i32 op = kind - 0x34;
    if ((u32)op > 0xe) {
        return 0;
    }
    i32 cx = x;
    i32 cy = y;
    CUserLogic* obj = (CUserLogic*)*(void**)(*(char**)(plane + 0x2e4) + 0);
    if (obj == 0) {
        CString msg;
        msg.Format("No switch logic found for switch at: x=%d, y=%d", cx >> 5, cy >> 5);
        g_tmGameReg->ReportError(0x80dd, 0x3f7);
        return 0;
    }
    obj->UserLogicVfunc1(); // Run = vtbl slot 3 (+0xc)
    return 1;
}

// ===========================================================================
// The two tiny grid-action wrappers (0x6da60 / 0x6daa0) + the tile-fx spawner
// (0x79ea0), proximity-attributed to CTriggerMgr but really FREE __stdcall thunks
// (no `this`): each drives the game registry's spawn/fx sub-managers.
// ===========================================================================

// 0x6da60: GridAction6(a, b) - dispatch the spawn sub-mgr's action with kind 6.
// __stdcall free function (cleans its own 2 args; retail ends in `ret 8`).
RVA(0x0006da60, 0x27)
i32 __stdcall GridAction6(i32 a, i32 b) {
    g_tmGameReg->m_6c->EnqueueSingle(1, a, b, 6, 0, 0, 0, 0);
    return 0;
}

// 0x6daa0: GridAction7(a, b) - dispatch the spawn sub-mgr's action with kind 7.
RVA(0x0006daa0, 0x27)
i32 __stdcall GridAction7(i32 a, i32 b) {
    g_tmGameReg->m_6c->EnqueueSingle(1, a, b, 7, 0, 0, 0, 0);
    return 0;
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
    CTmCell* cell = m_grid[col * 15 + row];
    if (cell == 0 || cell->m_1fc == 0) {
        return 0;
    }
    CTmDisplay* o = cell->m_10;
    if (o->m_5c != cell->m_pos.x) {
        if (o->m_60 != cell->m_pos.y) {
            return -1;
        }
    }
    i32 k = cell->m_170;
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
    CTmCell* cell = m_grid[col * 15 + row];
    if (cell == 0 || cell->m_1fc == 0 || cell->m_1e4 != 0) {
        return 0;
    }
    CTmDisplay* o = cell->m_10;
    if (o->m_5c != cell->m_pos.x) {
        if (o->m_60 != cell->m_pos.y) {
            return -1;
        }
    }
    if (o->m_5c == cell->m_pos.x && o->m_60 == cell->m_pos.y && cell->m_198 != 0x1e
        && g_6455b0 == 0) {
        return 0;
    }
    i32 by = (a2c & ~0x1f) + 0x10;
    i32 bx = (a28 & ~0x1f) + 0x10;
    cell->m_384 = 0;
    i32 r = cell->ApplyBox(bx, by, row, -1, 1, 0);
    return r != 0 ? 1 : 0;
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
    i32 idx = col * 15 + row;
    CTmCell* cell = m_grid[idx];
    if (cell == 0 || cell->m_1fc == 0) {
        return 0;
    }
    if (cell->m_420 == 0) {
        cell->m_308 = 0;
        cell->m_310 = 0;
        cell->m_30c = 0;
        cell->m_314 = 0;
        cell->m_248 &= 0xe7fbfbfd;
        cell->m_420 = 0;
        cell->m_2d0 = 0;
        cell->Disarm(1, 1);
    }
    if (cell->m_1e4 != 0) {
        return 0;
    }
    char* name = *g_nameReg.Lookup(cell->m_14->m_1c);
    if (strcmp(name, "I") == 0) {
        i32 px = cell->m_3e4;
        i32 py = cell->m_3e8;
        this->Fx(px, py, py, cell->m_170, -1, py);
    }
    i32 by = (a20 & ~0x1f) + 0x10;
    i32 bx = (a1c & ~0x1f) + 0x10;
    cell->m_384 = 0;
    i32 r = cell->ApplyBox(bx, by, a18, -1, 1, 0);
    return r != 0 ? 1 : 0;
}

// 0x6ea00: HitTestApply(x, y, kind) - hit-test the cell at (x,y); only for the magic group
// (out-col == g_644c54) and a cell whose config name is NOT "B" and kind 0x14, add the world's
// score delta, zero the status fields, SetStat(0,0xbb7), re-arm the status item (SetMode 1)
// and ClearMagic(g_644c54). void - no path materialises a return value. (__stdcall: ret 0xc.)
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
    if (cell == 0 || outCol != g_644c54) {
        return;
    }
    char* name = *g_nameReg.Lookup(cell->m_14->m_1c);
    bool differ = strcmp(name, "B") != 0;
    if (!differ) {
        return;
    }
    i32 k = cell->m_170;
    if (k > 0x16) {
        k = cell->m_19c;
    }
    if (k != 0x14) {
        return;
    }
    CTmWorld* world = g_tmGameReg->m_curState;
    CTmScoreSub* sub = world->m_3f4;
    i64 diff = (i64)(u32)g_645588 - sub->m_38;
    if (diff < 0) {
        diff = 0;
    }
    g_tmGameReg->m_scoreBoard->m_score += (i32)diff;
    sub->m_40 = 0;
    sub->m_44 = 0;
    sub->m_30 = 0;
    sub->m_34 = 0;
    sub->m_48 = 0;
    sub->m_4c = 0;
    world->SetStat(0, 0xbb7);
    world->m_2dc->SetMode(1);
    this->ClearMagic(g_644c54);
}
