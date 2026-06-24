// TriggerMgr.cpp - CTriggerMgr, the playfield tile-object / switch-trigger grid
// manager (trace placeholder ClassUnknown_23, C:\Proj\Gruntz). See TriggerMgr.h.
//
// Members are read by raw this+offset: the grid cells, the level/plane objects and
// the list-node payloads are full UNMATCHED engine classes modeled here as opaque
// shells so their member reads / helper calls reloc-mask. Functions in retail-RVA
// order.
#include <Gruntz/TriggerMgr.h>

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
    void StopFx(i32 a, i32 b);  // 0xd0120 (__thiscall, reloc-masked)
    void Refresh();             // 0xda2d0 (__thiscall, reloc-masked)
    void SetStat(i32 a, i32 b); // 0xd9240 (__thiscall, reloc-masked)
    char p0[0x2dc];
    CTmStatusItem* m_2dc; // +0x2dc  status-bar item
    char p1[0x504 - 0x2e0];
    void* m_504; // +0x504  pending-fx flag
};
struct CTmGameReg {
    void* PickPausedThenPlayState();       // 0x929b0 (reloc-masked) - the play/pause state obj
    void ReportError(i32 code, i32 flags); // 0x8dc60 (reloc-masked)
    char p0[0x2c];
    CTmWorld* m_2c; // +0x2c  the active world/play object
};
extern CTmGameReg* g_gameReg;

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
// the reloc without pulling the full CGrunt layout.
class CGrunt {
public:
    void ReadConfigFromButeMgr();
};

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
// (view@m_24: scroll struct @[m_5c]+0x40, origin @m_10/m_14) and forward to CellHitTest.
RVA(0x0006be30, 0x47)
void* CTriggerMgr::ScreenToCell(i32 sx, i32 sy, i32* outRow, i32* outCol, i32 startRow) {
    char* view = *(char**)(*(char**)((char*)this + 0x22c) + 0x24);
    char* scroll = *(char**)(*(char**)(view + 0x5c) + 0x40);
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
