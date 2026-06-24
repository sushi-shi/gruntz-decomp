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
// from the reconstructed leaves are declared; the +0x368 notify-hook flag is read.
struct CTmGrunt {
    void ClearAllSprites();      // 0x4b240 (reloc-masked)
    void ExitGrid();             // 0x641b0 (reloc-masked)
    void Route(i32 kind, i32 a); // 0x60150 (reloc-masked)
    char p0[0x368];
    void* m_368; // +0x368  notify hook
};

// The small overlay sub-object allocated at CTriggerMgr+0x25c (0x40 bytes). Only its
// two reloc-masked __thiscall hooks are dispatched from the reconstructed leaves.
struct CTmOverlay {
    void Tick();   // 0x97f0  (reloc-masked)
    i32 Release(); // 0x94c0  (reloc-masked) - ret used by OverlayRelease
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
struct CTmWorld {
    void StopFx(i32 a, i32 b); // 0xd0120 (__thiscall, reloc-masked)
    void Refresh();            // 0xda2d0 (__thiscall, reloc-masked)
    char p0[0x504];
    void* m_504; // +0x504  pending-fx flag
};
struct CTmGameReg {
    char p0[0x2c];
    CTmWorld* m_2c; // +0x2c  the active world/play object
};
extern CTmGameReg* g_gameReg;

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
