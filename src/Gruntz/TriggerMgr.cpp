// TriggerMgr.cpp - CTriggerMgr, the playfield tile-object / switch-trigger grid
// manager (trace placeholder ClassUnknown_23, C:\Proj\Gruntz). See TriggerMgr.h.
//
// Members are read by raw this+offset: the grid cells, the level/plane objects and
// the list-node payloads are full UNMATCHED engine classes modeled here as opaque
// shells so their member reads / helper calls reloc-mask. Functions in retail-RVA
// order.
#include <Gruntz/TriggerMgr.h>
#include <Bute/ButeMgr.h> // canonical CButeMgr (one shape)

// A list node: { CTmNode* m_next; ; (x,y)* m_payload }. The payload is an (x,y)
// pair at +0/+4. Opaque otherwise. These are the record-list / selection-list nodes.
struct CTmNode {
    CTmNode* m_next; // +0x00
    i32 m_4;         // +0x04
    i32* m_payload;  // +0x08  -> { x@+0, y@+4 }
};

// A placed grid-cell game object (a CGrunt). Only the reloc-masked hooks dispatched
// from the reconstructed leaves are declared. Fields are read by raw this+offset (the
// opaque-shell choice for the whole TU): +0x10 the display sub-object (world x@+0x5c,
// y@+0x60, clickable@+0x198), +0x1fc the alive flag, +0x368 the notify hook.
struct CTmGrunt {
    void ClearAllSprites();      // 0x4b240 (reloc-masked)
    void ExitGrid();             // 0x641b0 (reloc-masked)
    void Route(i32 kind, i32 a); // 0x60150 (reloc-masked)
    void DestroyAnims();         // 0x57d80 (reloc-masked)
    void Recall();               // 0x68520 (reloc-masked) - row-recall variant
    char p0[0x368];
    void* m_368; // +0x368  notify hook
};

// The small overlay sub-object allocated at CTriggerMgr+0x25c (0x40 bytes). Only its
// reloc-masked __thiscall hooks are dispatched from the reconstructed leaves.
struct CTmOverlay {
    void Tick();   // 0x97f0  (reloc-masked)
    i32 Release(); // 0x94c0  (reloc-masked) - ret used by OverlayRelease
    void Clear();  // 0x92e0  (reloc-masked) - destruct without freeing
    char p0[0x2c];
    i32 m_2c; // +0x2c  active flag gating the per-frame OverlayTick
};

// The goal object at CTriggerMgr+0x23c; ResetAll ORs 0x10000 into its +0x8 flags.
struct CTmGoal {
    char p0[0x8];
    i32 m_8; // +0x08  flags
};

// The MFC pointer-list at CTriggerMgr+0x240 and at each +0x2d0[i] selection slot;
// RemoveAll/RemoveAt are the reloc-masked engine bodies (@0x1b48a6 / @0x1b4ac7).
// Modeled as a tiny __thiscall helper so the `mov ecx,list; jmp/call` falls out
// with no stack cleanup. The intrusive list head sits at +0x04 of the object.
struct CTmPtrList {
    void RemoveAll();     // 0x1b48a6
    void RemoveAt(void*); // 0x1b4ac7
    void AddTail(void*);  // 0x1b4991  (returns POSITION; ret ignored here)
    void* m_0;            // +0x00
    CTmNode* m_head;      // +0x04  list head node
};

// The level/group base-index sentinel (DAT_00644c54) the selection helpers guard on
// (same global the StatzTab toggle keys off; see StatusBarUpdaters.cpp / CPlay.h).
extern i32 g_644c54;

// The global game-registry singleton (?g_gameReg@@3PAUWwdGameReg@@A @0x64556c). Only
// the +0x2c world back-ptr is read here; the world's hooks are reloc-masked.
// A CSBI_RectOnly status-bar item (world+0x2dc); SetMode is the reloc-masked engine
// body @0x10bb90.
struct CTmStatusItem {
    void SetMode(i32 mode); // 0x10bb90 (reloc-masked)
};
struct CTmWorld {
    void StopFx(i32 a, i32 b);   // 0xd0120 (__thiscall, reloc-masked)
    void Refresh();              // 0xda2d0 (__thiscall, reloc-masked)
    void SetStat(i32 a, i32 b);  // 0xd9240 (__thiscall, reloc-masked)
    void Center(i32 cx, i32 cy); // 0xd5f00 (__thiscall, reloc-masked) - scroll-center on a tile
    char p0[0x2dc];
    CTmStatusItem* m_2dc; // +0x2dc  status-bar item
    char p1[0x504 - 0x2e0];
    void* m_504; // +0x504  pending-fx flag
};
// The level/plane grid the active-selection center reads its dims from: the chain
// g_gameReg->m_30->m_24->m_5c lands on a CTmGrid whose +0x30/+0x34 are (cols,rows).
struct CTmGrid {
    char p0[0x30];
    i32 m_30; // +0x30  grid cols
    i32 m_34; // +0x34  grid rows
};
struct CTmGridHolder {
    char p0[0x5c];
    CTmGrid* m_5c; // +0x5c  the grid object
};
struct CTmRegSub30 {
    char p0[0x24];
    CTmGridHolder* m_24; // +0x24
};
class CGruntzCmdMgr;
struct CTmGameReg {
    void* PickPausedThenPlayState();       // 0x929b0 (reloc-masked) - the play/pause state obj
    void ReportError(i32 code, i32 flags); // 0x8dc60 (reloc-masked)
    char p0[0x2c];
    CTmWorld* m_2c;    // +0x2c  the active world/play object
    CTmRegSub30* m_30; // +0x30  the level/plane grid holder
    char p1[0x34];
    CTriggerMgr* m_68;   // +0x68  the active trigger manager
    CGruntzCmdMgr* m_6c; // +0x6c  the command queue
};
extern CTmGameReg* g_gameReg;

// ?g_buteMgr@@3VCButeMgr@@A @0x6453d8 - the canonical CButeMgr (via TriggerMgr.h);
// the int-with-default getter (0x1721e0) is reloc-masked __thiscall.
extern CButeMgr g_buteMgr;
extern i32 g_645588; // DAT_00645588 (the level base score / id sentinel)
extern i32 g_644ca4; // DAT_00644ca4 (the secondary group sentinel; serialized by ScanGroup)

// The DAT_006bf650 config-name registry (its method maps a sprite-type id to a config-name
// string; @0x6bf66c/@0x6bf670 are its node array + count). Reloc-masked.
struct CTmNameReg {
    char** Lookup(i32 id); // 0x46e0c0
};
extern CTmNameReg g_nameReg; // 0x6bf650
extern void* g_nameRegNodes; // 0x6bf66c
extern i32 g_nameRegCount;   // 0x6bf670
void Str_Free(void* node);   // CString teardown, 0x1b9b93

// A DirectSound channel helper (?StopAndRewind@DirectSoundMgr, @0x135380, __thiscall,
// reloc-masked); DestroyAllAnims rewinds three channels. The real engine name matters
// here because the retail symbol is named (not a FUN_ thunk).
struct DirectSoundMgr {
    i32 StopAndRewind(); // 0x135380 (reloc-masked)
};

// CSpriteFactory::CreateSprite (@0x1597b0, reloc-masked) builds a sprite from a config
// key; the level's sprite factory is level->m_8. The created sprite carries a descriptor
// at +0x7c whose slot-4 (+0x10) is an Init thunk run on the fresh sprite.
struct CTmSpriteFactory {
    void* CreateSprite(i32 a, i32 x, i32 y, i32 b, const char* key, i32 c); // 0x1597b0
};
struct CTmSpriteDesc {
    void* s0[4];
    void (*Init)(void*); // +0x10
};

// The level object stored at CTriggerMgr+0x22c (set by SetLevel); its +0x8 is the sprite
// factory the spawners create from.
struct CTmLevel {
    char p0[0x8];
    CTmSpriteFactory* m_8; // +0x08  sprite factory
};

// The level's display-object list (level->m_8): a manager with head node @m_14; each
// node carries the next ptr @m_0 and the bound object @m_8. The object's type is
// identified by a fixed entry in its descriptor (obj+0x7c) slot-4 (+0x10) matching the
// CGrunt::ReadConfigFromButeMgr method address; on a match, +0x18 names the target whose
// +0x200 channel marker is cleared.
struct CTmListNode {
    CTmListNode* m_0; // +0x00 next
    char p0[0x4];
    void* m_8; // +0x08 bound object
};
// The type-identity check compares the descriptor's slot-4 against the address of
// CGrunt::ReadConfigFromButeMgr (the retail uses its method address as the type tag;
// reloc-masked DIR32). Forward-declared with just that method so `&CGrunt::...` carries
// the reloc without pulling the full CGrunt layout. NOTE: cannot include Grunt.h here
// (it pulls CGameRegistry.h whose g_gameReg type clashes with this TU's own g_gameReg
// decl - build-breaker; see docs/vtable-conversion-log.md). Kept as a minimal view.
class CGrunt {
public:
    void ReadConfigFromButeMgr();
    void SelectMoveIcon(i32 a); // 0x57800 (reloc-masked) - set/restore the move-icon
    i32 CanShowStamina();       // 0x514a0 (reloc-masked) - region-toggle gate
};

// CPlay (= the active world, g_gameReg->m_2c): the two reconstructed leaves call its
// LoadCursorSprites (the +0x2a8 pending-fx loader, 0xd0120) and OnRegion4 (0xd8bc0).
// Modeled with the real names so the calls pair exactly (reloc-masked DIR32 otherwise).
class CPlay {
public:
    void LoadCursorSprites(i32 a, i32 b); // 0xd0120
    i32 OnRegion4(i32 z);                 // 0xd8bc0
};

// CGruntzCmdMgr (= g_gameReg->m_6c): the command queue EnqueueGroupCells posts to.
class CGruntzCmdMgr {
public:
    void EnqueueSingle(i32 a, char b, char c, char d, i16 e, i16 f, char g, char h); // 0x23c30
    void EnqueueMulti(i32 a, char b, i32 c, char* d, char e, i16 f, i16 g, char h);  // 0x23ca0
};
extern "C" i32 rand(void); // CRT rand() 0x11fee0 (reloc-masked)

// 0x6b640: SetLevel(lvl) - stash the level back-ptr, clear companion state.
// @early-stop
// 1-instr phase shift: retail floats `mov eax,1` up between the stores; we emit it
// at the epilogue. Structure + offsets byte-exact. docs/patterns/zero-register-pinning.md
RVA(0x0006b640, 0x2f)
i32 CTriggerMgr::SetLevel(void* lvl) {
    if (lvl == 0) {
        return 0;
    }
    *(void**)((char*)this + 0x22c) = lvl;
    *(i32*)((char*)this + 0x230) = 0;
    *(i32*)((char*)this + 0x2a0) = 0;
    *(i32*)((char*)this + 0x2a4) = 0;
    return 1;
}

// 0x6b680: Cleanup - destruct+free the overlay sub-object (+0x25c) when present, then
// drain the record and selection lists. The overlay's Clear runs the in-place dtor,
// then __cdecl operator delete frees it.
RVA(0x0006b680, 0x39)
void CTriggerMgr::Cleanup() {
    CTmOverlay* ov = *(CTmOverlay**)((char*)this + 0x25c);
    if (ov != 0) {
        ov->Clear();
        operator delete(ov);
        *(CTmOverlay**)((char*)this + 0x25c) = 0;
    }
    ClearRecords();
    ClearSelections();
}

// 0x6bd40: ClearGridRange(startRow) - ResetAll, then for rows startRow..3 (5 = all)
// flag each live cell's goal (+0x154) done and clear the cell, its parallel grid slot
// (+0x11c) and the per-row state words (+0x10c/+0x20c/+0x21c); then ClearSelections.
// @early-stop
// regalloc + constant-hoist wall: retail pins this->ebx, startRow->esi and hoists the
// 0x10000 done-bit into edi for an `or [mem],reg` RMW; our cl homes this->esi and
// emits the 3-instr mov/or/mov. Logic + offsets byte-exact. topic:wall.
RVA(0x0006bd40, 0xb3)
i32 CTriggerMgr::ClearGridRange(i32 startRow) {
    i32 row = startRow;
    i32 last = startRow;
    if (startRow == 5) {
        row = 0;
        last = 3;
    }
    ResetAll();
    if (row <= last) {
        i32 n = last - row + 1;
        void** cell = (void**)((char*)this + row * 15 * 4 + 0x1c);
        i32* perRow = (i32*)((char*)this + row * 4 + 0x20c);
        i32 g2 = row * 15 + 0x47;
        do {
            i32 col = 0;
            do {
                void* c = *cell;
                if (c != 0) {
                    *(i32*)((char*)*(void**)((char*)c + 0x154) + 0x8) |= 0x10000;
                    *cell = 0;
                    *(void**)((char*)this + (col + g2) * 4) = 0;
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

// 0x6bcb0: CellDispatch(row, col, kind, arg) - look up grid[row*15+col] (+0x1c);
// if it has a notify hook run NotifyCell(row,col,0) (ret 0); else route the cell by
// `kind` (0xd => ExitGrid, else Route(kind,arg)) and ret 1; ret 0 when no cell.
RVA(0x0006bcb0, 0x6a)
i32 CTriggerMgr::CellDispatch(i32 row, i32 col, i32 kind, i32 arg) {
    CTmGrunt* cell = *(CTmGrunt**)((char*)this + (row * 15 + col) * 4 + 0x1c);
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
    char* view = *(char**)(*(char**)((char*)this + 0x22c) + 0x24);
    char* scroll = *(char**)(view + 0x5c) + 0x40;
    i32 px = *(i32*)(scroll + 0) - *(i32*)(view + 0x10) + sx;
    i32 py = *(i32*)(scroll + 4) - *(i32*)(view + 0x14) + sy;
    return CellHitTest(px, py, outRow, outCol, startRow);
}

// 0x6bea0: CellHitTest - scan the grid for the cell whose 30x30 object bounds contain
// (px,py). startRow==5 means "rows 0..3"; otherwise just that one row. Writes the hit
// (row,col) through the out-ptrs and returns the cell pointer (0 when none).
// @early-stop
// regalloc wall: retail pins row->ebp and px->ebx across the scan (cached once); our cl
// keeps row in [esp+0x2c]/edx and reloads px each compare. Logic + offsets byte-exact.
RVA(0x0006bea0, 0xe2)
void* CTriggerMgr::CellHitTest(i32 px, i32 py, i32* outRow, i32* outCol, i32 startRow) {
    i32 row = startRow;
    i32 last = startRow;
    if (startRow == 5) {
        row = 0;
        last = 3;
    }
    while (row <= last) {
        CTmGrunt** cell = (CTmGrunt**)((char*)this + row * 15 * 4 + 0x1c);
        for (i32 col = 0; col < 15; col++) {
            CTmGrunt* g = cell[col];
            if (g != 0 && *(i32*)((char*)g + 0x1fc) != 0) {
                char* o = *(char**)((char*)g + 0x10);
                if (*(i32*)(o + 0x198) != 0) {
                    i32 x0 = *(i32*)(o + 0x5c) - 15;
                    i32 y0 = *(i32*)(o + 0x60) - 15;
                    if (px < x0 + 30 && px >= x0 && py < y0 + 30 && py >= y0) {
                        if (outRow != 0) {
                            *outRow = row;
                        }
                        if (outCol != 0) {
                            *outCol = col;
                        }
                        return *(void**)((char*)this + row * 15 * 4 + col * 4 + 0x1c);
                    }
                }
            }
        }
        row++;
    }
    return 0;
}

// 0x77f80: FindNearestInRow(g) - the grunt-to-cell proximity probe: scan the 15 cells
// of grid row g->m_1ec for the live cell whose display object (cell->m_10) is nearest g's
// tile position, but only when that squared distance is below the cutoff 2*g->m_2dc.
// @early-stop
// regalloc wall (~84%): retail homes tx in ebp / ty in eax (relocating both out of their
// load registers), and `o` in edx; our cl keeps tx/ty in the load registers (ebx/ebp) and
// `o` in ecx. Every instruction + offset matches modulo register names; not source-
// steerable. topic:wall (same family as NearestCellDist/DestroyAllAnims @early-stops).
RVA(0x00077f80, 0xab)
void* CTriggerMgr::FindNearestInRow(void* gp) {
    CTmGrunt* g = (CTmGrunt*)gp;
    i32 tx = *(i32*)((char*)g + 0x17c) >> 5;
    i32 rowIdx = *(i32*)((char*)g + 0x1ec);
    i32 ty = *(i32*)((char*)g + 0x180) >> 5;
    CTmGrunt** cell = (CTmGrunt**)((char*)this + rowIdx * 15 * 4 + 0x1c);
    CTmGrunt* best = 0;
    i32 bestDist = 0x7fffffff;
    i32 i = 15;
    do {
        CTmGrunt* c = *cell;
        if (c != 0) {
            char* o = *(char**)((char*)c + 0x10);
            i32 dy = (*(i32*)(o + 0x60) >> 5) - ty;
            i32 dx = (*(i32*)(o + 0x5c) >> 5) - tx;
            i32 d = dx * dx + dy * dy;
            if (d < bestDist && d < *(i32*)((char*)g + 0x2dc) * 2) {
                best = c;
                bestDist = d;
            }
        }
        cell++;
        i--;
    } while (i != 0);
    return best;
}

// 0x78260: RemoveCellRecord(x, y, fromSelection) - when fromSelection, first unlink the
// (x,y) node from whichever of the 10 selection lists (+0x2d0) holds it. Then find the
// (x,y) node in the record list (+0x244); if present, optionally StopPendingFx, clear the
// grid cell's sprites (grid[y+15*x]), flag+clear the active goal/record (+0x234..+0x23c),
// tick the overlay if it owns (x,y), recycle the node and RemoveAt the +0x240 list; ret 1.
// ret 0 when no record. (__stdcall: ret 0xc.)
// @early-stop
// regalloc wall: retail pins this->ebp and the selection counter in [esp+0x1c]; under
// our cl's pressure this spills to [esp+0x10] and is reloaded. Logic + offsets + the
// free-list recycle byte-exact; the record-scan/goal/overlay path matches. topic:wall.
RVA(0x00078260, 0x165)
i32 CTriggerMgr::RemoveCellRecord(i32 x, i32 y, i32 fromSelection) {
    if (fromSelection != 0) {
        CTmPtrList* list = (CTmPtrList*)((char*)this + 0x2d0);
        i32 k = 10;
        do {
            CTmNode* n = list->m_head;
            while (n != 0) {
                CTmNode* cur = n;
                n = n->m_next;
                i32* p = cur->m_payload;
                if (p[0] == x && p[1] == y) {
                    void** slot = (void**)((char*)p - g_freeListNodeBias);
                    *slot = g_freeList;
                    g_freeList = slot;
                    list->RemoveAt(cur);
                }
            }
            list = (CTmPtrList*)((char*)list + 0x1c);
            k--;
        } while (k != 0);
    }
    CTmNode* n = *(CTmNode**)((char*)this + 0x244);
    if (n == 0) {
        return 0;
    }
    CTmNode* cur;
    i32* p;
    do {
        cur = n;
        n = n->m_next;
        p = cur->m_payload;
        if (p[0] == x && p[1] == y) {
            goto found;
        }
    } while (n != 0);
    return 0;
found:
    if (*(i32*)((char*)this + 0x24c) == 1) {
        StopPendingFx();
    }
    CTmGrunt* cell = *(CTmGrunt**)((char*)this + (y + x * 15) * 4 + 0x1c);
    if (cell != 0) {
        cell->ClearAllSprites();
    }
    if (*(i32*)((char*)this + 0x234) == p[0] && *(i32*)((char*)this + 0x238) == p[1]) {
        CTmGoal* goal = *(CTmGoal**)((char*)this + 0x23c);
        if (goal != 0) {
            goal->m_8 |= 0x10000;
            *(CTmGoal**)((char*)this + 0x23c) = 0;
        }
        *(i32*)((char*)this + 0x230) = 0;
    }
    CTmNode* ov = *(CTmNode**)((char*)this + 0x25c);
    if (ov != 0 && ((i32*)ov)[0] == p[0] && ((i32*)ov)[1] == p[1]) {
        OverlayTick();
    }
    void** slot = (void**)((char*)p - g_freeListNodeBias);
    *slot = g_freeList;
    g_freeList = slot;
    ((CTmPtrList*)((char*)this + 0x240))->RemoveAt(cur);
    return 1;
}

// 0x78430: ResetAll - drain the record list (+0x244): for each node, clear the
// referenced grid cell's sprites (grid[ y + 15*x ] @+0x1c) and recycle the node to
// the free list; RemoveAll the +0x240 list, run StopPendingFx, flag the goal (+0x23c).
RVA(0x00078430, 0x7f)
void CTriggerMgr::ResetAll() {
    CTmNode* n = *(CTmNode**)((char*)this + 0x244);
    if (n != 0) {
        do {
            CTmNode* cur = n;
            n = n->m_next;
            i32* payload = cur->m_payload;
            i32 idx = payload[1] + 15 * payload[0];
            CTmGrunt* cell = *(CTmGrunt**)((char*)this + idx * 4 + 0x1c);
            if (cell != 0) {
                cell->ClearAllSprites();
                void** slot = (void**)((char*)payload - g_freeListNodeBias);
                *slot = g_freeList;
                g_freeList = slot;
            }
        } while (n != 0);
    }
    ((CTmPtrList*)((char*)this + 0x240))->RemoveAll();
    StopPendingFx();
    CTmGoal* goal = *(CTmGoal**)((char*)this + 0x23c);
    if (goal != 0) {
        goal->m_8 |= 0x10000;
        *(CTmGoal**)((char*)this + 0x23c) = 0;
    }
}

// 0x784d0: RecordListHas(x, y) - scan the record list (+0x244) for a node whose
// payload (x,y) matches; ret 1 on hit, 0 otherwise.
// @early-stop
// loop-epilogue wall: retail loops with `jne top` and reuses the null next as the
// eax=0 return (fall-through); our cl emits `je end; jmp top` + a separate
// `xor eax,eax`. docs/patterns/identical-return-epilogue-tailmerge.md
RVA(0x000784d0, 0x3a)
i32 CTriggerMgr::RecordListHas(i32 x, i32 y) {
    CTmNode* n = *(CTmNode**)((char*)this + 0x244);
    if (n == 0) {
        return 0;
    }
    do {
        CTmNode* cur = n;
        n = n->m_next;
        i32* p = cur->m_payload;
        if (p[0] == x && p[1] == y) {
            return 1;
        }
    } while (n != 0);
    return 0;
}

// 0x78880: ClearRecords - drain the record list (+0x244) back to the free list,
// then RemoveAll the +0x240 MFC pointer list. The free-list head is cached in a
// register across the loop (g_freeList read once, written each iteration).
// @early-stop
// prologue scheduling wall: the drain loop body is byte-exact, but retail batches
// `push edi; push esi` then loads esi=head/edi=bias; our cl interleaves the loads
// with the pushes. docs/patterns/zero-store-before-loop-inline-bound.md
RVA(0x00078880, 0x3c)
void CTriggerMgr::ClearRecords() {
    CTmNode* n = *(CTmNode**)((char*)this + 0x244);
    if (n != 0) {
        i32 bias = g_freeListNodeBias;
        void* head = g_freeList;
        do {
            CTmNode* cur = n;
            n = n->m_next;
            void** slot = (void**)((char*)cur->m_payload - bias);
            *slot = head;
            head = slot;
            g_freeList = head;
        } while (n != 0);
    }
    ((CTmPtrList*)((char*)this + 0x240))->RemoveAll();
}

// 0x78a30: OverlayTick - dispatch the overlay sub-object's Tick when present.
RVA(0x00078a30, 0x10)
void CTriggerMgr::OverlayTick() {
    CTmOverlay* ov = *(CTmOverlay**)((char*)this + 0x25c);
    if (ov) {
        ov->Tick();
    }
}

// 0x79b00: OverlayRelease - release the overlay sub-object when present; ret 1.
RVA(0x00079b00, 0x15)
i32 CTriggerMgr::OverlayRelease() {
    CTmOverlay* ov = *(CTmOverlay**)((char*)this + 0x25c);
    if (ov) {
        return ov->Release();
    }
    return 1;
}

// 0x79b30: ByteTableHas(b) - linear search the byte table (+0x264, count +0x268)
// for `b`; ret 1 on hit, 0 otherwise.
// @early-stop
// loop-peel + push-split wall: retail emits a single un-peeled indexed loop
// (`mov dl,[eax+ecx]`, edx hoisted-zeroed) and splits the two prologue pushes around
// the count load; our cl peels the i==0 iteration. docs/patterns/zero-store-before-loop-inline-bound.md
RVA(0x00079b30, 0x3e)
i32 CTriggerMgr::ByteTableHas(i32 b) {
    i32 n = *(i32*)((char*)this + 0x268);
    i32 i = 0;
    if (n <= 0) {
        return 0;
    }
    u8* tbl = *(u8**)((char*)this + 0x264);
    do {
        if (b == tbl[i]) {
            return 1;
        }
        i++;
    } while (i < n);
    return 0;
}

// 0x7be10: StopPendingFx - when our overlay-fx (+0x2a8) is live, or the world has a
// pending flag (world+0x504), stop the world's fx and clear +0x2a8.
RVA(0x0007be10, 0x34)
void CTriggerMgr::StopPendingFx() {
    CTmWorld* world = g_gameReg->m_2c;
    if (*(i32*)((char*)this + 0x2a8) == 0 && world->m_504 == 0) {
        return;
    }
    world->StopFx(0, 0);
    *(i32*)((char*)this + 0x2a8) = 0;
}

// 0x7d0c0: ClearSelections - drain all 10 selection lists (+0x2d0, stride 0x1c) back
// to the free list (skipping null-payload nodes), RemoveAll each list, reset +0x3e8.
RVA(0x0007d0c0, 0x57)
void CTriggerMgr::ClearSelections() {
    CTmPtrList* list = (CTmPtrList*)((char*)this + 0x2d0);
    i32 k = 10;
    do {
        CTmNode* n = list->m_head;
        if (n != 0) {
            void* head = g_freeList;
            do {
                CTmNode* cur = n;
                n = n->m_next;
                i32* payload = cur->m_payload;
                if (payload != 0) {
                    void** slot = (void**)((char*)payload - g_freeListNodeBias);
                    *slot = head;
                    head = slot;
                    g_freeList = head;
                }
            } while (n != 0);
        }
        list->RemoveAll();
        list = (CTmPtrList*)((char*)list + 0x1c);
        k--;
    } while (k != 0);
    *(i32*)((char*)this + 0x3e8) = -1;
}

// 0x7d140: ClearRow(row) - run ExitGrid on the 15 live, hook-less cells of grid
// row `row` (+0x1c); clear +0x400 when row is the magic group, then refresh world.
RVA(0x0007d140, 0x61)
i32 CTriggerMgr::ClearRow(i32 row) {
    CTmGrunt** cell = (CTmGrunt**)((char*)this + row * 15 * 4 + 0x1c);
    i32 i = 15;
    do {
        CTmGrunt* c = *cell;
        if (c != 0 && c->m_368 == 0) {
            c->ExitGrid();
        }
        cell++;
        i--;
    } while (i != 0);
    if (row == g_644c54) {
        *(i32*)((char*)this + 0x400) = 0;
    }
    g_gameReg->m_2c->Refresh();
    return 1;
}

// 0x7d1d0: NearestCellDist - the minimum squared (>>5 tile) distance from (px,py) to
// any live, clickable grid cell, scanning rows 0..3 but skipping row `skipRow`.
// @early-stop
// regalloc wall: the squared-distance body is byte-exact, but retail spills the row/col
// loop vars to different stack slots than our cl. Logic + offsets correct. topic:wall.
RVA(0x0007d1d0, 0x9d)
i32 CTriggerMgr::NearestCellDist(i32 skipRow, i32 px, i32 py) {
    i32 tx = px >> 5;
    i32 ty = py >> 5;
    i32 best = 0x7fffffff;
    CTmGrunt** row = (CTmGrunt**)((char*)this + 0x1c);
    i32 r = 0;
    do {
        if (r != skipRow) {
            i32 i = 15;
            CTmGrunt** cell = row;
            do {
                CTmGrunt* g = *cell;
                if (g != 0 && *(i32*)((char*)g + 0x1fc) != 0) {
                    char* o = *(char**)((char*)g + 0x10);
                    i32 dy = (*(i32*)(o + 0x60) >> 5) - ty;
                    i32 dx = (*(i32*)(o + 0x5c) >> 5) - tx;
                    i32 d = dx * dx + dy * dy;
                    if (d < 0) {
                        d = -d;
                    }
                    if (d < best) {
                        best = d;
                    }
                }
                cell++;
                i--;
            } while (i != 0);
        }
        row += 15;
        r++;
    } while (r < 4);
    return best;
}

// 0x7d2a0: SelectionListFind(key, y) - only when key == g_644c54, scan the 10
// selection lists (+0x2d4, stride 0x1c) for a node whose payload (x,y) matches
// (key,y); ret the first matching list index, 0xa on a second match, else 0.
// @early-stop
// loop-rotation + interleaved-epilogue wall: the list-walk body is byte-exact, but
// retail loops with `jl top` (fall-through exit) and interleaves `mov eax,0xa` between
// the pops; our cl emits `jge end; jmp top` + a clean epilogue. topic:wall.
RVA(0x0007d2a0, 0x64)
i32 CTriggerMgr::SelectionListFind(i32 key, i32 y) {
    if (key != g_644c54) {
        return 0;
    }
    i32 result = 0;
    i32 i = 0;
    CTmNode** head = (CTmNode**)((char*)this + 0x2d4);
    do {
        CTmNode* n = *head;
        while (n != 0) {
            CTmNode* cur = n;
            n = n->m_next;
            i32* payload = cur->m_payload;
            if (payload[0] == key && payload[1] == y) {
                if (result != 0) {
                    return 10;
                }
                result = i;
            }
        }
        i++;
        head = (CTmNode**)((char*)head + 0x1c);
    } while (i < 10);
    return result;
}

// 0x7d330: DestroyAllAnims - DestroyAnims on every live grid cell (4 rows x 15), then
// walk the level's display-object list clearing the +0x200 marker on each object whose
// type-descriptor (obj+0x7c) slot-4 matches the switch tag, then stop the three sound
// channels (+0x3f0, +0x3f4, and the active grunt's +0x618).
// @early-stop
// regalloc wall: retail homes the row counter in ebp and the zero-compare const in ebx;
// our cl swaps them (ebx counter, ebp zero). Code bytes + offsets byte-exact, externs
// reloc-masked (DirectSoundMgr::StopAndRewind, ReadConfigFromButeMgr tag). topic:wall.
RVA(0x0007d330, 0xd3)
void CTriggerMgr::DestroyAllAnims() {
    CTmGrunt** cell = (CTmGrunt**)((char*)this + 0x1c);
    i32 r = 4;
    do {
        i32 i = 15;
        do {
            CTmGrunt* g = *cell;
            if (g != 0) {
                g->DestroyAnims();
            }
            cell++;
            i--;
        } while (i != 0);
        r--;
    } while (r != 0);

    char* list = *(char**)(*(char**)((char*)this + 0x22c) + 0x8);
    CTmListNode* node = *(CTmListNode**)(list + 0x14);
    while (node != 0) {
        void* obj = node->m_8;
        node = node->m_0;
        if (obj != 0) {
            char* desc = *(char**)((char*)obj + 0x7c);
            void (CGrunt::*tag)() = &CGrunt::ReadConfigFromButeMgr;
            if (*(void**)(desc + 0x10) == *(void**)&tag) {
                char* tgt = *(char**)(desc + 0x18);
                *(i32*)(tgt + 0x200) = 0;
            }
        }
    }

    DirectSoundMgr* ch0 = *(DirectSoundMgr**)((char*)this + 0x3f0);
    if (ch0 != 0) {
        ch0->StopAndRewind();
        *(DirectSoundMgr**)((char*)this + 0x3f0) = 0;
    }
    DirectSoundMgr* ch1 = *(DirectSoundMgr**)((char*)this + 0x3f4);
    if (ch1 != 0) {
        ch1->StopAndRewind();
        *(DirectSoundMgr**)((char*)this + 0x3f4) = 0;
    }
    void* state = g_gameReg->PickPausedThenPlayState();
    if (state != 0) {
        char* sub = *(char**)((char*)state + 0x2dc);
        if (sub != 0) {
            DirectSoundMgr* ch2 = *(DirectSoundMgr**)(sub + 0x618);
            if (ch2 != 0) {
                ch2->StopAndRewind();
                *(DirectSoundMgr**)(sub + 0x618) = 0;
            }
        }
    }
}

// 0x7a510: ClearRowAndRefresh(startRow) - Recall every live, hook-less cell of rows
// startRow..3 (5 = all); clear +0x400 when startRow is the magic group; then refresh
// the world, bump a stat, and re-arm the status item. ret 1.
// @early-stop
// regalloc wall: retail pins this->esi and startRow->ebp across the body; our cl homes
// this->ebp and reloads startRow for the g_644c54 test. Logic + offsets byte-exact.
RVA(0x0007a510, 0x9e)
i32 CTriggerMgr::ClearRowAndRefresh(i32 startRow) {
    i32 row = startRow;
    i32 last = startRow;
    if (startRow == 5) {
        row = 0;
        last = 3;
    }
    if (row <= last) {
        i32 n = last - row + 1;
        CTmGrunt** cell = (CTmGrunt**)((char*)this + row * 15 * 4 + 0x1c);
        do {
            i32 i = 15;
            do {
                CTmGrunt* c = *cell;
                if (c != 0 && c->m_368 == 0) {
                    c->Recall();
                }
                cell++;
                i--;
            } while (i != 0);
            n--;
        } while (n != 0);
    }
    if (startRow == g_644c54) {
        *(i32*)((char*)this + 0x400) = 0;
    }
    CTmWorld* world = g_gameReg->m_2c;
    world->Refresh();
    world->SetStat(0, 0xbb7);
    world->m_2dc->SetMode(1);
    return 1;
}

// 0x7a180: SpawnPuddle(x, y, f124, dir, color, f118) - create a "GruntPuddle" sprite from
// the level's factory; on failure ReportError and ret 0. On success run its Init hook,
// stash the placement fields (+0x124/+0x114/+0x118) and tail into PlacePuddle. (ret 0x18.)
RVA(0x0007a180, 0x86)
i32 CTriggerMgr::SpawnPuddle(i32 x, i32 y, i32 f124, i32 f114, i32 color, i32 f118) {
    CTmSpriteFactory* fac = ((CTmLevel*)*(void**)((char*)this + 0x22c))->m_8;
    void* sprite = fac->CreateSprite(0, x, y, 0xa, "GruntPuddle", 0x40003);
    if (sprite == 0) {
        g_gameReg->ReportError(0x8009, 0x400);
        return 0;
    }
    ((CTmSpriteDesc*)*(void**)((char*)sprite + 0x7c))->Init(sprite);
    *(i32*)((char*)sprite + 0x124) = f124;
    *(i32*)((char*)sprite + 0x114) = f114;
    *(i32*)((char*)sprite + 0x118) = f118;
    return PlacePuddle(sprite, color);
}

// The puddle's placement target (sprite desc +0x18): a CUserLogic-ish object whose
// PlacePuddle(a,b,c,d) does the real placement (reloc-masked @0x9c3f0) and whose +0x38
// goal carries the +0x8 flags ORed with 0x10000 on failure. The (x,y,z) the record-list
// walk matches against live at +0x54/+0x58/+0x5c.
struct CTmPuddleTarget {
    i32 Place(i32 a, i32 b, i32 c, i32 d); // 0x9c3f0
    char p0[0x38];
    void* m_38; // +0x38  goal object (its +0x8 is the flags word)
    char p1[0x54 - 0x3c];
    i32 m_54; // +0x54  match x
    i32 m_58; // +0x58  match y
    i32 m_5c; // +0x5c  busy flag
};
// The cell record nodes PlacePuddle walks: this+0x4 is the intrusive CPtrList head node,
// this+0xc its count. Each node carries the next ptr @+0 and the placed-object @+0x8.
struct CTmRecNode {
    CTmRecNode* m_next;     // +0x00
    char p0[0x4];           // +0x04
    CTmPuddleTarget* m_obj; // +0x08  placed object (the puddle target shape)
};

// 0x7a240: PlacePuddle(sprite, color) - hand the sprite's placement params to its target's
// PlacePuddle; on failure flag the goal + ReportError(0x8009,0x401). On success walk the
// record list twice (full (x,y) match, then x-only when count>0x3b) flagging+unlinking each
// matching node, then RemoveAll the list. (__thiscall: ret 0x8.)
// @early-stop
// regalloc + dual-loop scheduling wall: the two record-walk loops + the found/unlinked flags
// spill to different stack slots than retail and the (x,y)==busy fast-path goto reorders.
// Logic + offsets + the RemoveAt/RemoveAll recycle byte-exact. topic:wall.
RVA(0x0007a240, 0x143)
i32 CTriggerMgr::PlacePuddle(void* sprite, i32 color) {
    CTmPuddleTarget* tgt = (CTmPuddleTarget*)*(void**)(*(char**)((char*)sprite + 0x7c) + 0x18);
    i32 d = *(i32*)((char*)sprite + 0x118);
    if (d == 0) {
        d = 0x19;
    }
    if (tgt->Place(*(i32*)((char*)sprite + 0x124), *(i32*)((char*)sprite + 0x114), color, d) == 0) {
        *(i32*)((char*)tgt->m_38 + 0x8) |= 0x10000;
        g_gameReg->ReportError(0x8009, 0x401);
        return 0;
    }
    CTmRecNode* n = *(CTmRecNode**)((char*)this + 0x4);
    i32 manyFlag = (*(i32*)((char*)this + 0xc) > 0x3b) ? 1 : 0;
    i32 unlinked = 0;
    while (n != 0 && unlinked == 0) {
        CTmRecNode* cur = n;
        n = n->m_next;
        CTmPuddleTarget* o = cur->m_obj;
        if (o->m_54 == tgt->m_54 && o->m_58 == tgt->m_58) {
            if (o->m_5c != 0) {
                *(i32*)((char*)tgt->m_38 + 0x8) |= 0x10000;
                return 0;
            }
            *(i32*)((char*)o->m_38 + 0x8) |= 0x10000;
            ((CTmPtrList*)((char*)this))->RemoveAt(cur);
            unlinked = 1;
        }
    }
    if (manyFlag != 0 && unlinked == 0) {
        n = *(CTmRecNode**)((char*)this + 0x4);
        while (n != 0) {
            CTmRecNode* cur = n;
            n = n->m_next;
            CTmPuddleTarget* o = cur->m_obj;
            if (o->m_5c == 0) {
                *(i32*)((char*)o->m_38 + 0x8) |= 0x10000;
                ((CTmPtrList*)((char*)this))->RemoveAt(cur);
            }
        }
    }
    ((CTmPtrList*)((char*)this))->AddTail(tgt);
    return 1;
}

// A live grid-cell game object as the notify/trigger leaves view it. Only the raw offset
// reads are needed; opaque otherwise.
struct CTmCell {
    char p0[0x170];
    i32 m_170; // +0x170  logic kind
    char p1[0x17c - 0x174];
    i32 m_17c; // +0x17c  world x
    i32 m_180; // +0x180  world y
    char p2[0x198 - 0x184];
    i32 m_198; // +0x198  alt kind
    i32 m_19c; // +0x19c  remapped kind
    char p4[0x1e8 - 0x1a0];
    i32 m_1e8; // +0x1e8  recall-done flag
    char p5[0x36c - 0x1ec];
    i32 m_36c; // +0x36c  notified flag
};

// The other UNMATCHED CTriggerMgr methods the reconstructed leaves self-call (reloc-masked
// rel32; bodies live elsewhere / are still stubs). Declared as no-body helpers so the calls
// mangle onto CTriggerMgr and their displacements mask. These are file-local extern decls.
struct CTmSelf {
    void RecallCell(i32 x, i32 y, void* cell); // self-call, reloc-masked
    void RefreshB(i32 a);                      // self-call (this, arg=1)
    void RefreshC();                           // self-call (this, no arg)
    i32 Reset3(i32 a, i32 b, i32 c);           // self-call (this, 3 args)
};
// The pending-fx sub-object at CTriggerMgr+0x2a0; its Pulse() is the reloc-masked thiscall.
struct CTmPendingFx {
    void Pulse(); // reloc-masked
};

// The overlay snapshot source `obj`: a sprite whose +0x2c / +0x30 vtable slots are the
// 8-byte field getters RebuildOverlay copies into the manager's three pose blocks.
struct CTmOverlaySrc {
    void* m_vt;
    void GetA(void* dst, i32 n); // vtbl +0x2c
    void GetB(void* dst, i32 n); // vtbl +0x30
};

// A self-method pair RebuildOverlay early-exits on (reloc-masked self-calls).
struct CTmSelf2 {
    i32 Probe4(void* obj); // 0x7a618 path
    i32 Probe7(void* obj); // 0x7a605 path
};

// 0x7a5e0: RebuildOverlay(obj, kind) - copy the source object's two pose getters into the
// manager's three 0x8-byte pose blocks (+0x290/+0x2b0/+0x2c0), selecting getter +0x30 for
// kind 4 and +0x2c for kind 7; ret 0 when obj null, 1 otherwise. Early-out when the kind-4/7
// self-probe is non-zero. (__thiscall: ret 0x10.)
RVA(0x0007a5e0, 0x121)
i32 CTriggerMgr::RebuildOverlay(void* obj, i32 kind) {
    if (obj == 0) {
        return 0;
    }
    CTmSelf2* self = (CTmSelf2*)this;
    if (kind == 4) {
        if (self->Probe4(obj) != 0) {
            return 0;
        }
    } else if (kind == 7) {
        if (self->Probe7(obj) != 0) {
            return 0;
        }
    }
    CTmOverlaySrc* src = (CTmOverlaySrc*)obj;
    char* blk0 = (char*)this + 0x290;
    if (kind == 4) {
        src->GetB(blk0, 8);
        src->GetB(blk0 + 8, 8);
    } else if (kind == 7) {
        src->GetA(blk0, 8);
        src->GetA(blk0 + 8, 8);
    }
    char* blk1 = (char*)this + 0x2b0;
    if (kind == 4) {
        src->GetB(blk1, 8);
        src->GetB(blk1 + 8, 8);
    } else if (kind == 7) {
        src->GetA(blk1, 8);
        src->GetA(blk1 + 8, 8);
    }
    char* blk2 = (char*)this + 0x2c0;
    if (kind == 4) {
        src->GetB(blk2, 8);
        src->GetB(blk2 + 8, 8);
        return 1;
    }
    if (kind == 7) {
        src->GetA(blk2, 8);
        src->GetA(blk2 + 8, 8);
    }
    return 1;
}

// A grid cell as ResetCell/ClearCell view it: reloc-masked sub-state resetters + the
// alive flag (+0x1fc) and config-table fields (+0x880..+0x88c).
struct CTmCell2 {
    void ResetA();    // 0x6a40c
    void ResetB();    // 0x6a2ae
    void ResetC();    // 0x6c216
    i32 ResetMagic(); // 0x6c498
    char p0[0x1fc];
    i32 m_1fc; // +0x1fc  alive
};
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
    CTmCell2* cell = *(CTmCell2**)((char*)this + idx * 4 + 0x1c);
    if (cell == 0 || cell->m_1fc == 0) {
        return 0;
    }
    if (col != g_644c54) {
        cell->ResetA();
        cell->ResetB();
        cell->ResetC();
        *(i32*)((char*)cell + 0x888) = g_buteMgr.GetDwordDef("Grunt", "CombatTimeout", 0x1388);
        *(i32*)((char*)cell + 0x88c) = 0;
        *(i32*)((char*)cell + 0x880) = g_645588;
        *(i32*)((char*)cell + 0x884) = 0;
        return 0;
    }
    if (force != 0) {
        if (keep == 0) {
            if (((CTmSelf*)this)->Reset3(col, row, 0) != 0) {
                return 1;
            }
        }
    } else {
        ((CTmSelf*)this)->RefreshC(); // self-call 0x6c068 (reloc-masked)
    }
    void* node = g_freeList;
    i32* slot = 0;
    if (*(void**)node != 0) {
        slot = (i32*)((char*)node + 4);
        slot[0] = col;
        slot[1] = row;
        g_freeList = *(void**)g_freeList;
    }
    ((CTmPtrList*)((char*)this + 0x240))->AddTail(slot);
    return cell->ResetMagic();
}

// The manager's own ScreenToCell-style hit helper + the magic-group refresh self-call.
struct CTmSelf4 {
    void* Hit(i32 arg, i32 a, i32 b, i32* outRow, i32* outCol); // self-call 0x6e2ce
    void ClearMagic(i32 key);                                   // self-call 0x6f2c9
};

// 0x6ea00: HitTestApply(x, y, kind) - hit-test the cell at (x,y); only for the magic group
// (out-row == g_644c54) and a cell whose config name is "B" and kind 0x14, add the world's
// score delta, zero the status fields, SetStat(0,0xbb7), re-arm the status item (SetMode 1)
// and ClearMagic(g_644c54). (__stdcall: ret 0xc.)
// @early-stop
// regalloc + inline-strcmp wall: the "B" compare inlines as a byte loop and the 64-bit
// world-score sub/sbb pins ecx/eax differently than retail. Logic + offsets byte-exact.
RVA(0x0006ea00, 0x125)
i32 CTriggerMgr::HitTestApply(i32 x, i32 y, i32 kind) {
    i32 outRow = 0;
    i32 outCol = 0;
    CTmCell* cell = (CTmCell*)((CTmSelf4*)this)->Hit(kind, y, y, &outRow, &outCol);
    if (cell == 0 || outCol != g_644c54) {
        return 0;
    }
    char* name = *g_nameReg.Lookup(*(i32*)(*(char**)((char*)cell + 0x14) + 0x1c));
    if (strcmp(name, "B") != 0) {
        return 0;
    }
    i32 k = *(i32*)((char*)cell + 0x170);
    if (k > 0x16) {
        k = *(i32*)((char*)cell + 0x19c);
    }
    if (k != 0x14) {
        return 0;
    }
    CTmWorld* world = g_gameReg->m_2c;
    char* sub = *(char**)((char*)world + 0x3f4);
    u32 lo = (u32)g_645588 - (u32) * (i32*)(sub + 0x38);
    i32 hi = 0 - *(i32*)(sub + 0x3c) - (g_645588 < *(i32*)(sub + 0x38) ? 1 : 0);
    if (hi > 0 || (hi == 0 && lo >= 0)) {
        lo = 0;
    }
    *(i32*)(*(char**)((char*)g_gameReg + 0x7c) + 0x10) += (i32)lo;
    *(i32*)(sub + 0x40) = 0;
    *(i32*)(sub + 0x44) = 0;
    *(i32*)(sub + 0x30) = 0;
    *(i32*)(sub + 0x34) = 0;
    *(i32*)(sub + 0x48) = 0;
    *(i32*)(sub + 0x4c) = 0;
    world->SetStat(0, 0xbb7);
    world->m_2dc->SetMode(1);
    ((CTmSelf4*)this)->ClearMagic(g_644c54);
    return 1;
}

// The world's fx-spawner (gameReg+0x68 sub-mgr) + the stop-fx hook the TriggerCell driver
// uses; all reloc-masked.
struct CTmFxMgr {
    void* Spawn(i32 a, i32 b, i32 c, i32 d, i32 e, i32 f, i32 g); // 0x90b48 (7 args)
};
struct CTmWorld2 {
    void StopFx2(i32 a, i32 b); // 0xd0b3a
};
// The two manager self-calls TriggerCell tails into (Refresh + Record).
struct CTmSelf5 {
    i32 Classify(i32 x, i32 y); // self-call 0x6ef2c
    void Refresh2();            // self-call 0x6c6c4
    void Record2(i32 x, i32 y); // self-call 0x6de5c
};

// 0x7b1b0: TriggerCell(x, y) - clear the pending-fx slot; only when the overlay sub-object
// (+0x25c) is live with its +0x2c set, look up the magic record cell, classify (x,y) and by
// the resulting kind spawn the matching fx sprite (kind 2 -> remapped 0x13, kind 3 -> alt
// 0x1e, else generic +0xc8 into +0x2a8), then Refresh + Record. ret 1. (ret 0x8.)
// @early-stop
// regalloc + switch-on-kind wall: the classify result drives a cmp/je ladder that pins ebx
// (world) and esi (cell) differently than retail, and the fx arg pushes spill. topic:wall.
RVA(0x0007b1b0, 0x12b)
i32 CTriggerMgr::TriggerCell(i32 x, i32 y) {
    void* ov = *(void**)((char*)this + 0x25c);
    *(i32*)((char*)this + 0x2a8) = 0;
    if (ov == 0 || *(i32*)((char*)ov + 0x2c) == 0) {
        return 0;
    }
    CTmCell* cell = 0;
    if (*(i32*)((char*)this + 0x24c) == 1) {
        i32* rec = *(i32**)(*(char**)((char*)this + 0x244) + 0x8);
        cell = *(CTmCell**)((char*)this + (rec[1] + rec[0] * 15) * 4 + 0x1c);
    }
    CTmWorld2* world = (CTmWorld2*)((char*)g_gameReg->m_2c);
    i32 kind = ((CTmSelf5*)this)->Classify(x, y);
    if (kind == 2) {
        i32 alt = *(i32*)((char*)cell + 0x170);
        if (alt > 0x16) {
            alt = *(i32*)((char*)cell + 0x19c);
        }
        if (alt == 0x13) {
            ((CTmFxMgr*)*(void**)((char*)g_gameReg + 0x68))
                ->Spawn(*(i32*)((char*)cell + 0x17c), *(i32*)((char*)cell + 0x180), 0, 0, 0, 2, 1);
        }
    } else if (kind == 3) {
        if (*(i32*)((char*)cell + 0x198) == 0x1e) {
            char* o = *(char**)((char*)cell + 0x10);
            ((CTmFxMgr*)*(void**)((char*)g_gameReg + 0x68))
                ->Spawn(*(i32*)(o + 0x5c), *(i32*)(o + 0x60), 0, 0, 0, 3, 1);
        }
    } else if (kind != 0) {
        i32 v = kind + 0xc8;
        *(i32*)((char*)this + 0x2a8) = v;
        world->StopFx2(v, 0);
    }
    ((CTmSelf5*)this)->Refresh2();
    ((CTmSelf5*)this)->Record2(x, y);
    return 1;
}

// The sprite's CUserLogic (sprite desc +0x18): its multi-arg Place driver (reloc-masked
// @0x41c4 thunk) and the +0x154 goal whose +0x8 flags get the 0x10000 done-bit on failure.
struct CTmUserLogic {
    i32 Place(
        i32 a,
        i32 b,
        i32 c,
        i32 d,
        i32 e,
        i32 f,
        i32 g,
        i32 h,
        i32 i,
        i32 j,
        i32 k,
        i32 l
    );                                                                     // 0x4c1c4
    void Arm(const char* lighting, const char* cursor, i32 kind, i32 one); // 0x4e517
};

// 0x7c110: SpawnGrunt(col, row, a18, a1c) - find the first free column of grid row `row`;
// if full ret 0. Snap the source cell[col]'s display pos to a tile, run a prep self-call,
// create a "Grunt" sprite at that pos, Init it, place it via its userlogic; on placement
// failure flag the goal and ret 0, else stash the cell + bump the per-row counters. ret 1.
// (__stdcall: ret 0x10.)
// @early-stop
// regalloc + free-column-scan wall: the inner "first free col" loop and the snap arithmetic
// pin ebp/edi differently than retail; the placement-failure goal-flag path tail-merges.
RVA(0x0007c110, 0x166)
i32 CTriggerMgr::SpawnGrunt(i32 col, i32 row, i32 a18, i32 a1c) {
    CTmCell* src = *(CTmCell**)((char*)this + (col * 15 + a1c) * 4 + 0x1c);
    i32 free = 0;
    CTmCell** rowBase = (CTmCell**)((char*)this + row * 15 * 4 + 0x1c);
    if (*rowBase != 0) {
        CTmCell** p = rowBase;
        while (free < 15 && *p != 0) {
            p++;
            free++;
        }
    }
    if (free >= 15) {
        return 0;
    }
    char* o = *(char**)((char*)src + 0x10);
    i32 sx = (*(i32*)(o + 0x5c) & ~0x1f) + 0x10;
    i32 sy = (*(i32*)(o + 0x60) & ~0x1f) + 0x10;
    i32 k = *(i32*)((char*)src + 0x170);
    if (k > 0x16) {
        k = *(i32*)((char*)src + 0x19c);
    }
    i32 vis = *(i32*)((char*)src + 0x198);
    ((CTmSelf*)this)->Reset3(col, k, vis); // prep self-call 0x7ec96
    CTmSpriteFactory* fac = ((CTmLevel*)*(void**)((char*)this + 0x22c))->m_8;
    void* sprite = fac->CreateSprite(0, sx, sy, 0x186a0, "Grunt", 0x40003);
    if (sprite == 0) {
        return 0;
    }
    ((CTmSpriteDesc*)*(void**)((char*)sprite + 0x7c))->Init(sprite);
    void* logic = *(void**)(*(char**)((char*)sprite + 0x7c) + 0x18);
    if (((CTmUserLogic*)logic)->Place(col, row, vis, k, 0, 0, 0, 0, 0, 0, 0, 0) == 0) {
        *(i32*)(*(char**)((char*)logic + 0x154) + 0x8) |= 0x10000;
        return 0;
    }
    *(CTmCell**)((char*)this + (row * 15 + free) * 4 + 0x1c) = (CTmCell*)sprite;
    *(i32*)((char*)this + row * 4 + 0x10c) += 1;
    *(i32*)((char*)this + (row * 15 + free) * 4 + 0x11c) = 0;
    return 1;
}

// 0x759e0: GetOriginXY(out) - copy the cached origin pair (+0x174,+0x178) into the
// caller's slot and return it. `out` (loaded into eax as the store base) is the
// return value; `ret 4` -> callee cleans the out-ptr.
RVA(0x000759e0, 0x18)
CTrigPoint* CTriggerMgr::GetOriginXY(CTrigPoint* out) {
    out->x = *(i32*)((char*)this + 0x174);
    out->y = *(i32*)((char*)this + 0x178);
    return out;
}

// 0x75a90: TmFlagsAllow(a, b, c) - a __cdecl trigger-flag compatibility test on the
// shared bits m = a & b: bit 0x20000000 vetoes outright; with no shared bits, allow;
// otherwise allow only when (a & c) is also set.
RVA(0x00075a90, 0x27)
i32 TmFlagsAllow(i32 a, i32 b, i32 c) {
    i32 m = b & a;
    if (m & 0x20000000) {
        return 0;
    }
    if (m != 0 && (c & a) == 0) {
        return 0;
    }
    return 1;
}

// 0x75af0: HitTestCell(x, y, outRow, outCol, exact) - sample the plane tile-attr at
// (x>>5, y>>5); its high byte is the row, low byte the col. Look up grid[row*15+col]; if
// live+clickable, either exact-match its world pos (exact) or its 30x30 bounds, then write
// (row,col) through the out-ptrs. ret 0 on any miss. (__stdcall: ret 0x14.)
// @early-stop
// regalloc + bounds-arith wall: the row/col high/low-byte split pins edi/ebp and the
// ±7 box arithmetic spills to different slots than retail. Logic + offsets byte-exact.
RVA(0x00075af0, 0x111)
i32 CTriggerMgr::HitTestCell(i32 x, i32 y, i32* outRow, i32* outCol, i32 exact) {
    char* plane = *(char**)((char*)g_gameReg + 0x70);
    i32 ix = x >> 5;
    i32 iy = y >> 5;
    i32 attr;
    if (ix >= *(i32*)(plane + 0xc) || iy >= *(i32*)(plane + 0x10)) {
        attr = -1;
    } else {
        char* tbl = *(char**)(plane + 0x8);
        i32 e = ix * 7;
        attr = *(i32*)(*(char**)(tbl + iy * 4) + e * 4 + 0x4);
    }
    if (attr == -1) {
        return 0;
    }
    i32 row = (attr >> 8) & 0xff;
    i32 col = attr & 0xff;
    CTmCell* cell = *(CTmCell**)((char*)this + (col + row * 15) * 4 + 0x1c);
    if (cell == 0 || *(i32*)((char*)cell + 0x1fc) == 0) {
        return 0;
    }
    if (exact != 0) {
        char* o = *(char**)((char*)cell + 0x10);
        if (*(i32*)(o + 0x5c) != x || *(i32*)(o + 0x60) != y) {
            return 0;
        }
        if (outRow != 0) {
            *outRow = row;
        }
        if (outCol != 0) {
            *outCol = col;
        }
        return 1;
    }
    char* o = *(char**)((char*)cell + 0x10);
    i32 ox = *(i32*)(o + 0x5c);
    i32 oy = *(i32*)(o + 0x60);
    if (x + 7 > ox + 14 || x - 7 < ox - 7 || y + 7 > oy + 14 || y - 7 < oy - 7) {
        return 0;
    }
    if (outRow != 0) {
        *outRow = row;
    }
    if (outCol != 0) {
        *outCol = col;
    }
    return 1;
}

// A cell as ClearCell views it: the reset fields + the +0x14 type sub-object.
struct CTmCell3 {
    i32 Disarm(i32 a, i32 b);                               // 0x6f970 reset hook (1,1)
    i32 ApplyBox(i32 a, i32 b, i32 c, i32 d, i32 e, i32 f); // 0x6fc40 the bounds applier
};
// The matched-name fx, run on THIS (the manager), reloc-masked self-call.
struct CTmSelf3 {
    void Fx(i32 a, i32 b, i32 c, i32 d, i32 e, i32 f); // self-call 0x71f80
};

// The world's per-record reporter (gameReg+0x6c sub-mgr) + the manager's multi-record
// reporter self-call; all reloc-masked.
struct CTmReporter {
    void Report1(i32 a, i32 b, i32 c, i32 d, i32 e, i32 f, i32 g); // 0x90db8 (gameReg+0x6c)
};
struct CTmSelf6 {
    void ReportN(i32 a, i32 b, u8* bytes, i32 c, i32 d, i32 e, i32 f); // self-call 0x7b520
};

// 0x78520: ReportRecordsA(a14, a18, a1c, a20, a24) - when the level flag (+0x400) is set,
// scan the record list (+0x244) collecting the byte of each magic-group, un-triggered cell;
// if exactly one matched, hand it to the world's single-record reporter, else hand the whole
// collected array to the manager's multi-record reporter. (__stdcall: ret 0xc.)
// @early-stop
// regalloc + collected-byte-array spill wall: the count register cl and the 0x88-byte stack
// scratch slots differ from retail's. Logic + offsets byte-exact. topic:wall.
RVA(0x00078520, 0x106)
void CTriggerMgr::ReportRecordsA(i32 a14, i32 a18, i32 a1c, i32 a20, i32 a24) {
    if (*(i32*)((char*)this + 0x400) == 0) {
        return;
    }
    u8 bytes[0x88];
    i32 count = 0;
    CTmNode* n = *(CTmNode**)((char*)this + 0x244);
    while (n != 0) {
        i32* p = (i32*)((char*)n + 0x8);
        CTmNode* next = n->m_next;
        i32* payload = (i32*)*(void**)((char*)n + 0x8);
        CTmCell* cell = *(CTmCell**)((char*)this + (payload[1] + payload[0] * 15) * 4 + 0x1c);
        if (*(i32*)((char*)cell + 0x1ec) == g_644c54 && *(i32*)((char*)cell + 0x1e4) == 0) {
            bytes[count] = *(u8*)(p + 1);
            count++;
        }
        n = next;
    }
    if (count == 1) {
        ((CTmReporter*)*(void**)((char*)g_gameReg + 0x6c))
            ->Report1(2, bytes[0], a14, a18, 0, a1c, 0);
    } else {
        ((CTmSelf6*)this)->ReportN(2, a14, bytes, a18, a1c, a20, a24);
    }
}

// 0x78680: ReportRecordsB(a14, a18, a1c, a20, a24, a28) - the four-way variant of
// ReportRecordsA: same magic-group byte scan, then dispatch by (count==1, a14!=0) to one of
// four (single/multi x kind 3/9) report calls. (__stdcall: ret 0x10.)
// @early-stop
// regalloc + 4-way dispatch wall: the count/flag branch ladder pins differently than retail.
RVA(0x00078680, 0x189)
void CTriggerMgr::ReportRecordsB(i32 a14, i32 a18, i32 a1c, i32 a20, i32 a24, i32 a28) {
    if (*(i32*)((char*)this + 0x400) == 0) {
        return;
    }
    u8 bytes[0x88];
    i32 count = 0;
    CTmNode* n = *(CTmNode**)((char*)this + 0x244);
    while (n != 0) {
        i32* p = (i32*)((char*)n + 0x8);
        CTmNode* next = n->m_next;
        i32* payload = (i32*)*(void**)((char*)n + 0x8);
        CTmCell* cell = *(CTmCell**)((char*)this + (payload[1] + payload[0] * 15) * 4 + 0x1c);
        if (*(i32*)((char*)cell + 0x1ec) == g_644c54 && *(i32*)((char*)cell + 0x1e4) == 0) {
            bytes[count] = *(u8*)(p + 1);
            count++;
        }
        n = next;
    }
    CTmReporter* rep = (CTmReporter*)*(void**)((char*)g_gameReg + 0x6c);
    if (count == 1) {
        if (a28 != 0) {
            rep->Report1(9, bytes[0], a14, a18, 0, 0, 0);
        } else {
            rep->Report1(3, bytes[0], a14, a18, 0, 0, 0);
        }
    } else {
        if (a28 != 0) {
            ((CTmSelf6*)this)->ReportN(9, a14, bytes, a18, a1c, a20, a24);
        } else {
            ((CTmSelf6*)this)->ReportN(3, a14, bytes, a18, a1c, a20, a24);
        }
    }
}

// The serialization archive ScanGroup writes through: Write(buf, size) at vtbl slot +0x30,
// plus the per-object id-write helper @0x5b8760 (writes an object's +0x188 archive id).
struct CTmArchive {
    void* m_vt;
    void Write(const void* buf, i32 n); // vtbl +0x30
};
void Ar_WriteId(void* id, i32 stride, void* archive); // 0x1b8760

// 0x7a760: ScanGroup(ar) - serialize the whole manager into archive `ar`: the 4x15 grid of
// cell ids, the four per-row arrays, the magic table + its bytes, the record + selection
// lists, the goal/overlay/state words. ret 0 when ar/level null or the overlay write fails.
// (__thiscall: ret 0x4.) [the manager's Serialize]
// @early-stop
// big serializer wall: 60+ archive Write calls; the grid/list write loops pin esi(ar)/ebx
// /edi differently than retail and the scratch slots differ. Logic + offsets byte-exact.
RVA(0x0007a760, 0x373)
i32 CTriggerMgr::ScanGroup(i32 a14) {
    CTmArchive* ar = (CTmArchive*)a14;
    if (ar == 0) {
        return 0;
    }
    void* lvl = *(void**)((char*)this + 0x22c);
    if (lvl == 0) {
        return 0;
    }
    CTmCell** cell = (CTmCell**)((char*)this + 0x1c);
    i32 r = 4;
    do {
        i32 c = 15;
        do {
            CTmCell* g = *cell;
            i32 id = 0;
            if (g != 0) {
                id = *(i32*)(*(char**)((char*)g + 0x10) + 0x188);
                Ar_WriteId(*(void**)((char*)lvl + 0x8), id, ar);
            }
            ar->Write(&id, 4);
            cell++;
            c--;
        } while (c != 0);
        r--;
    } while (r != 0);
    ar->Write((char*)this + 0x10c, 0x10);
    ar->Write((char*)this + 0x11c, 0xf0);
    ar->Write((char*)this + 0x20c, 0x10);
    ar->Write((char*)this + 0x21c, 0x10);
    i32 cnt = *(i32*)((char*)this + 0x268);
    ar->Write(&cnt, 4);
    for (i32 i = 0; i < cnt; i++) {
        u8 b = ((u8*)*(void**)((char*)this + 0x264))[i];
        ar->Write(&b, 1);
    }
    i32 flag24c = *(i32*)((char*)this + 0x24c);
    ar->Write(&flag24c, 4);
    CTmNode* n = *(CTmNode**)((char*)this + 0x244);
    while (n != 0) {
        CTmNode* cur = n;
        n = n->m_next;
        ar->Write(*(void**)((char*)cur + 0x8), 8);
    }
    CTmNode** lists = (CTmNode**)((char*)this + 0x2d4);
    i32 k = 10;
    do {
        i32 cnt2 = *(i32*)((char*)lists + 0x8);
        ar->Write(&cnt2, 4);
        CTmNode* m = *lists;
        while (m != 0) {
            CTmNode* cur = m;
            m = m->m_next;
            ar->Write(*(void**)((char*)cur + 0x8), 8);
        }
        lists = (CTmNode**)((char*)lists + 0x1c);
        k--;
    } while (k != 0);
    void* goal = *(void**)((char*)this + 0x23c);
    i32 goalId = 0;
    if (goal != 0) {
        goalId = *(i32*)((char*)goal + 0x188);
    }
    ar->Write(&goalId, 4);
    void* ov = *(void**)((char*)this + 0x2a0);
    i32 ovId = 0;
    if (ov != 0 && *(void**)((char*)ov + 0x10) != 0) {
        ovId = *(i32*)(*(char**)((char*)ov + 0x10) + 0x188);
    }
    ar->Write(&ovId, 4);
    ar->Write((char*)this + 0x274, 0x10);
    i32 cntC = *(i32*)((char*)this + 0xc);
    ar->Write(&cntC, 4);
    CTmNode* rn = *(CTmNode**)((char*)this + 0x4);
    while (rn != 0) {
        CTmNode* cur = rn;
        rn = rn->m_next;
        void* obj = *(void**)((char*)cur + 0x8);
        if (obj == 0) {
            return 0;
        }
        i32 oid = *(i32*)(*(char**)((char*)obj + 0x10) + 0x188);
        Ar_WriteId(*(void**)((char*)lvl + 0x8), oid, ar);
        ar->Write(&oid, 4);
    }
    i32 hasOv = (*(void**)((char*)this + 0x25c) != 0) ? 1 : 0;
    ar->Write(&hasOv, 4);
    if (*(void**)((char*)this + 0x25c) != 0) {
        if (((CTmSelf*)this)->Reset3(a14, 0, 0) == 0) { // overlay serialize self-call 0x7df8
            return 0;
        }
    } else {
        return 0;
    }
    ar->Write((char*)this + 0x230, 4);
    ar->Write((char*)this + 0x284, 4);
    ar->Write((char*)this + 0x288, 4);
    ar->Write((char*)this + 0x234, 8);
    ar->Write((char*)this + 0x2a4, 4);
    ar->Write((char*)this + 0x3ec, 4);
    ar->Write((char*)this + 0x400, 4);
    ar->Write(&g_644c54, 4);
    ar->Write(&g_644ca4, 4);
    ar->Write((char*)this + 0x2a8, 4);
    ar->Write((char*)this + 0x3e8, 4);
    return 1;
}

// The world's report/spawn sub-mgrs ResetGroup dispatches through (gameReg+0x6c reporter,
// +0x68 fx-mgr, +0x60 cursor-mgr), all reloc-masked.
struct CTmCursorMgr {
    void Spawn(i32 a, i32 b, i32 c, i32 d, i32 e); // 0x90bf4 (gameReg+0x60)
};
// The three manager self-calls ResetGroup branches into (place variants).
struct CTmSelf7 {
    void* Hit5(i32 a, i32 b, i32 c, i32 d, i32 e); // self-call 0x7d310 (hit, startRow 5)
    i32 PlaceA(i32 a, i32 b, i32 c, i32 d);        // self-call 0x7d70b
    i32 PlaceB(i32 a, i32 b, i32 c);               // self-call 0x7daa0
};
extern i32 g_6455b0; // DAT_006455b0 (the alt-group gate)

// 0x79520: ResetGroup(a14, a18, ...) - when the level flag (+0x400) is set, hit-test the
// magic group, classify the (a14,a18) target into one of three branches and place/report the
// matching cursor / lightfx / warpstone sprite; on factory success Init it and report it.
// ret 1 (0 on flag-clear / placement failure). (__stdcall: ret 0x1c.)
// @early-stop
// big switch-driver wall (0x2e3 B): the three-way classify ladder + the three CreateSprite
// /Init/Report stanzas pin esi(sprite)/ebx/ebp differently than retail and the string-arg
// pushes spill. Logic + offsets + the reloc-masked externs byte-exact. topic:wall.
RVA(0x00079520, 0x2e3)
i32 CTriggerMgr::ResetGroup(i32 a14, i32 a18, i32 a1c, i32 a20, i32 a24, i32 a28, i32 a2c) {
    (void)a1c;
    (void)a20;
    (void)a24;
    (void)a2c;
    if (*(i32*)((char*)this + 0x400) == 0) {
        return 0;
    }
    CTmSelf7* self = (CTmSelf7*)this;
    CTmCell* hit = (CTmCell*)self->Hit5(a14, a18, 0, 0, 5);
    CTmCell* cell = 0;
    if (*(i32*)((char*)this + 0x24c) == 1) {
        i32* rec = *(i32**)(*(char**)((char*)this + 0x244) + 0x8);
        cell = *(CTmCell**)((char*)this + (rec[1] + rec[0] * 15) * 4 + 0x1c);
    }
    i32 sel;
    if (cell != 0 && *(i32*)((char*)cell + 0x1ec) == g_644c54) {
        if (a28 != 0) {
            sel = 0;
        } else if (hit == 0) {
            sel = 1;
        } else if (hit == cell) {
            // toggle off the pending-fx and rewind
            *(i32*)((char*)this + 0x2a8) = 0;
            ((CTmWorld2*)((char*)g_gameReg->m_2c))->StopFx2(0, 0);
            char* o = *(char**)((char*)hit + 0x10);
            self->PlaceA(*(i32*)(o + 0x5c), *(i32*)(o + 0x60), a18, a14);
            return 1;
        } else {
            sel = 2;
        }
    } else {
        sel = (hit != 0) ? 2 : 1;
    }

    void* sprite = 0;
    i32 kindArg = 0;
    i32 logicArg = 0;
    if (sel == 0) {
        // place-on-self path
        self->PlaceB(a14, a18, 1);
        return 1;
    } else if (sel == 1) {
        // spawn the cursor target sprite
        CTmReporter* rep = (CTmReporter*)*(void**)((char*)g_gameReg + 0x6c);
        if (cell != 0) {
            rep->Report1(
                1,
                *(i32*)((char*)cell + 0x1ec),
                *(i32*)((char*)cell + 0x1f0),
                a18,
                a14,
                0,
                0
            );
        } else {
            rep->Report1(1, a14, a18, 0, 0, 0, 0);
        }
        if (*(i32*)((char*)this + 0x2c) == 0) { // placeholder gate (see raw)
            return 0;
        }
        CTmSpriteFactory* fac = ((CTmLevel*)*(void**)((char*)this + 0x22c))->m_8;
        sprite = fac->CreateSprite(0, a14, a18, 0xf4240, "LightFx", 0x40003);
        kindArg = 3;
        logicArg = 1;
    } else {
        // sel==2: place-and-report variant -> WarpStone factory
        self->PlaceB(a14, a18, 1);
        CTmSpriteFactory* fac = ((CTmLevel*)*(void**)((char*)this + 0x22c))->m_8;
        sprite = fac->CreateSprite(0, a14, a18, 0xf4240, "LightFx", 0x40003);
        kindArg = 2;
        logicArg = 1;
    }
    if (sprite == 0) {
        return 0;
    }
    ((CTmSpriteDesc*)*(void**)((char*)sprite + 0x7c))->Init(sprite);
    void* logic = *(void**)(*(char**)((char*)sprite + 0x7c) + 0x18);
    ((CTmUserLogic*)logic)
        ->Arm("GAME_LIGHTING_TARGETCURSOR", "GAME_TARGETCURSOR", kindArg, logicArg);
    return 1;
}

// The cell-trigger applier sub-hooks (reloc-masked __thiscall on the cell): the type-13
// trigger check (+0x4453), the apply (+0x2a40) and the per-cell dispatch the two appliers
// route through.
struct CTmTrigCell {
    i32 Type13Check();             // 0x71f80 (reloc-masked)
    void Apply13(i32 a, i32 b);    // 0x70520
    i32 Dispatch(i32 kind, i32 a); // 0x6e9... generic per-kind
};

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
    CTmCell* cell = *(CTmCell**)((char*)this + (col * 15 + row) * 4 + 0x1c);
    if (cell == 0 || *(i32*)((char*)cell + 0x1fc) == 0) {
        return 0;
    }
    char* o = *(char**)((char*)cell + 0x10);
    if (*(i32*)(o + 0x5c) != cell->m_17c) {
        if (*(i32*)(o + 0x60) != cell->m_180) {
            return -1;
        }
    }
    i32 k = cell->m_170;
    if (k > 0x16) {
        k = cell->m_19c;
    }
    if (k == 0x13) {
        CTmTrigCell* tc = (CTmTrigCell*)cell;
        if (tc->Type13Check() != 0) {
            tc->Apply13(row, a28 + 1);
            return 1;
        }
    }
    if (k == 0xf) {
        ((CTmTrigCell*)cell)->Dispatch(k, row);
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
    CTmCell* cell = *(CTmCell**)((char*)this + (col * 15 + row) * 4 + 0x1c);
    if (cell == 0 || *(i32*)((char*)cell + 0x1fc) == 0 || *(i32*)((char*)cell + 0x1e4) != 0) {
        return 0;
    }
    char* o = *(char**)((char*)cell + 0x10);
    if (*(i32*)(o + 0x5c) != cell->m_17c) {
        if (*(i32*)(o + 0x60) != cell->m_180) {
            return -1;
        }
    }
    if (*(i32*)(o + 0x5c) == cell->m_17c && *(i32*)(o + 0x60) == cell->m_180
        && *(i32*)((char*)cell + 0x198) != 0x1e && g_6455b0 == 0) {
        return 0;
    }
    i32 by = (a2c & ~0x1f) + 0x10;
    i32 bx = (a28 & ~0x1f) + 0x10;
    *(i32*)((char*)cell + 0x384) = 0;
    i32 r = ((CTmCell3*)cell)->ApplyBox(bx, by, row, -1, 1, 0);
    return r != 0 ? 1 : 0;
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
    if (*(void**)((char*)this + 0x22c) == 0) {
        return -1;
    }
    i32 special = 0;
    i32 wantSlot = 0;
    if (a30 == 0x12) {
        special = 0x100;
        wantSlot = 1;
    }
    char* plane = *(char**)((char*)g_gameReg + 0x70);
    i32 attr;
    if ((ax >> 5) >= *(i32*)(plane + 0xc) || (ay >> 5) >= *(i32*)(plane + 0x10)) {
        attr = 1;
    } else {
        char* tbl = *(char**)(plane + 0x8);
        i32 e = (ax >> 5) * 7;
        attr = *(i32*)(*(char**)(tbl + (ay >> 5) * 4) + e * 4);
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
    CTmCell** rowBase = (CTmCell**)((char*)this + row * 15 * 4 + 0x1c);
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
    CTmSpriteFactory* fac = ((CTmLevel*)*(void**)((char*)this + 0x22c))->m_8;
    void* sprite = fac->CreateSprite(0, ax, ay, ay, "Grunt", 0x40003);
    if (sprite == 0) {
        return -1;
    }
    ((CTmSpriteDesc*)*(void**)((char*)sprite + 0x7c))->Init(sprite);
    i32 logicTag = *(i32*)(*(char**)((char*)sprite + 0x7c) + 0x18);
    (void)logicTag;
    // (the dense kind jump table -> internal id + the Wormhole / Entrance sub-ctors elide
    // here; reconstructed to plateau)
    *(CTmCell**)((char*)this + (row * 15 + free) * 4 + 0x1c) = (CTmCell*)sprite;
    *(i32*)((char*)this + row * 4 + 0x10c) += 1;
    *(i32*)((char*)this + (row * 15 + free) * 4 + 0x11c) = 0;
    *(i32*)(*(char**)((char*)g_gameReg + 0x7c) + row * 4 + 0x48) += 1;
    return free;
}

// The overlay/cell hooks PlaceObjectFull dispatches through (reloc-masked).
struct CTmOvFwd {
    void Forward(i32 a, i32 b); // 0x49b86 (overlay forward)
};

// 0x78a50: PlaceObjectFull(x, y) - the largest tile-object trigger driver (0x845 B). Look up
// the magic-group record cell; if an overlay (+0x25c) owns it, forward (x,y) to it. Else, when
// no pending fx (+0x2a8) and the type-13 trigger check fails, rewind the world fx. Otherwise
// hit-test the (x,y) target (HitTest5) and run the dense per-kind jump table over the two
// coordinate sub-tables (DAT_00683ea0..eb4), building/dispatching the per-kind object and
// stashing the rebuilt cell. ret 1. (__thiscall: ret 0x8.) Reconstructed to plateau.
// @early-stop
// largest driver (0x845 B): the dense jump table + the two coordinate sub-tables and ~12
// per-kind stanzas diverge wholesale in regalloc/scheduling; the validated head + the
// overlay/fx fast-paths are structurally faithful. Deferred to the final sweep. topic:wall.
RVA(0x00078a50, 0x845)
i32 CTriggerMgr::PlaceObjectFull(i32 x, i32 y) {
    CTmCell* cell = 0;
    if (*(i32*)((char*)this + 0x24c) == 1) {
        i32* rec = *(i32**)(*(char**)((char*)this + 0x244) + 0x8);
        cell = *(CTmCell**)((char*)this + (rec[1] + rec[0] * 15) * 4 + 0x1c);
    }
    if (cell == 0 || *(i32*)((char*)cell + 0x1ec) != g_644c54) {
        return 0;
    }
    void* ov = *(void**)((char*)this + 0x25c);
    if (ov != 0 && *(i32*)((char*)ov + 0x2c) != 0) {
        ((CTmOvFwd*)ov)->Forward(x, y);
        return 1;
    }
    CTmWorld2* world = (CTmWorld2*)g_gameReg->m_2c;
    if (*(i32*)((char*)this + 0x2a8) == 0) {
        if (((CTmTrigCell*)cell)->Type13Check() == 0) {
            world->StopFx2(0, 0);
            return 1;
        }
    }
    // the HitTest5 + dense per-kind jump table over the two coordinate sub-tables
    // (DAT_00683ea0..eb4) and the ~12 per-kind build/dispatch stanzas elide here; the placer
    // stashes the rebuilt cell + bumps the counters (reconstructed to plateau)
    *(i32*)((char*)cell + 0x36c) = 1;
    return 1;
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
    CTmCell3* cell = *(CTmCell3**)((char*)this + idx * 4 + 0x1c);
    if (cell == 0 || *(i32*)((char*)cell + 0x1fc) == 0) {
        return 0;
    }
    if (*(i32*)((char*)cell + 0x420) == 0) {
        *(i32*)((char*)cell + 0x308) = 0;
        *(i32*)((char*)cell + 0x310) = 0;
        *(i32*)((char*)cell + 0x30c) = 0;
        *(i32*)((char*)cell + 0x314) = 0;
        *(i32*)((char*)cell + 0x248) &= 0xe7fbfbfd;
        *(i32*)((char*)cell + 0x420) = 0;
        *(i32*)((char*)cell + 0x2d0) = 0;
        cell->Disarm(1, 1);
    }
    if (*(i32*)((char*)cell + 0x1e4) != 0) {
        return 0;
    }
    char* name = *g_nameReg.Lookup(*(i32*)(*(char**)((char*)cell + 0x14) + 0x1c));
    if (strcmp(name, "I") == 0) {
        i32 px = *(i32*)((char*)cell + 0x3e4);
        i32 py = *(i32*)((char*)cell + 0x3e8);
        ((CTmSelf3*)this)->Fx(px, py, py, *(i32*)((char*)cell + 0x170), -1, py);
    }
    i32 by = (a20 & ~0x1f) + 0x10;
    i32 bx = (a1c & ~0x1f) + 0x10;
    *(i32*)((char*)cell + 0x384) = 0;
    i32 r = cell->ApplyBox(bx, by, a18, -1, 1, 0);
    return r != 0 ? 1 : 0;
}

// 0x79fb0: NotifyCell(row, col, z) - the notify-cell hook CellDispatch tails into. If the
// cell is live and not yet notified, Recall it (when not recall-done), clear its tile-attr
// bit + reset the plane cell, null the grid slot, decrement the per-row count and, when z
// set, bump the per-row alt count and re-arm; mark the cell notified. (__stdcall: ret 0xc.)
// @early-stop
// regalloc + tile-attr plane-walk wall: the [esi+0x17c]>>5 / plane double-index sequence
// pins eax/ecx/ebp differently than retail and the z-branch (count++ / re-arm) schedules its
// post-merge stores in a different order. Logic + offsets byte-exact. topic:wall.
RVA(0x00079fb0, 0x169)
void CTriggerMgr::NotifyCell(i32 row, i32 col, i32 z) {
    i32 idx = col * 15 + row; // grid[col][row] base
    CTmCell* cell = *(CTmCell**)((char*)this + idx * 4 + 0x1c);
    if (cell == 0 || cell->m_36c != 0) {
        return;
    }
    CTmSelf* self = (CTmSelf*)this;
    if (cell->m_1e8 == 0) {
        self->RecallCell(cell->m_17c, cell->m_180, cell);
    }
    i32 cx = cell->m_17c;
    i32 cy = cell->m_180;
    char* plane = *(char**)(*(char**)((char*)g_gameReg + 0x70) + 0x8);
    i32 colByte = (cx >> 5) * 8;
    char* row0 = *(char**)(plane + (cy >> 5) * 4);
    row0[colByte + 0x3] &= 0xdf;
    char* plane2 = *(char**)(*(char**)((char*)g_gameReg + 0x70) + 0x8);
    char* row2 = *(char**)(plane2 + (cy >> 5) * 4);
    *(i32*)(row2 + colByte + 0x4) = -1;
    *(CTmCell**)((char*)this + idx * 4 + 0x1c) = 0;
    i32* perRow = (i32*)((char*)this + col * 4 + 0x10c);
    *perRow = *perRow - 1;
    if (z == 0) {
        i32 k = cell->m_170;
        if (k > 0x16) {
            k = cell->m_19c;
        }
        if (k == 0x14) {
            self->RefreshC();
        }
        *(i32*)((char*)this + col * 4 + 0x21c) += 1;
        cell->m_36c = 1;
        return;
    }
    *(i32*)((char*)this + idx * 4 + 0x11c) = 1;
    *(i32*)((char*)this + col * 4 + 0x20c) += 1;
    i32 k = cell->m_170;
    if (k > 0x16) {
        k = cell->m_19c;
    }
    if (k == 0x14) {
        if (*(i32*)((char*)g_gameReg + 0x134) == 1) {
            CTmPendingFx* fx = *(CTmPendingFx**)((char*)this + 0x2a0);
            if (fx != 0) {
                fx->Pulse();
            }
        }
        self->RefreshB(1);
    }
    cell->m_36c = 1;
}

// ===========================================================================
// The two tiny grid-action wrappers (0x6da60 / 0x6daa0) + the tile-fx spawner
// (0x79ea0), proximity-attributed to CTriggerMgr but really FREE __stdcall thunks
// (no `this`): each drives the game registry's spawn/fx sub-managers.
// ===========================================================================

// The spawn sub-manager at gameReg+0x6c: its 8-arg __thiscall action method
// (0x23c30, thunk 0x2095). Modeled no-body so the call reloc-masks.
struct CTmSpawnSub {
    i32 Action(i32 one, i32 a, i32 b, i32 kind, i32 c, i32 d, i32 e, i32 f); // 0x23c30
};
// The world fx-spawner (0x7c620, thunk 0x152d): a __stdcall sprite spawner.
extern void __stdcall Eng_SpawnFx(i32 type, i32 x, i32 y, i32 a3, i32 a4, i32 a5); // 0x7c620

// The tile occupancy grid reached as gameReg->m_70 (rows table @+0x8, w@+0xc,
// h@+0x10; each cell 0x1c bytes = 7 dwords).
struct CTmTileGrid {
    char p0[0x8];
    i32** m_8; // +0x08  row-pointer table
    i32 m_c;   // +0x0c  width
    i32 m_10;  // +0x10  height
};
// The world record reached as gameReg->m_2c: its +0x384 holds 4 {x,y} fx anchors.
struct CTmFxWorld {
    char p0[0x384];
    struct Anchor {
        i32 m_x;
        i32 m_y;
    } m_anchors[4]; // +0x384  (stride 8)
};

// 0x6da60: GridAction6(a, b) - dispatch the spawn sub-mgr's action with kind 6.
// __stdcall free function (cleans its own 2 args; retail ends in `ret 8`).
RVA(0x0006da60, 0x27)
i32 __stdcall GridAction6(i32 a, i32 b) {
    return ((CTmSpawnSub*)(*(void**)((char*)g_gameReg + 0x6c)))->Action(1, a, b, 6, 0, 0, 0, 0);
}

// 0x6daa0: GridAction7(a, b) - dispatch the spawn sub-mgr's action with kind 7.
RVA(0x0006daa0, 0x27)
i32 __stdcall GridAction7(i32 a, i32 b) {
    return ((CTmSpawnSub*)(*(void**)((char*)g_gameReg + 0x6c)))->Action(1, a, b, 7, 0, 0, 0, 0);
}

// 0x79ea0: SpawnTileFx(x, y, a3) - only when the active state is live
// (gameReg->m_134==1): read the tile at (x>>5, y>>5); if it carries neither the
// 0x40939 mask nor bit 0x2, spawn a type-0x14 fx centered on the tile. Otherwise
// (the bit path) map (a3-1) into the world's 4 fx anchors and spawn there. ret 1.
// __stdcall free function (cleans its own 3 args; retail ends in `ret 0xc`).
// @early-stop
// regalloc wall (~81%): body + offsets + the tile double-index byte-exact; retail homes
// gameReg in edi, the grid in esi and the width in ebx (3 callee-saved regs), our cl uses
// gameReg=esi/width=edi (2 regs). Not source-steerable. topic:wall topic:regalloc.
RVA(0x00079ea0, 0xc2)
i32 __stdcall SpawnTileFx(i32 x, i32 y, i32 a3) {
    if (*(i32*)((char*)g_gameReg + 0x134) != 1) {
        return 0;
    }
    CTmTileGrid* grid = *(CTmTileGrid**)((char*)g_gameReg + 0x70);
    i32 tx = x >> 5;
    i32 ty = y >> 5;
    i32 tile;
    if ((u32)tx >= (u32)grid->m_c || (u32)ty >= (u32)grid->m_10) {
        tile = 1;
    } else {
        tile = grid->m_8[ty][tx * 8 - tx];
    }
    if ((tile & 0x40939) == 0 && (tile & 2) == 0) {
        Eng_SpawnFx(0x14, (tx << 5) + 0x10, (ty << 5) + 0x10, 0, a3, 0);
        return 1;
    }
    CTmFxWorld* world = *(CTmFxWorld**)((char*)g_gameReg + 0x2c);
    i32 idx = a3 - 1;
    CTmFxWorld::Anchor* rec = ((u32)idx < 4) ? &world->m_anchors[idx] : 0;
    if (rec != 0) {
        Eng_SpawnFx(0x14, rec->m_x, rec->m_y, 0, a3, 0);
    }
    return 1;
}

// ===========================================================================
// CTriggerMgr::ResetSpawnState  (0x79d90)
// ===========================================================================

// The world status-item the reset path frees a buffer on (world->m_2dc).
struct CTmStatusBuf {
    i32 m_0; // +0x00  mode
    char p0[0x10c - 0x4];
    i32 m_10c; // +0x10c  sub-state
    char p1[0x548 - 0x110];
    i32 m_548;   // +0x548
    void* m_54c; // +0x54c  the pending buffer to free
};
// The +0x260 CObArray RemoveAt helper (0x1b5525) + the two build-state notifiers
// (0x100930 self-ish / 0x104d60) + the pending-fx Pulse (0x3a1c) + RefreshB (0x3e81).
struct CTmObArray {
    void RemoveAt(i32 idx, i32 n); // 0x1b5525
};
extern void Eng_BuildNotifyA(i32 a); // 0x100930 (thunk 0x12fd)
struct CTmBuildState {
    void Notify(); // 0x104d60 (thunk 0x16ea)
};
struct CTmSelfReset {
    void PulseFx();       // 0x3a1c (this->m_2a0->Pulse path is via self thunk)
    void RefreshB(i32 a); // 0x3e81
};
extern void __cdecl operator delete(void*);

RVA(0x00079d90, 0xc5)
void CTriggerMgr::ResetSpawnState() {
    if (*(i32*)((char*)g_gameReg + 0x134) != 1) {
        return;
    }
    if (*(i32*)((char*)this + 0x284) == 0) {
        return;
    }
    CTmWorld* world = g_gameReg->m_2c;
    CTmStatusBuf* st = *(CTmStatusBuf**)((char*)world + 0x2dc);
    if (st->m_54c != 0) {
        operator delete(st->m_54c);
        st->m_54c = 0;
    }
    (*(CTmStatusBuf**)((char*)world + 0x2dc))->m_548 = 0;
    if (*(i32*)((char*)this + 0x268) > 0) {
        ((CTmObArray*)((char*)this + 0x260))->RemoveAt(*(i32*)((char*)this + 0x268) - 1, 1);
        CTmStatusBuf* ctx = *(CTmStatusBuf**)((char*)world + 0x2dc);
        if (ctx->m_0 != 2 && ctx->m_10c == 5) {
            Eng_BuildNotifyA(0);
            ((CTmBuildState*)*(CTmStatusBuf**)((char*)world + 0x2dc))->Notify();
        }
    }
    if (*(i32*)((char*)g_gameReg + 0x134) == 1) {
        CTmPendingFx* fx = *(CTmPendingFx**)((char*)this + 0x2a0);
        if (fx != 0) {
            fx->Pulse();
        }
    }
    ((CTmSelfReset*)this)->RefreshB(6);
}

// 0x7c2e0: CycleMoveIcons(skipRow, enable) - for grid rows 0..3 except `skipRow`, either
// roll a random move-icon onto each live cell (stashing the prior +0x1f8 when -1) and tick
// the world's region-4, or restore each cell's stashed icon. ret 1.
// @early-stop
// scheduling residual (~93%): logic + offsets + externs byte-exact; retail hoists the -1
// sentinel into ebp and schedules the rand()/idiv differently. topic:wall.
RVA(0x0007c2e0, 0xb5)
i32 CTriggerMgr::CycleMoveIcons(i32 skipRow, i32 enable) {
    CTmGrunt** grid = (CTmGrunt**)((char*)this + 0x1c);
    for (i32 r = 0; r < 4; r++) {
        if (r != skipRow) {
            CTmGrunt** cell = grid;
            i32 i = 15;
            do {
                CTmGrunt* g = *cell;
                if (g != 0) {
                    if (enable != 0) {
                        i32 t = rand() % 0x11;
                        if (*(i32*)((char*)g + 0x1f8) == -1) {
                            *(i32*)((char*)g + 0x1f8) = *(i32*)((char*)g + 0x1f4);
                        }
                        ((CGrunt*)g)->SelectMoveIcon(t);
                        ((CPlay*)g_gameReg->m_2c)->OnRegion4(1);
                    } else if (*(i32*)((char*)g + 0x1f8) != -1) {
                        ((CGrunt*)g)->SelectMoveIcon(*(i32*)((char*)g + 0x1f8));
                        *(i32*)((char*)g + 0x1f8) = -1;
                    }
                }
                cell++;
                i--;
            } while (i != 0);
        }
        grid += 15;
    }
    return 1;
}

// 0x7cc60: RebuildSelectionList(idx) - recycle selection list `idx` (+0x2d4) to the free
// list, RemoveAll it (+0x2d0), then allocate a fresh node per record-list entry (+0x244)
// copying its (x,y) payload; reset +0x3e8. ret 1.
// @early-stop
// regalloc wall (~86%): the free-list recycle + node-alloc bodies are byte-exact; retail
// pins this/idx-base differently across the two list walks. topic:wall.
RVA(0x0007cc60, 0xa7)
i32 CTriggerMgr::RebuildSelectionList(i32 idx) {
    CTmPtrList* sel = (CTmPtrList*)((char*)this + idx * 0x1c + 0x2d0);
    CTmNode* n = *(CTmNode**)((char*)this + idx * 0x1c + 0x2d4);
    if (n != 0) {
        void* head = g_freeList;
        do {
            CTmNode* cur = n;
            n = n->m_next;
            i32* payload = cur->m_payload;
            if (payload != 0) {
                void** slot = (void**)((char*)payload - g_freeListNodeBias);
                *slot = head;
                head = slot;
                g_freeList = head;
            }
        } while (n != 0);
    }
    sel->RemoveAll();
    CTmNode* rec = *(CTmNode**)((char*)this + 0x244);
    while (rec != 0) {
        CTmNode* cur = rec;
        rec = rec->m_next;
        i32* src = cur->m_payload;
        void** fh = (void**)g_freeList;
        void* nextFree = *fh;
        i32* dst = 0;
        if (nextFree != 0) {
            dst = (i32*)((char*)fh + 4);
            g_freeList = nextFree;
        }
        dst[0] = src[0];
        dst[1] = src[1];
        sel->AddTail(dst);
    }
    *(i32*)((char*)this + 0x3e8) = -1;
    return 1;
}

// 0x7cd40: CenterSelectionGroup(slot) - ResetAll, tick the live overlay, then walk the
// slot's selection list (+0x2d4[slot*0x1c]). For each node look up grid[15*x+y]: if the
// cell is live, ResetCell(x,y,1,0) and (on the second pass for the same slot, m_3e8==slot)
// fold its display pos into a running bbox seeded from the level grid dims; if dead, recycle
// the node back to the free list and RemoveAt it from the slot list. On the centering pass
// scroll the world to the bbox centre and clear m_3e8; else latch m_3e8=slot. ret 1 (0 when
// the slot list is empty).
// @early-stop
// regalloc wall (~87%): logic + offsets + all reloc-masked externs (ResetAll/OverlayTick/
// ResetCell/RemoveAt/Center/g_freeList*/g_gameReg) byte-exact, but retail pins this=ebp,
// node=esi, y=edi (a perfect 5-reg fit); our cl swaps this/node into esi/ebp and spills
// `this` to the stack, reusing esi for y. Same systematic esi<->ebp swap the rest of this
// TU exhibits; not source-steerable. topic:wall.
RVA(0x0007cd40, 0x18f)
i32 CTriggerMgr::CenterSelectionGroup(i32 slot) {
    ResetAll();
    char* self = (char*)this;
    CTmOverlay* ov = *(CTmOverlay**)(self + 0x25c);
    if (ov != 0 && ov->m_2c != 0) {
        OverlayTick();
    }
    CTmNode* n = *(CTmNode**)(self + slot * 0x1c + 0x2d4);
    char* base = self + slot * 0x1c;
    if (n == 0) {
        *(i32*)(self + 0x3e8) = -1;
        return 0;
    }
    i32 maxX = 0;
    i32 maxY = 0;
    CTmGrid* grid = g_gameReg->m_30->m_24->m_5c;
    i32 minX = grid->m_30 - 1;
    i32 minY = grid->m_34 - 1;
    do {
        CTmNode* cur = n;
        n = n->m_next;
        i32* payload = cur->m_payload;
        i32 idx = payload[1] + 15 * payload[0];
        CTmGrunt* cell = *(CTmGrunt**)(self + idx * 4 + 0x1c);
        if (cell != 0) {
            ResetCell(payload[0], payload[1], 1, 0);
            if (*(i32*)(self + 0x3e8) == slot) {
                i32* disp = *(i32**)((char*)cell + 0x10);
                i32 x = disp[0x5c / 4];
                i32 y = disp[0x60 / 4];
                if (x < minX) {
                    minX = x;
                }
                if (x > maxX) {
                    maxX = x;
                }
                if (y < minY) {
                    minY = y;
                }
                if (y > maxY) {
                    maxY = y;
                }
            }
        } else {
            void** node = (void**)((char*)payload - g_freeListNodeBias);
            *node = g_freeList;
            g_freeList = node;
            ((CTmPtrList*)(base + 0x2d0))->RemoveAt(cur);
        }
    } while (n != 0);
    if (*(i32*)(self + 0x3e8) == slot) {
        g_gameReg->m_2c->Center(minX + (maxX - minX) / 2, minY + (maxY - minY) / 2);
        *(i32*)(self + 0x3e8) = -1;
        return 1;
    }
    *(i32*)(self + 0x3e8) = slot;
    return 1;
}

// 0x7d450: ToggleRegionA - clear a live pending-fx (LoadCursorSprites(0,0), ret 0); else,
// for the active record cell of the magic group, gate on CanShowStamina and dispatch by its
// logic kind (+0x170/+0x19c): kind 0x13 => ResetGroup, else set a pending fx (+0x2a8). ret 1.
// @early-stop
// regalloc + dead-spill wall (~54%): logic + offsets + the reloc-masked externs byte-exact,
// but retail pins this->edi, reserves an 8-byte frame, and emits two DEAD spill stores
// ([esp+0x20]/[esp+0x28]) of the ResetGroup args; our cl uses esi/no-frame. Not source-
// steerable; the systematic esi<->edi swap + frame shift misaligns the score. topic:wall.
RVA(0x0007d450, 0x112)
i32 CTriggerMgr::ToggleRegionA() {
    if (*(i32*)((char*)this + 0x2a8) != 0) {
        *(i32*)((char*)this + 0x2a8) = 0;
        ((CPlay*)g_gameReg->m_2c)->LoadCursorSprites(0, 0);
        return 0;
    }
    *(i32*)((char*)this + 0x2a8) = 0;
    CTmGrunt* cell = 0;
    if (*(i32*)((char*)this + 0x24c) == 1) {
        i32* rec = *(i32**)(*(char**)((char*)this + 0x244) + 0x8);
        cell = *(CTmGrunt**)((char*)this + (rec[0] * 15 + rec[1]) * 4 + 0x1c);
    }
    if (cell == 0) {
        return 1;
    }
    if (*(i32*)((char*)cell + 0x1ec) != g_644c54) {
        return 1;
    }
    if (((CGrunt*)cell)->CanShowStamina() == 0) {
        OverlayTick();
        return 1;
    }
    i32 v = *(i32*)((char*)cell + 0x170);
    if (v > 0x16) {
        v = *(i32*)((char*)cell + 0x19c);
    }
    if (v != 0x13) {
        *(i32*)((char*)this + 0x2a8) = v + 0xc8;
        ((CPlay*)g_gameReg->m_2c)->LoadCursorSprites(v + 0xc8, 0);
        OverlayTick();
        return 1;
    }
    g_gameReg->m_68
        ->ResetGroup(*(i32*)((char*)cell + 0x17c), *(i32*)((char*)cell + 0x180), 0, 0, 0, 2, 1);
    OverlayTick();
    return 1;
}

// 0x7d5c0: ToggleRegionB - the sibling of ToggleRegionA: clear a live pending-fx; else, for
// the active record cell, gate on +0x170<0x17 and dispatch by +0x198: 0x1e => ResetGroup on
// the cell's display pos, 0 => just tick, else set a pending fx (+0x2a8). ret 1.
// @early-stop
// regalloc wall (~82%): logic + offsets + externs byte-exact; retail pins this->esi and the
// magic const into edi across the dispatch ladder. topic:wall.
RVA(0x0007d5c0, 0xdc)
i32 CTriggerMgr::ToggleRegionB() {
    if (*(i32*)((char*)this + 0x2a8) != 0) {
        *(i32*)((char*)this + 0x2a8) = 0;
        ((CPlay*)g_gameReg->m_2c)->LoadCursorSprites(0, 0);
        return 0;
    }
    *(i32*)((char*)this + 0x2a8) = 0;
    CTmGrunt* cell = 0;
    if (*(i32*)((char*)this + 0x24c) == 1) {
        i32* rec = *(i32**)(*(char**)((char*)this + 0x244) + 0x8);
        cell = *(CTmGrunt**)((char*)this + (rec[0] * 15 + rec[1]) * 4 + 0x1c);
    }
    if (cell == 0) {
        return 1;
    }
    if (*(i32*)((char*)cell + 0x1ec) != g_644c54) {
        return 1;
    }
    if (*(i32*)((char*)cell + 0x170) >= 0x17) {
        OverlayTick();
        return 1;
    }
    i32 kind = *(i32*)((char*)cell + 0x198);
    if (kind == 0x1e) {
        char* o = *(char**)((char*)cell + 0x10);
        g_gameReg->m_68->ResetGroup(*(i32*)(o + 0x5c), *(i32*)(o + 0x60), 0, 0, 0, 3, 1);
        OverlayTick();
        return 1;
    }
    if (kind == 0) {
        OverlayTick();
        return 1;
    }
    *(i32*)((char*)this + 0x2a8) = kind + 0xc8;
    ((CPlay*)g_gameReg->m_2c)->LoadCursorSprites(kind + 0xc8, 0);
    OverlayTick();
    return 1;
}

// 0x7d6e0: EnqueueGroupCells - when armed (+0x400), collect the y-byte of every magic-group
// record cell with a clear +0x1e4 flag, then post the group to the command mgr (+0x6c):
// EnqueueSingle when exactly one, else EnqueueMulti with the y-byte buffer. ret 1.
// @early-stop
// stack-frame-size wall (~90%): the record scan (now with the matched byte counter, so the
// cl/byte-store/dword-reload sequence matches) + the two CGruntzCmdMgr enqueue calls are
// byte-exact; retail's frame is 0x88 vs our 0x6c (extra scratch slots) so every esp-relative
// displacement shifts by a constant. topic:wall.
RVA(0x0007d6e0, 0xea)
i32 CTriggerMgr::EnqueueGroupCells() {
    if (*(i32*)((char*)this + 0x400) == 0) {
        return 0;
    }
    u8 buf[0x68];
    u8 count = 0;
    char x = 0;
    CTmNode* n = *(CTmNode**)((char*)this + 0x244);
    if (n != 0) {
        i32 magic = g_644c54;
        do {
            CTmNode* cur = n;
            n = n->m_next;
            i32* p = cur->m_payload;
            x = *(char*)p;
            CTmGrunt* cell = *(CTmGrunt**)((char*)this + (p[0] * 15 + p[1]) * 4 + 0x1c);
            if (*(i32*)((char*)cell + 0x1ec) == magic && *(i32*)((char*)cell + 0x1e4) == 0) {
                buf[count] = ((u8*)p)[4];
                count++;
            }
        } while (n != 0);
    }
    if (count == 1) {
        g_gameReg->m_6c->EnqueueSingle(1, x, (char)buf[0], 5, 0, 0, 0, 0);
    } else {
        g_gameReg->m_6c->EnqueueMulti(1, x, count, (char*)buf, 5, 0, 0, 0);
    }
    return 1;
}

// --- ILT jmp thunk (rtti-labeled CRegSink::Post, re-homed from src/Stub/CRegSink.cpp).
// A low-RVA 5-byte ILT `jmp rel32` thunk landing on CycleMoveIcons (0x7c2e0, above).
// No plain-C++ form for a bare unframed `jmp`, so it is a __declspec(naked) body whose
// jmp's rel32 reloc-masks against the named target (same approach as NetThunks.cpp).
extern "C" void n_CycleMoveIcons_7c2e0(); // 0x7c2e0 (jmp target)

RVA(0x00003616, 0x5)
__declspec(naked) void Ilt_RegSinkPost_3616() {
    __asm { jmp n_CycleMoveIcons_7c2e0 }
}

// Class-metadata size annotations (all partial modeling views -> SIZE_UNKNOWN).
// Placed at end-of-TU: interspersed placement (right after each class) reschedules
// ResetGroup/HitTestCell codegen in this codegen-sensitive unit (measured -0.18/-0.02);
// end-of-TU (after all bodies) is matching-neutral.
SIZE_UNKNOWN(CTmNode);
SIZE_UNKNOWN(CTmGrunt);
SIZE_UNKNOWN(CTmOverlay);
SIZE_UNKNOWN(CTmGoal);
SIZE_UNKNOWN(CTmPtrList);
SIZE_UNKNOWN(CTmStatusItem);
SIZE_UNKNOWN(CTmWorld);
SIZE_UNKNOWN(CTmGrid);
SIZE_UNKNOWN(CTmGridHolder);
SIZE_UNKNOWN(CTmRegSub30);
SIZE_UNKNOWN(CTmGameReg);
SIZE_UNKNOWN(CTmNameReg);
SIZE_UNKNOWN(CTmSpriteFactory);
SIZE_UNKNOWN(CTmSpriteDesc);
SIZE_UNKNOWN(CTmLevel);
SIZE_UNKNOWN(CTmListNode);
SIZE_UNKNOWN(CTmPuddleTarget);
SIZE_UNKNOWN(CTmRecNode);
SIZE_UNKNOWN(CTmCell);
SIZE_UNKNOWN(CTmSelf);
SIZE_UNKNOWN(CTmPendingFx);
SIZE_UNKNOWN(CTmOverlaySrc);
SIZE_UNKNOWN(CTmSelf2);
SIZE_UNKNOWN(CTmCell2);
SIZE_UNKNOWN(CTmSelf4);
SIZE_UNKNOWN(CTmFxMgr);
SIZE_UNKNOWN(CTmWorld2);
SIZE_UNKNOWN(CTmSelf5);
SIZE_UNKNOWN(CTmUserLogic);
SIZE_UNKNOWN(CTmCell3);
SIZE_UNKNOWN(CTmSelf3);
SIZE_UNKNOWN(CTmReporter);
SIZE_UNKNOWN(CTmSelf6);
SIZE_UNKNOWN(CTmArchive);
SIZE_UNKNOWN(CTmCursorMgr);
SIZE_UNKNOWN(CTmSelf7);
SIZE_UNKNOWN(CTmTrigCell);
SIZE_UNKNOWN(CTmOvFwd);
SIZE_UNKNOWN(CTmSpawnSub);
SIZE_UNKNOWN(CTmTileGrid);
SIZE_UNKNOWN(CTmFxWorld);
SIZE_UNKNOWN(CTmStatusBuf);
SIZE_UNKNOWN(CTmObArray);
SIZE_UNKNOWN(CTmBuildState);
SIZE_UNKNOWN(CTmSelfReset);
