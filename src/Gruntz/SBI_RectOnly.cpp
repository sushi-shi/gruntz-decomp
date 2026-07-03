#include <Gruntz/SBI_RectOnly.h> // canonical CSBI_RectOnly + engine-referent views
#include <Globals.h>
// SBI_RectOnly.cpp - Gruntz CSBI_RectOnly (C:\Proj\Gruntz).
// The constructor is matched byte-exact.
//
// CSBI_RectOnly derives from CStatusBarItem. The retail ctor inlines the base
// ctor (zeroing m_4/m_24/m_28; the base's m_8=0 store is dropped as dead because
// the derived ctor sets m_8=1), then stores its own vptr, then m_8=1. That fold
// is exactly why CStatusBarItem's ctor is inline in the shared header (MSVC 5.0
// will not fold an out-of-line base ctor).

// CSBI_RectOnly + all its engine-referent views (CSbiSlot/CSbiStream/CSbiSprite/
// ...) and the slot-state enum/consts now live in the canonical shared header
// <Gruntz/SBI_RectOnly.h> (included above). Only the RVA-keyed method bodies and
// the DATA()-bound globals remain in this TU.

// An unnamed engine DWORD global read by the HUD-rect group setters.
DATA(0x00245588)
extern i32 g_dat645588;

// The current local-player / area index (PlaceCursorTarget's tile-grid column).
DATA(0x00244c54)
extern i32 g_644c54;

// CSbiLookupMap/CSbiCueRecord/CSbiCuePlayer/CSbiMusicHost/CSbiGameMgr/CSbiSubMgr/
// CSbiTile*/CSbiActiveObj/CSbiLogger/CSbiWndHost/CGameReg moved to
// <Gruntz/SBI_RectOnly.h>.
DATA(0x0024556c)
extern CGameReg* g_gameReg;

// The reentrancy gate + cue-item id pair the highlight handlers play through.
DATA(0x0021ab20)
extern i32 g_61ab20;
DATA(0x0021ab24)
extern i32 g_61ab24;
// The draw-clock mirror (g_6bf3c0), unsigned for the wrap-safe gate compare.
DATA(0x002bf3c0)
extern "C" u32 g_6bf3c0;

// Global serialize-sequence counter (bumped once per Serialize).
DATA(0x00229ad0)
extern i32 g_serialCounter;

// The engine free-list head + the node-pointer bias (raw subtrahend), shared with
// Projectile/TriggerMgr. The teardown returns each pooled element to this list.
DATA(0x00245544)
extern void* g_freeList;
DATA(0x0024554c)
extern i32 g_freeListNodeBias;

CStatusBarItem::~CStatusBarItem() {}
i32 CStatusBarItem::SbiVfunc0() {
    return 0;
}

// ---------------------------------------------------------------------------
// CSBI_RectOnly::CSBI_RectOnly()
// Inlines the CStatusBarItem base ctor (the dead m_8=0 store is elided), stores
// its own vptr, then sets m_8 = 1.
RVA(0x00101fa0, 0x1b)
CSBI_RectOnly::CSBI_RectOnly() {
    m_8 = 1;
}

i32 CSBI_RectOnly::SbiVfunc0() {
    return 1;
}

// ---------------------------------------------------------------------------
// CSBI_RectOnly::Setup - vtable slot 2 (0xe86e0). The 10-arg config setter (the
// last two args are accepted by the ABI but unused). Bails (returns 0) if either
// the object id (a2) or the owner (a1) is null; otherwise stores the eight live
// args into the base-region fields, marks m_4 = 1 (active) and returns 1.
// @early-stop
// 90.4%: every field/store and the control flow are correct, BUT retail writes the
// +0x14 sub-block through a `lea edx,[ecx+0x14]` base (strict sequential load-store)
// while MSVC5 here folds it to ecx-relative disp8 stores and interleaves the loads.
// A pure instruction-selection/scheduling residual (docs/patterns/
// statement-schedule-faithful.md) - direct, array, and struct-pointer spellings all
// fold the same; not steerable from C. Deferred to the final sweep.
RVA(0x000e86e0, 0x53)
i32 CSBI_RectOnly::
    Setup(i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6, i32 a7, i32 a8, i32 a9, i32 a10) {
    if (a2 == 0 || a1 == 0) {
        return 0;
    }
    m_2c = a1;
    m_24 = a2;
    m_10 = a4;
    m_rect14.m_0 = a5;
    m_rect14.m_4 = a6;
    m_rect14.m_8 = a7;
    m_rect14.m_c = a8;
    m_c = a3;
    m_4 = 1;
    return 1;
}

// m_34 = m_30 = 0.
RVA(0x000e7400, 0x9)
void CSBI_RectOnly::ResetCounters() {
    m_34 = 0;
    m_30 = 0;
}

// Reset slots 0..4, then m_activeSlot = -1.
RVA(0x00105520, 0x21)
void CSBI_RectOnly::ResetSlots() {
    for (i32 i = 0; i < 5; i++) {
        ArmSlot(i);
    }
    m_activeSlot = -1;
}

// Arm slot[idx]: state = armed, value = 1; notify the slot's pointer (vfunc 0x30).
RVA(0x00105560, 0x33)
void CSBI_RectOnly::ArmSlot(i32 idx) {
    m_slots[idx].m_state = kSlotArmed;
    m_slots[idx].m_value = 1;
    if (m_slotNotify[idx]) {
        m_slotNotify[idx]->Notify(1);
    }
}

// Probe slots 0..4; return 1 on first hit, else 0.
RVA(0x00105710, 0x23)
i32 CSBI_RectOnly::AnySlotActive() {
    for (i32 i = 0; i < 5; i++) {
        if (ProbeSlot(i)) {
            return 1;
        }
    }
    return 0;
}

// m_gaugeTarget = min(m_gauge + delta, 100).
RVA(0x00105750, 0x1f)
void CSBI_RectOnly::AdvanceGauge(i32 delta) {
    i32 v = m_gauge + delta;
    if (v >= 100) {
        v = 100;
    }
    m_gaugeTarget = v;
}

// m_gaugeTarget = m_gauge = value.
RVA(0x001057d0, 0x13)
void CSBI_RectOnly::SetGauge(i32 value) {
    m_gaugeTarget = value;
    m_gauge = value;
}

// Place the cursor on the resolved tile under highlight row `row`: probe the active
// object's tile at (g_644c54, row); bail (0) if the probe fails or the grid
// cell is empty. Forward the tile's origin pair to the sub-manager's ScrollTo, then
// (when `commit` is set and the active object accepts the scroll) latch the placed
// column/row and reload the camera sprite. Always returns 1 past the two probes.
// @early-stop
// ~71%: the code bytes are byte-exact vs retail (same regs/order/offsets; verified by
// llvm-objdump -dr base vs target). The residual is purely the reloc-symbol-naming
// scoring tail - this TU models the g_gameReg singleton as ?g_gameReg@@3PAUCGameReg@@A
// while the retail obj names it _g_mgrSettings, so the three DIR32 data relocs don't
// pair (weighted heavily on a short function). g_644c54 + the ILT call thunks already
// pair. A TU-wide g_gameReg rename, not a per-function fix; matcher.md reloc artifact.
RVA(0x00105800, 0x9e)
i32 CSBI_RectOnly::PlaceCursorTarget(i32 row, i32 commit) {
    i32 col = g_644c54;
    if (g_gameReg->m_68->ProbeXY(col, row, 0, 0) == 0) {
        return 0;
    }
    CSbiTileEntry* entry = g_gameReg->m_68->m_grid[row + col * 15];
    if (entry == 0) {
        return 0;
    }
    g_gameReg->m_curState->ScrollTo(entry->m_10->m_5c, entry->m_10->m_60);
    if (commit != 0) {
        CSbiActiveObj* obj = g_gameReg->m_68;
        if (obj->ScrollProbe(col, row)) {
            obj->m_234 = col;
            obj->m_238 = row;
            obj->m_230 = 1;
            obj->LoadCameraSprite();
        }
    }
    return 1;
}

// Run the seven per-stat refresh updaters in sequence.
RVA(0x001058d0, 0x34)
void CSBI_RectOnly::RefreshAll() {
    UpdateGruntOvenStatusBar();
    TickGauge();
    UpdateRezConveyorStatusBar();
    LoadRezMachineConfig();
    LoadChipMachineConfig();
    UpdateChipGrinderStatusBar();
    UpdateDestructButtonStatusBar();
}

// Clear the gauge, zero m_gauge/m_gaugeTarget, run two updaters, set toggle flags.
RVA(0x00105920, 0x47)
void CSBI_RectOnly::Reset() {
    ResetSlots();
    m_gaugeTarget = 0;
    m_gauge = 0;
    ResetGroupA();
    UpdateRezMachineSnoozeStatusBar();
    RebuildGroupA();
    m_modeState = 1;
    m_destructWarnActive = 0;
}

// Toggle stat[idx]: if already set, clear it; else set it on.
RVA(0x00107aa0, 0x23)
void CSBI_RectOnly::ToggleStat(i32 idx) {
    if (m_statFlags[idx]) {
        ClearStatToggle(idx);
    } else {
        LoadStatzTabToggleSprite(idx, 1);
    }
}

// Find the index of the first enabled hit-test rect containing (x,y); -1 none.
// @early-stop
// bool-materialization wall: retail keeps the redundant inner `p->m_enabled` test and
// materializes the point-in-rect predicate via `mov ecx,1 / xor ecx,ecx / test`;
// MSVC5 folds our `&&` chain into direct control flow (~80%). Logic byte-correct.
RVA(0x00105280, 0x61)
i32 CSBI_RectOnly::HitTest(i32 x, i32 y) {
    if (m_hitTestDisabled == 0) {
        for (i32 i = 0; i < 15; i++) {
            CSbiRect* p = m_hitRects[i];
            if (p && p->m_enabled) {
                i32 hit =
                    p->m_enabled && x < p->m_xHi && x >= p->m_xLo && y < p->m_yHi && y >= p->m_yLo;
                if (hit) {
                    return i;
                }
            }
        }
    }
    return -1;
}

// Reset the three group-A slots (arm state=0/value=1) and notify each pointer.
RVA(0x00106610, 0x3b)
void CSBI_RectOnly::ResetGroupA() {
    for (i32 i = 0; i < 3; i++) {
        m_groupSlots[i].m_state = kSlotArmed;
        m_groupSlots[i].m_value = 1;
        if (m_groupNotify[i]) {
            m_groupNotify[i]->Notify(-1);
        }
    }
}

// Latch HUD-rect group A from three args + a global dword.
// @early-stop
// scheduling wall: logic byte-correct, but MSVC defers the m_hudRectA_x store and
// reorders the m_hudRectA_clockHi/m_hudRectA_clock zero+global pair vs retail; ~72%, not steerable from
// C (see docs/patterns/u64-store-clock-hi-zero.md, statement-schedule-faithful).
RVA(0x001066f0, 0x3b)
void CSBI_RectOnly::SetHudRectA(i32 y0, i32 x0, i32 z) {
    m_hudRectA_y = y0;
    m_hudRectA_x = x0;
    m_hudRectA_z = z;
    m_hudRectA_zHi = 0;
    m_hudRectA_clock = g_dat645588;
    m_hudRectA_clockHi = 0;
}

// Latch HUD-rect group B from three args + a global dword.
// @early-stop
// scheduling wall: same store-reorder as SetHudRectA; ~72%.
RVA(0x00106740, 0x3b)
void CSBI_RectOnly::SetHudRectB(i32 y0, i32 x0, i32 z) {
    m_hudRectB_y = y0;
    m_hudRectB_x = x0;
    m_hudRectB_z = z;
    m_hudRectB_zHi = 0;
    m_hudRectB_clock = g_dat645588;
    m_hudRectB_clockHi = 0;
}

// Commit the active slot: either re-arm it, or push the cooked commit level and
// notify its pointer; then clear the active-slot index.
RVA(0x00106790, 0x62)
void CSBI_RectOnly::CommitSlot(i32 active) {
    if (active) {
        ArmSlot(m_activeSlot);
        m_activeSlot = -1;
    } else {
        m_slots[m_activeSlot].m_value = kSlotCommitLevel;
        if (m_slotNotify[m_activeSlot]) {
            m_slotNotify[m_activeSlot]->Notify(m_slots[m_activeSlot].m_value);
        }
        m_activeSlot = -1;
    }
}

// Clear highlight-grid cell [row,group] (state + handle = 0), then re-notify.
RVA(0x001069c0, 0x2e)
void CSBI_RectOnly::ClearHlCell(i32 row, i32 group) {
    i32 idx = group + row * 4;
    m_hlGrid[idx].m_state = 0;
    m_hlGrid[idx].m_handle = 0;
    NotifyAllSlots();
}

// Set highlight-grid cell [row,group] handle if its slot is free; 1 = set, 0 = busy.
RVA(0x00106b40, 0x44)
i32 CSBI_RectOnly::SetHlCell(i32 row, i32 handle, i32 group) {
    i32 idx = group + row * 4;
    if (m_hlGrid[idx].m_state) {
        return 0;
    }
    m_hlGrid[idx].m_handle = handle;
    m_hlGrid[idx].m_state = 1;
    NotifyAllSlots();
    return 1;
}

// Fire every live slot's notifier: the four singletons, the 3x4 group grid
// (each pointer notified with its row's handle), then two trailing singletons.
RVA(0x00106a00, 0xbf)
void CSBI_RectOnly::NotifyAllSlots() {
    if (m_notify0) {
        m_notify0->v28();
    }
    if (m_notify2) {
        m_notify2->v28();
    }
    if (m_notify3) {
        m_notify3->v28();
    }
    if (m_extraNotify0 && m_extraNotifyArg0) {
        m_extraNotify0->Notify(m_extraNotifyArg0);
    }

    CSbiSlotPtr** p = &m_hlNotify[4]; // group B base; ±4 elements reach groups A / C
    i32* h = &m_hlGrid[4].m_handle;   // anchor on the handle field (+0x3dc)
    for (i32 n = 0; n < 4; n++) {
        if (p[-4]) {
            p[-4]->Notify(h[-24]); // -4 rows = -0x60 bytes = -24 ints
        }
        if (p[0]) {
            p[0]->Notify(h[0]);
        }
        if (p[4]) {
            p[4]->Notify(h[24]);
        }
        p++;
        h += 6; // one row = 0x18 bytes = 6 ints
    }

    if (m_notify1) {
        m_notify1->v28();
    }
    if (m_extraNotify1) {
        m_extraNotify1->Notify(m_extraNotifyArg1);
    }
}

// Stream the full rect-only item state through the archive (stream slot 0x30).
// Returns 0 if the stream or the active game-manager is null; bumps the global
// serialize counter; ends with a variable-length loop over the m_ptrTable[] pointer
// table (count m_ptrCount), each element streamed as 8 bytes. Field buffers are
// addressed by offset (the codegen is naming-independent here).
// @early-stop
// ~95.6%: the entire ~70-field transfer body is byte-exact; the residual is a
// regalloc/frame-size difference in the trailing 3x4 nested loop (retail pins
// the inner counter in ebp + reserves 1 stack dword via `push ecx`; the
// recompile spills the inner counter and reserves 3 via `sub esp,0xc`). Not
// steerable from C (docs/patterns regalloc/scheduling walls); deferred.
RVA(0x001090a0, 0x38f)
i32 CSBI_RectOnly::Serialize(CSbiStream* s) {
    if (s == 0) {
        return 0;
    }
    if (g_gameReg->m_world == 0) {
        return 0;
    }

    char* B = (char*)this;
    s->Transfer(B, 4); // offset 0 (vptr field)
    s->Transfer(B + 0x4, 4);

    g_serialCounter++;
    i32 seq = 0;
    // m_8 is the base CStatusBarItem `int` member, overlaid here as a pointer to
    // the sequence holder (retail reads it as a pointer and fetches +0x188).
    // Authentic int-as-pointer overlay; retyping it lives in the base class, not
    // here, so the cast stays (flagged in the report).
    if (m_8) {
        seq = ((CSbiSeqHolder*)m_8)->m_188;
    }
    s->Transfer(&seq, 4);

    s->Transfer(B + 0x10, 0x10);
    s->Transfer(B + 0x20, 4);
    s->Transfer(B + 0x24, 4);
    s->Transfer(B + 0x28, 4);
    s->Transfer(B + 0x110, 4);
    s->Transfer(B + 0x62c, 4);

    char* p = B + 0x114;
    for (i32 i = 0; i < 15; i++) {
        s->Transfer(p, 4);
        p += 4;
    }

    s->Transfer(B + 0x34c, 4);
    s->Transfer(B + 0x350, 4);
    s->Transfer(B + 0x354, 4);
    s->Transfer(B + 0x35c, 4);
    s->Transfer(B + 0x360, 4);
    s->Transfer(B + 0x10c, 4);
    s->Transfer(B + 0x298, 4);
    s->Transfer(B + 0x29c, 4);
    s->Transfer(B + 0x524, 4);
    s->Transfer(B + 0x52c, 4);
    s->Transfer(B + 0x528, 4);
    s->Transfer(B + 0x544, 4);
    s->Transfer(B + 0x504, 0x10);
    s->Transfer(B + 0x514, 0x10);
    s->Transfer(B + 0x548, 4);
    s->Transfer(B + 0x550, 4);
    s->Transfer(B + 0x554, 4);
    s->Transfer(B + 0x4c8, 4);
    s->Transfer(B + 0x4cc, 4);
    s->Transfer(B + 0x4e8, 4);
    s->Transfer(B + 0x4ec, 4);
    s->Transfer(B + 0x318, 4);
    s->Transfer(B + 0x31c, 4);
    s->Transfer(B + 0x330, 4);
    s->Transfer(B + 0x334, 4);
    s->Transfer(B + 0x558, 4);
    s->Transfer(B + 0x55c, 4);
    s->Transfer(B + 0x574, 4);
    s->Transfer(B + 0x578, 4);

    char* q = B + 0x224;
    for (i32 j = 0; j < 5; j++) {
        s->Transfer(q - 4, 4);
        s->Transfer(q, 4);
        q += 0x18;
    }
    char* r = B + 0x2c4;
    for (i32 k = 0; k < 3; k++) {
        s->Transfer(r - 4, 4);
        s->Transfer(r, 4);
        r += 0x18;
    }
    char* nb = B + 0x378;
    i32 outer = 3;
    do {
        for (i32 m = 0; m < 4; m++) {
            s->Transfer(nb, 4);
            s->Transfer(nb + 4, 4);
            nb += 0x18;
        }
    } while (--outer);

    i32 count = m_ptrCount;
    s->Transfer(&count, 4);
    for (u32 n = 0; n < (u32)count; n++) {
        s->Transfer(m_ptrTable[n], 8);
    }
    return 1;
}

// CSbiSeqMap/CSbiSeqObj moved to <Gruntz/SBI_RectOnly.h>.

// CSBI_RectOnly::Deserialize - the load/restore counterpart of Serialize. Pulls
// the full rect-only item state from the archive via stream slot 0x2c (Read);
// resolves the base m_8 sequence holder from the streamed seq id through the
// game-manager's object map (validated by the looked-up object's type tag == 5);
// reads every field back; returns each pooled m_ptrTable element to the engine
// free-list, sizes the +0x530 collection, then reloads the pointer table from the
// free-list. Field buffers are offset-addressed (naming-independent), mirroring
// Serialize.
// @early-stop
// twin of Serialize (95.6%, regalloc/frame wall in the trailing nested loop): the
// whole ~70-field Read body + the seq resolution + the free-list return/reload are
// reconstructed byte-faithfully, but the same trailing 3x4 nested-loop induction /
// frame-size regalloc choice (plus the free-list induction wall shared with
// Teardown/InsertPtr) caps it below 100%. Not steerable from C; deferred.
RVA(0x00109520, 0x44c)
i32 CSBI_RectOnly::Deserialize(CSbiStream* s) {
    if (s == 0) {
        return 0;
    }
    CSbiGameMgr* gm = g_gameReg->m_world;
    if (gm == 0) {
        return 0;
    }
    char* B = (char*)this;
    *(i32*)(B + 0x618) = 0;
    TeardownNotify(0);

    s->Read(B, 4);
    s->Read(B + 0x4, 4);

    g_serialCounter++;
    i32 seq = 0;
    s->Read(&seq, 4);

    void* obj = 0;
    CSbiSeqMap* map = (CSbiSeqMap*)(*(char**)((char*)gm + 8) + 0x48);
    i32 m8 = 0;
    if (map->Lookup(seq, &obj)) {
        if (obj != 0) {
            m8 = (((CSbiSeqObj*)obj)->TypeTag() == 5) ? (i32)obj : 0;
        }
    }
    m_8 = m8;
    if (m_8 == 0 && seq != 0) {
        return 0;
    }

    s->Read(B + 0x10, 0x10);
    s->Read(B + 0x20, 4);
    s->Read(B + 0x24, 4);
    s->Read(B + 0x28, 4);
    s->Read(B + 0x110, 4);
    s->Read(B + 0x62c, 4);

    char* p = B + 0x114;
    for (i32 i = 0; i < 15; i++) {
        s->Read(p, 4);
        p += 4;
    }

    s->Read(B + 0x34c, 4);
    s->Read(B + 0x350, 4);
    s->Read(B + 0x354, 4);
    s->Read(B + 0x35c, 4);
    s->Read(B + 0x360, 4);
    s->Read(B + 0x10c, 4);
    s->Read(B + 0x298, 4);
    s->Read(B + 0x29c, 4);
    s->Read(B + 0x524, 4);
    s->Read(B + 0x52c, 4);
    s->Read(B + 0x528, 4);
    s->Read(B + 0x544, 4);
    s->Read(B + 0x504, 0x10);
    s->Read(B + 0x514, 0x10);
    s->Read(B + 0x548, 4);
    s->Read(B + 0x550, 4);
    s->Read(B + 0x554, 4);
    s->Read(B + 0x4c8, 4);
    s->Read(B + 0x4cc, 4);
    s->Read(B + 0x4e8, 4);
    s->Read(B + 0x4ec, 4);
    s->Read(B + 0x318, 4);
    s->Read(B + 0x31c, 4);
    s->Read(B + 0x330, 4);
    s->Read(B + 0x334, 4);
    s->Read(B + 0x558, 4);
    s->Read(B + 0x55c, 4);
    s->Read(B + 0x574, 4);
    s->Read(B + 0x578, 4);

    char* q = B + 0x224;
    for (i32 j = 0; j < 5; j++) {
        s->Read(q - 4, 4);
        s->Read(q, 4);
        q += 0x18;
    }
    char* r = B + 0x2c4;
    for (i32 k = 0; k < 3; k++) {
        s->Read(r - 4, 4);
        s->Read(r, 4);
        r += 0x18;
    }
    char* nb = B + 0x378;
    i32 outer = 3;
    do {
        for (i32 m = 0; m < 4; m++) {
            s->Read(nb, 4);
            s->Read(nb + 4, 4);
            nb += 0x18;
        }
    } while (--outer);

    for (i32 t = 0; t < m_ptrCount; t++) {
        void* pp = m_ptrTable[t];
        if (pp) {
            void** node = (void**)((char*)pp - g_freeListNodeBias);
            *node = g_freeList;
            g_freeList = node;
        }
    }
    m_ptrPool.RemoveAll(0, -1);

    i32 count = 0;
    s->Read(&count, 4);
    m_ptrPool.RemoveAll(count, -1);
    for (u32 n = 0; n < (u32)count; n++) {
        char* head = (char*)g_freeList;
        void* node = 0;
        if (*(i32*)head != 0) {
            node = head + 4;
            g_freeList = *(void**)head;
        }
        s->Read(node, 8);
        m_ptrTable[n] = node;
    }
    return 1;
}

// Periodic highlight-cursor tick (RVA-ascending). Bail while the suppress flag
// (m_hlBusy) is set or the game is over (g_gameReg->m_134==1). If this is the
// subtype-2 cursor item, refresh its state first. When the active tab is not the
// gauge tab (4), latch it inactive (SetTabState(4,3)) and Deactivate; otherwise
// advance the 4-state cursor (forward wraps 4->0; the `reverse` path only guards
// the signed-overflow wrap), then refresh the widgets and re-arm.
RVA(0x0010b4f0, 0xaa)
void CSBI_RectOnly::AdvanceTab(i32 reverse) {
    if (m_hlBusy != 0) {
        return;
    }
    if (g_gameReg->m_134 == 1) {
        return;
    }
    if (*(i32*)this == kSubtypeTag) {
        RefreshState();
    }
    if (m_activeTab != 4) {
        SetTabState(4, 3);
        Deactivate();
        return;
    }
    if (reverse != 0) {
        if (++m_tabCycle < 0) {
            m_tabCycle = 3;
        }
    } else {
        if (++m_tabCycle >= 4) {
            m_tabCycle = 0;
        }
    }
    ResetWidgets(0);
    TryActivate();
    Deactivate();
}

// Highlight-click handler for group 0 (rows m_hlGrid[0..3]): bail if the busy
// gate is set or the row is not armed (state==1); validate the row's handle, play
// the GAME_TABHIGHLIGHT1 cue on the draw-clock window if the music gate is free,
// then latch the pending row, clear the handle, and re-notify. Single-exit nesting
// (success-deepest, one trailing `return 0`) reproduces retail's shared FAIL tail
// (docs/patterns/nested-if-success-deepest-error-tail.md); the global pair must be
// read TOGETHER inside the `if (found)` so MSVC lands g_61ab20 in ecx / g_61ab24 in
// edx like retail.
// @early-stop
// ~94.2%: code bytes are byte-exact vs retail; the residual is purely the
// reloc-symbol-naming scoring tail (DIR32 to differently-named globals/the cue
// string vs retail's REL32). Not a wall - matcher.md reloc-typing artifact.
RVA(0x0010b5d0, 0xdd)
i32 CSBI_RectOnly::HlClickGroup0(i32 row) {
    if (g_gameReg->m_curState->m_4f0 == 0 && m_hlGrid[row].m_state == 1) {
        i32 handle = m_hlGrid[row].m_handle;
        i32* slot = &m_hlGrid[row].m_handle;
        if (ResolveHandle(handle)) {
            CSbiMusicHost* host = g_gameReg->m_world->m_28;
            if (host->m_30 == 0) {
                void* found = 0;
                CSbiLookupMap* map = &host->m_map10;
                map->Lookup("GAME_TABHIGHLIGHT1", &found);
                if (found) {
                    i32 gate = g_61ab20;
                    i32 item = g_61ab24;
                    if (gate != 0) {
                        CSbiCueRecord* p = (CSbiCueRecord*)found;
                        if (g_6bf3c0 - (u32)p->m_14 >= (u32)p->m_18) {
                            p->m_14 = g_6bf3c0;
                            p->m_10->ConfigureItem(item, 0, 0, 0);
                        }
                    }
                }
            }
            m_pendingHlRow = row;
            *slot = 0;
            NotifyAllSlots();
            return 1;
        }
    }
    return 0;
}

// Highlight-click handler for group 1 (rows m_hlGrid[4..7]); identical to group 0
// with the +4-row base folded into the index.
// @early-stop
// ~94.2%: code byte-exact; reloc-symbol-naming tail only (see HlClickGroup0).
RVA(0x0010b6f0, 0xdd)
i32 CSBI_RectOnly::HlClickGroup1(i32 row) {
    if (g_gameReg->m_curState->m_4f0 == 0 && m_hlGrid[row + 4].m_state == 1) {
        i32 handle = m_hlGrid[row + 4].m_handle;
        i32* slot = &m_hlGrid[row + 4].m_handle;
        if (ResolveHandle(handle)) {
            CSbiMusicHost* host = g_gameReg->m_world->m_28;
            if (host->m_30 == 0) {
                void* found = 0;
                CSbiLookupMap* map = &host->m_map10;
                map->Lookup("GAME_TABHIGHLIGHT1", &found);
                if (found) {
                    i32 gate = g_61ab20;
                    i32 item = g_61ab24;
                    if (gate != 0) {
                        CSbiCueRecord* p = (CSbiCueRecord*)found;
                        if (g_6bf3c0 - (u32)p->m_14 >= (u32)p->m_18) {
                            p->m_14 = g_6bf3c0;
                            p->m_10->ConfigureItem(item, 0, 0, 0);
                        }
                    }
                }
            }
            m_pendingHlRow = row;
            *slot = 0;
            NotifyAllSlots();
            return 1;
        }
    }
    return 0;
}

// Highlight-click handler for group 2 (rows m_hlGrid[8..11]); +8-row base.
// @early-stop
// ~94.2%: code byte-exact; reloc-symbol-naming tail only (see HlClickGroup0).
RVA(0x0010b810, 0xdd)
i32 CSBI_RectOnly::HlClickGroup2(i32 row) {
    if (g_gameReg->m_curState->m_4f0 == 0 && m_hlGrid[row + 8].m_state == 1) {
        i32 handle = m_hlGrid[row + 8].m_handle;
        i32* slot = &m_hlGrid[row + 8].m_handle;
        if (ResolveHandle(handle)) {
            CSbiMusicHost* host = g_gameReg->m_world->m_28;
            if (host->m_30 == 0) {
                void* found = 0;
                CSbiLookupMap* map = &host->m_map10;
                map->Lookup("GAME_TABHIGHLIGHT1", &found);
                if (found) {
                    i32 gate = g_61ab20;
                    i32 item = g_61ab24;
                    if (gate != 0) {
                        CSbiCueRecord* p = (CSbiCueRecord*)found;
                        if (g_6bf3c0 - (u32)p->m_14 >= (u32)p->m_18) {
                            p->m_14 = g_6bf3c0;
                            p->m_10->ConfigureItem(item, 0, 0, 0);
                        }
                    }
                }
            }
            m_pendingHlRow = row;
            *slot = 0;
            NotifyAllSlots();
            return 1;
        }
    }
    return 0;
}

// Step the gauge one unit toward its target; on reaching 100 fire the completion
// hook; if the gauge moved, refresh the two gauge notifiers (the sink latches the
// new reading into its m_44). The toward-target step is spelled as `<`/`<=goto`/`>`
// (no outer `!=`) so the equal case folds into the jge/jle pair like retail and the
// two inc/dec branches share one store, not an extra top `je`.
// @early-stop
// ~95.7%: byte-exact except a 1-instr regalloc coin-flip in the m_gaugeSink->m_44 store
// block (retail keeps the gauge value in eax + loads the vtable into edx; the
// recompile uses edx for the value). Not steerable from C; deferred.
RVA(0x00105480, 0x7d)
void CSBI_RectOnly::TickGauge() {
    i32 changed = 0;
    i32 g = m_gauge;
    i32 t = m_gaugeTarget;
    if (g < t) {
        g++;
    } else if (g <= t) {
        goto noChange;
    } else {
        g--;
    }
    m_gauge = g;
    changed = 1;
noChange:;
    if (m_gauge == 100) {
        if (GaugeComplete()) {
            changed = 1;
            GaugeFinish(0);
        }
    }
    if (changed) {
        if (m_gaugeSink && m_gaugeNotify) {
            m_gaugeNotify->v28();
            m_gaugeSink->m_44 = m_gauge;
            m_gaugeSink->v28();
        }
    }
}

// Find the first slot whose state is ready; re-arm it and report found.
RVA(0x00109a90, 0x25)
i32 CSBI_RectOnly::FindReadySlot() {
    for (i32 i = 0; i < 5; i++) {
        if (m_slots[i].m_state == kSlotReady) {
            ArmSlot(i);
            return 1;
        }
    }
    return 0;
}

// Release the per-tab sprite widgets for the given tab group (idx, with -1 = all).
RVA(0x00101420, 0x110)
i32 CSBI_RectOnly::ClearTabSprites(i32 idx) {
    if (idx == -1 || idx == 0) {
        if (m_tabSprite0) {
            m_tabSprite0->Release();
        }
        if (m_tabSprite2) {
            m_tabSprite2->Release();
        }
        if (m_tabSprite1) {
            m_tabSprite1->Release();
        }
        if (m_tabSprite3) {
            m_tabSprite3->Release();
        }
        if (m_tabSprite4) {
            m_tabSprite4->Release();
        }
    }
    if (idx == 5 || idx == -1) {
        if (m_tabSprite5) {
            m_tabSprite5->Release();
        }
        if (m_tabSprite6) {
            m_tabSprite6->Release();
        }
        if (m_tabSprite7) {
            m_tabSprite7->Release();
        }
        if (m_tabSprite8) {
            m_tabSprite8->Release();
        }
        if (m_tabSprite9) {
            m_tabSprite9->Release();
        }
        if (m_tabSprite10) {
            m_tabSprite10->Release();
        }
    }
    if (idx == 6 || idx == -1) {
        if (m_tabSprite11) {
            m_tabSprite11->Release();
        }
        if (m_tabSprite12) {
            m_tabSprite12->Release();
        }
        if (m_tabSprite13) {
            m_tabSprite13->Release();
        }
        if (m_tabSprite14) {
            m_tabSprite14->Release();
        }
    }
    return 1;
}

// Deactivate the rect-only item: if it is the subtype-2 cursor item, latch the
// game-reg cursor rect; then fire the deactivate notifier across list[0] and the
// active-tab list, clear, and tag the state. The +0x30 list array (0x1c stride,
// head at +0) is offset-addressed: the typed array view would overlap the
// matched m_30/m_34 ints, so the raw access is the codegen-faithful model here.
// @early-stop
// ~97.2%: the two notify-list walks + teardown are byte-exact; the residual is a
// 3-instr regalloc choice in the cursor-rect prologue (retail loads g_gameReg via
// `mov eax,moffs` and computes the two `-0x45/-0x30` offsets with non-destructive
// `lea` into fresh regs; the recompile loads into ecx and uses `sub` in place).
// Not steerable from C - logic byte-correct; deferred to the final sweep.
RVA(0x00100cb0, 0x8b)
i32 CSBI_RectOnly::Deactivate() {
    if (*(i32*)this == kSubtypeTag) {
        CGameReg* g = g_gameReg;
        i32 a = g->m_modeW;
        i32 b = g->m_modeH;
        i32 x = a - 0x45;
        i32 y = b - 0x30;
        m_28 = y;
        m_24 = x;
        SetCursorRect(x, y);
    }

    char* B = (char*)this;
    CSbiNotifyNode* n = *(CSbiNotifyNode**)(B + 0x30);
    while (n) {
        CSbiNotifyNode* cur = n;
        n = n->m_next;
        if (cur->m_payload) {
            ((CSbiSlotPtr*)cur->m_payload)->v28();
        }
    }

    CSbiNotifyNode* m = *(CSbiNotifyNode**)(B + m_activeTab * 0x1c + 0x30);
    while (m) {
        CSbiNotifyNode* cur = m;
        m = m->m_next;
        if (cur->m_payload) {
            ((CSbiSlotPtr*)cur->m_payload)->v28();
        }
    }

    ClearTabSprites(-1);
    *(i32*)(B + 0x20) = 2;
    return 1;
}

// Switch the active tab: short-circuit if already on it (and not forced);
// otherwise notify the per-tab list, clear it, drop the tab-5 sprites, latch the
// new tab, and re-probe. On probe failure report the error and bail.
RVA(0x001020a0, 0xae)
i32 CSBI_RectOnly::SetTab(i32 tab, i32 flag) {
    if (tab == m_activeTab && flag == 0) {
        return 1;
    }
    CSbiNotifyNode* n = m_listB8.m_head;
    while (n) {
        CSbiNotifyNode* cur = n;
        n = n->m_next;
        if (cur->m_payload) {
            cur->m_payload->Notify(1);
        }
    }
    m_listB8.RemoveAll();
    m_tabSprite5 = 0;
    m_tabSprite6 = 0;
    m_tabSprite7 = 0;
    m_tabSprite8 = 0;
    m_tabSprite9 = 0;
    m_tabSprite10 = 0;
    m_activeTab = tab;
    if (!TabRefresh()) {
        g_gameReg->ReportError(kActivateErrId, kSetTabErrTag);
        return 0;
    }
    TabCommit();
    return 1;
}

// Tear down the rect-only item: log its position tag, fire the pre-teardown
// notify, return every pooled m_ptrTable element to the engine free-list, then
// RemoveAll the +0x530 collection.
// @early-stop
// ~84.7%: the log/notify head + the free-list return + the +0x530 RemoveAll are
// byte-correct; the residual is the free-loop's induction-variable choice (retail
// indexes m_ptrTable[ecx] reloading the base each iter and keeps the running
// free-list head in edx; the recompile strength-reduces to a walking pointer +
// pins the head in a callee-saved edi). Regalloc/induction wall; deferred.
RVA(0x000fe350, 0x6d)
void CSBI_RectOnly::Teardown() {
    g_gameReg->m_38->LogPos("StatusBar Position", *(i32*)this);
    TeardownNotify(0);
    for (i32 i = 0; i < m_ptrCount; i++) {
        void* p = m_ptrTable[i];
        if (p) {
            void** node = (void**)((char*)p - g_freeListNodeBias);
            *node = g_freeList;
            g_freeList = node;
        }
    }
    m_ptrPool.RemoveAll(0, -1);
}

// Activate the rect-only item; gate on the offset-0 subtype tag and a probe.
RVA(0x00104d60, 0x48)
i32 CSBI_RectOnly::TryActivate() {
    // Offset-0 read: in retail this object's slot 0 holds a small integer
    // subtype tag (the manual-vtable-stamp tag device shared with CStatusBarMgr),
    // not a real C++ vptr. We model the vtable via `virtual`,
    // so slot 0 cannot also be a named field here without dropping the vtable;
    // the raw `*(int*)this` read is the faithful model. See report (flagged).
    if (*(i32*)this == kSubtypeTag) {
        return Activate();
    }
    if (!Stub_0ffde0_probe()) {
        g_gameReg->ReportError(kActivateErrId, kActivateErrTag);
        return 0;
    }
    SetTabState(m_activeTab, 3);
    return 1;
}

// Enter mode: latch m_modeArmed, conditionally reset the toggle pair, notify m_modeNotify.
RVA(0x0010bb90, 0x3f)
void CSBI_RectOnly::SetMode(i32 mode) {
    m_modeArmed = 1;
    if (mode && m_modeState != 7) {
        m_destructWarnActive = 0;
        m_modeState = 1;
        if (m_modeNotify) {
            m_modeNotify->Notify(1);
        }
    }
}

// Find the rect widget under (x,y) by walking the three hit-test lists (the +0x30
// list, the active-tab list at +tab*0x1c+0x30, then the +0xd8 list); return the
// first enabled rect whose span contains the point, else null. Same point-in-rect
// predicate as HitTest, materialized to a bool per retail.
RVA(0x000ffcb0, 0xe2)
CSbiRect* CSBI_RectOnly::HitTestRects(i32 x, i32 y) {
    char* B = (char*)this;
    CSbiNotifyNode* n = *(CSbiNotifyNode**)(B + 0x30);
    while (n) {
        CSbiRect* r = (CSbiRect*)n->m_payload;
        n = n->m_next;
        if (r && r->m_enabled) {
            i32 hit = x < r->m_xHi && x >= r->m_xLo && y < r->m_yHi && y >= r->m_yLo;
            if (hit) {
                return r;
            }
        }
    }
    CSbiNotifyNode* m = *(CSbiNotifyNode**)(B + m_activeTab * 0x1c + 0x30);
    while (m) {
        CSbiRect* r = (CSbiRect*)m->m_payload;
        m = m->m_next;
        if (r && r->m_enabled) {
            i32 hit = x < r->m_xHi && x >= r->m_xLo && y < r->m_yHi && y >= r->m_yLo;
            if (hit) {
                return r;
            }
        }
    }
    CSbiNotifyNode* k = m_listD4.m_head;
    while (k) {
        CSbiRect* r = (CSbiRect*)k->m_payload;
        k = k->m_next;
        if (r && r->m_enabled) {
            i32 hit = x < r->m_xHi && x >= r->m_xLo && y < r->m_yHi && y >= r->m_yLo;
            if (hit) {
                return r;
            }
        }
    }
    return 0;
}

// Initialize the two HUD rects (+0x504, +0x514) via Win32 SetRect through its IAT
// thunk (CSE'd into one load), clear the four highlight-grid groups, latch flags,
// reset the pending-row index.
RVA(0x00106900, 0x8d)
void CSBI_RectOnly::InitTabRects() {
    for (i32 i = 0; i < 4; i++) {
        ClearHlCell(0, i);
        ClearHlCell(1, i);
        ClearHlCell(2, i);
    }
    m_machinePhase = 1;
    m_extraNotifyArg0 = 0;
    *(i32*)((char*)this + 0x4e8) = 0;
    m_extraNotifyArg1 = 0;
    SetRect((LPRECT)((char*)this + 0x504), 0, 0, 1, 1);
    SetRect((LPRECT)((char*)this + 0x514), 0x49, 0xd7, 0x61, 0xef);
    m_pendingHlRow = -1;
}

// Group-A click handler: hit-test the lists; if no rect, drop the tab sprites.
// Otherwise dispatch the rect's slot-9 click; if it is a kind-2 (subtype) widget
// and no hit-test is disabled, play the GAME_TABHIGHLIGHT1 cue on the draw-clock
// window, then forward the offset command id.
RVA(0x000ff9f0, 0xe4)
i32 CSBI_RectOnly::ClickToggle(i32 x, i32 y, i32 z) {
    CSbiRect* r = HitTestRects(x, y);
    if (r == 0) {
        ClearTabSprites(-1);
        return 1;
    }
    r->Click24(z, x, y);
    if (r->m_kind != 2) {
        ClearTabSprites(-1);
        return 1;
    }
    i32 cmd = r->m_cmd;
    if (m_hitTestDisabled == 0) {
        if (cmd >= 1 && cmd <= 5) {
            SetTabState(cmd, 2);
        } else {
            ClearTabSprites(0);
        }
    }
    if (m_activeTab == 5) {
        if (r->m_tab == 5) {
            SetTabState(cmd, 2);
        } else {
            ClearTabSprites(5);
        }
    }
    if (m_toggleActive) {
        if (r->m_tab == 6) {
            SetTabState(cmd, 2);
            return 1;
        }
        ClearTabSprites(5);
    }
    return 1;
}

// CSbiResetHost moved to <Gruntz/SBI_RectOnly.h>.

// Reset every per-tab widget list (8 notify lists at +0x2c, stride 0x1c) - notify
// each payload, then RemoveAll - then (when keepHost is set) flag the +0x8 host as
// aborted, and finally zero the whole widget/notify field block.
// @early-stop
// ~78%: instruction selection + scheduling are byte-identical end to end (the
// 8-list walk, the host flag-OR, every field zero, both rep-stos memsets), but
// MSVC pins `this` in ebx / the zero in ebp where the recompile uses esi/ebx -
// the same regalloc register-naming coin-flip as ClearTabGroup. Not steerable
// from C; documented regalloc wall, deferred to the final sweep.
RVA(0x00100930, 0x16c)
void CSBI_RectOnly::ResetWidgets(i32 keepHost) {
    char* B = (char*)this;
    char* list = B + 0x2c;
    for (i32 outer = 8; outer != 0; outer--) {
        CSbiNotifyNode* n = *(CSbiNotifyNode**)(list + 4);
        while (n) {
            CSbiNotifyNode* cur = n;
            n = n->m_next;
            if (cur->m_payload) {
                cur->m_payload->Notify(1);
            }
        }
        ((CSbiPtrList*)list)->RemoveAll();
        list += 0x1c;
    }
    if (keepHost) {
        CSbiResetHost* h = *(CSbiResetHost**)(B + 8);
        if (h) {
            h->m_40 |= 1;
            h = *(CSbiResetHost**)(B + 8);
            h->m_8 |= 0x10000;
        }
    }
    m_tabSprite0 = 0;
    m_tabSprite1 = 0;
    m_tabSprite2 = 0;
    m_tabSprite3 = 0;
    m_tabSprite4 = 0;
    m_tabSprite5 = 0;
    m_tabSprite6 = 0;
    m_tabSprite7 = 0;
    m_tabSprite8 = 0;
    m_tabSprite9 = 0;
    m_tabSprite10 = 0;
    m_tabSprite11 = 0;
    m_tabSprite12 = 0;
    m_tabSprite13 = 0;
    m_tabSprite14 = 0;
    m_8 = 0;
    i32 i;
    for (i = 0; i < 15; i++) {
        m_hitRects[i] = 0;
    }
    for (i = 0; i < 15; i++) {
        m_statObj[i] = 0;
    }
    i32* sp = (i32*)(B + 0x204);
    sp[0] = 0;
    sp[1] = 0;
    sp[2] = 0;
    sp[3] = 0;
    sp[4] = 0;
    i32* gp = (i32*)(B + 0x308);
    gp[0] = 0;
    gp[1] = 0;
    gp[2] = 0;
    for (i = 0; i < 12; i++) {
        m_hlNotify[i] = 0;
    }
    i32* tp = m_61c;
    tp[0] = 0;
    tp[1] = 0;
    tp[2] = 0;
    tp[3] = 0;
    m_extraNotify0 = 0;
    m_extraNotify1 = 0;
    m_modeNotify = 0;
    m_notify0 = 0;
    m_notify2 = 0;
    m_notify3 = 0;
    m_notify1 = 0;
    *(i32*)(B + 0x348) = 0;
    m_gaugeNotify = 0;
    m_gaugeSink = 0;
    *(i32*)(B + 0x358) = 0;
}

// Exit the alternate (toggle) mode: bail if not active; notify+clear the +0xd4
// list, drop the trailing sprites + the +0x548 flag, then either re-arm the
// active tab (when no handle is pending and the game is not over) or just clear
// the hit-test flag; finish through Deactivate.
RVA(0x0010b210, 0xc5)
void CSBI_RectOnly::ExitMode() {
    if (m_toggleActive == 0) {
        return;
    }
    CSbiNotifyNode* n = m_listD4.m_head;
    while (n) {
        CSbiNotifyNode* cur = n;
        n = n->m_next;
        if (cur->m_payload) {
            cur->m_payload->Notify(1);
        }
    }
    m_listD4.RemoveAll();
    i32 handle = m_toggleHandle;
    m_tabSprite11 = 0;
    m_tabSprite12 = 0;
    m_tabSprite13 = 0;
    m_tabSprite14 = 0;
    m_hlBusy = 0;
    if (handle == 0 && g_gameReg->m_134 != 1) {
        if (*(i32*)this == 2) {
            TabRefresh();
        }
        if (m_activeTab != 5) {
            SetTabState(5, 3);
        }
        SetTab(5, 1);
        Deactivate();
    } else {
        m_hitTestDisabled = 0;
    }
    m_toggleActive = 0;
    m_toggleHandle = 0;
    Deactivate();
}

// Tear down the widgets owned by the active tab: notify+RemoveAll the active-tab
// list, then (switch on the tab index) zero the per-tab field group. Bails when no
// tab is active or the tab index is out of [1,5].
// @early-stop
// ~67%: instruction selection + scheduling are byte-identical (the list walk, the
// RemoveAll, the jump-table dispatch, every per-case field zero), but MSVC pins
// `this` in ebx and the zero constant in ebp (5 callee-saved pushes) where the
// recompile uses esi/ebx (4 pushes) - a regalloc register-naming coin-flip not
// steerable from C. Documented regalloc wall; deferred to the final sweep.
RVA(0x00100b00, 0x139)
void CSBI_RectOnly::ClearTabGroup() {
    char* B = (char*)this;
    if (m_activeTab == 0) {
        return;
    }
    CSbiNotifyNode* n = *(CSbiNotifyNode**)(B + m_activeTab * 0x1c + 0x30);
    while (n) {
        CSbiNotifyNode* cur = n;
        n = n->m_next;
        if (cur->m_payload) {
            cur->m_payload->Notify(1);
        }
    }
    ((CSbiPtrList*)(B + m_activeTab * 0x1c + 0x2c))->RemoveAll();
    switch (m_activeTab) {
        case 1:
            m_tabSprite5 = 0;
            m_tabSprite6 = 0;
            m_tabSprite7 = 0;
            m_tabSprite8 = 0;
            m_tabSprite9 = 0;
            m_tabSprite10 = 0;
            m_modeNotify = 0;
            break;
        case 2: {
            for (i32 i = 0; i < 15; i++) {
                m_statObj[i] = 0;
            }
            break;
        }
        case 3: {
            i32* p = m_61c;
            p[0] = 0;
            p[1] = 0;
            p[2] = 0;
            p[3] = 0;
            break;
        }
        case 4: {
            i32* p = (i32*)(B + 0x204);
            p[0] = 0;
            p[1] = 0;
            p[2] = 0;
            p[3] = 0;
            p[4] = 0;
            m_gaugeNotify = 0;
            m_gaugeSink = 0;
            break;
        }
        case 5: {
            i32* p = (i32*)(B + 0x308);
            p[0] = 0;
            p[1] = 0;
            p[2] = 0;
            *(i32*)(B + 0x348) = 0;
            for (i32 i = 0; i < 12; i++) {
                m_hlNotify[i] = 0;
            }
            m_notify0 = 0;
            m_notify2 = 0;
            m_notify3 = 0;
            m_notify1 = 0;
            m_extraNotify0 = 0;
            m_extraNotify1 = 0;
            break;
        }
    }
}

// Configure a rect-only item from a descriptor: bail unless both the host and the
// subtype are valid; latch the descriptor fields (subtype, command, the four rect
// coords, the rect-test command), then look the cue key up in the host's map and,
// if found, resolve the start frame from the cue's frame range/table.
// @early-stop
// ~62%: the gate, every field latch (same statement order as retail), the map
// lookup and the frame-range resolution are byte-correct in instruction selection,
// but retail stages the four rect coords through a reused `edx = this+0x14`
// pointer where the recompile addresses them as direct `[this+0x14..]` offsets,
// and the surrounding store schedule + register naming diverge accordingly. An
// addressing-mode/scheduling wall, not steerable from C; deferred.
RVA(0x000e72f0, 0xc4)
i32 CSBI_RectOnly::ConfigureRect(
    i32 sub,
    CSbiConfigHost* host,
    i32 cmd,
    i32 obj,
    i32 r0,
    i32 r1,
    i32 r2,
    i32 r3,
    i32 key,
    i32 frame,
    i32 extra
) {
    (void)extra;
    char* B = (char*)this;
    if (host == 0 || sub == 0) {
        return 0;
    }
    *(i32*)(B + 0x2c) = sub;
    *(i32*)(B + 0x10) = obj;
    i32* rc = (i32*)(B + 0x14);
    m_24 = (i32)host;
    m_28 = 0;
    m_4 = 1;
    rc[0] = r0;
    rc[1] = r1;
    rc[2] = r2;
    rc[3] = r3;
    *(i32*)(B + 0xc) = cmd;
    if (key == 0) {
        return 0;
    }
    CSbiConfigRecord* rec = 0;
    host->m_10->m_10map.Lookup(key, &rec);
    m_34 = (i32)rec;
    if (rec == 0) {
        return 0;
    }
    i32 f = frame;
    if (f == -1) {
        f = rec->m_64;
    }
    *(i32*)(B + 0x38) = f;
    if (f >= rec->m_64 && f <= rec->m_68) {
        m_30 = rec->m_14[f];
    } else {
        m_30 = 0;
    }
    return 1;
}

// Drive the tab-selection sprites: require the five bank-A sprites all present,
// then switch on the tab id to Show the selected sprite (with flag) and Hide the
// rest, gated per case by m_hlBusy (busy => early-return 1). Banks A (1-5), B
// (0x1f4-0x1fa), C (0x324/0x325), D (0x327/0x328). `state` is the sprite index arg.
// @early-stop
// ~85.5% (1305 B): the 5-sprite guard, both jump-table dispatches and every
// Show/Hide sequence are byte-correct; the residual is the shared-tail merging
// retail performs across adjacent cases (the `jmp` convergence at the m_tabSprite3/m_tabSprite4
// and bank-B tails) which the recompile duplicates per case instead. A
// scheduling/tail-merge wall on a big switch; not steerable from C. Deferred.
RVA(0x00100d70, 0x519)
i32 CSBI_RectOnly::SetTabState(i32 tab, i32 state) {
    if (m_tabSprite0 == 0 || m_tabSprite1 == 0 || m_tabSprite2 == 0 || m_tabSprite3 == 0
        || m_tabSprite4 == 0) {
        return 0;
    }
    switch (tab) {
        case 1:
            if (m_hlBusy) {
                return 1;
            }
            m_tabSprite0->Show(state, 1);
            m_tabSprite2->Hide(state);
            m_tabSprite1->Hide(state);
            m_tabSprite3->Hide(state);
            m_tabSprite4->Hide(state);
            return 1;
        case 2:
            if (m_hlBusy) {
                return 1;
            }
            m_tabSprite0->Hide(state);
            m_tabSprite2->Show(state, 1);
            m_tabSprite1->Hide(state);
            m_tabSprite3->Hide(state);
            m_tabSprite4->Hide(state);
            return 1;
        case 3:
            if (m_hlBusy) {
                return 1;
            }
            m_tabSprite0->Hide(state);
            m_tabSprite2->Hide(state);
            m_tabSprite1->Show(state, 1);
            m_tabSprite3->Hide(state);
            m_tabSprite4->Hide(state);
            return 1;
        case 4:
            if (m_hlBusy) {
                return 1;
            }
            m_tabSprite0->Hide(state);
            m_tabSprite2->Hide(state);
            m_tabSprite1->Hide(state);
            m_tabSprite3->Show(state, 1);
            m_tabSprite4->Hide(state);
            return 1;
        case 5:
            if (m_hlBusy) {
                return 1;
            }
            m_tabSprite0->Hide(state);
            m_tabSprite2->Hide(state);
            m_tabSprite1->Hide(state);
            m_tabSprite3->Hide(state);
            m_tabSprite4->Show(state, 1);
            return 1;
        case 0x1f4:
            if (m_hlBusy) {
                return 1;
            }
            m_tabSprite5->Show(state, 1);
            m_tabSprite6->Hide(state);
            m_tabSprite7->Hide(state);
            m_tabSprite8->Hide(state);
            m_tabSprite9->Hide(state);
            m_tabSprite10->Hide(state);
            return 1;
        case 0x1f5:
            if (m_hlBusy) {
                return 1;
            }
            m_tabSprite5->Hide(state);
            m_tabSprite6->Show(state, 1);
            m_tabSprite7->Hide(state);
            m_tabSprite8->Hide(state);
            m_tabSprite9->Hide(state);
            m_tabSprite10->Hide(state);
            return 1;
        case 0x1f6:
            if (m_hlBusy) {
                return 1;
            }
            m_tabSprite5->Hide(state);
            m_tabSprite6->Hide(state);
            m_tabSprite7->Show(state, 1);
            m_tabSprite8->Hide(state);
            m_tabSprite9->Hide(state);
            m_tabSprite10->Hide(state);
            return 1;
        case 0x1f7:
            if (m_hlBusy) {
                return 1;
            }
            m_tabSprite5->Hide(state);
            m_tabSprite6->Hide(state);
            m_tabSprite7->Hide(state);
            m_tabSprite8->Show(state, 1);
            m_tabSprite9->Hide(state);
            m_tabSprite10->Hide(state);
            return 1;
        case 0x1f8:
            if (m_hlBusy) {
                return 1;
            }
            m_tabSprite5->Hide(state);
            m_tabSprite6->Hide(state);
            m_tabSprite7->Hide(state);
            m_tabSprite8->Hide(state);
            m_tabSprite9->Show(state, 1);
            m_tabSprite10->Hide(state);
            return 1;
        case 0x1f9:
            if (m_hlBusy) {
                return 1;
            }
            m_tabSprite5->Hide(state);
            m_tabSprite6->Hide(state);
            m_tabSprite7->Hide(state);
            m_tabSprite8->Hide(state);
            m_tabSprite9->Hide(state);
            m_tabSprite10->Show(state, 1);
            return 1;
        case 0x1fa:
            if (m_hlBusy) {
                return 1;
            }
            m_tabSprite10->Show(state, 1);
            return 1;
        case 0x324:
            if (m_tabSprite11) {
                m_tabSprite11->Show(state, 1);
            }
            m_tabSprite12->Hide(state);
            return 1;
        case 0x325:
            if (m_tabSprite11) {
                m_tabSprite11->Hide(state);
            }
            m_tabSprite12->Show(state, 1);
            return 1;
        case 0x327:
            m_tabSprite13->Show(state, 1);
            m_tabSprite14->Hide(state);
            return 1;
        case 0x328:
            m_tabSprite13->Hide(state);
            m_tabSprite14->Show(state, 1);
            return 1;
    }
    return 1;
}

// Enter a highlight handle at the pending row m_pendingHlRow: pick the group from arg1's
// range, then either (arg0 != 0) clear the group's row m_pendingHlRow and shift every set
// row below it down by one (inserting at the top), or (arg0 == 0) just latch the
// handle into row m_pendingHlRow's cell. Always re-notify and reset m_pendingHlRow.
RVA(0x00106820, 0xa8)
void CSBI_RectOnly::EnterHlRow(i32 shift, i32 key) {
    if (m_pendingHlRow == -1) {
        return;
    }
    i32 group;
    if (key >= 0x22) {
        group = 2;
    } else {
        group = (key >= 0x17);
    }
    if (shift != 0) {
        ClearHlCell(group, m_pendingHlRow);
        for (i32 row = m_pendingHlRow - 1; row >= 0; row--) {
            CSbiHlRow* cell = &m_hlGrid[row + group * 4];
            if (cell->m_state == 1) {
                m_hlGrid[row + group * 4 + 1].m_state = 1;
                cell[1].m_handle = cell->m_handle;
                cell->m_state = 0;
                cell->m_handle = 0;
            }
        }
    } else {
        m_hlGrid[m_pendingHlRow + group * 4].m_handle = key;
    }
    NotifyAllSlots();
    m_pendingHlRow = -1;
}

// Group-A highlight click: hit-test the rect under (x,y); dispatch its slot-7
// click; then, only for a tab-1 widget whose command is in the highlight range
// and when nothing blocks it, play the GAME_TABHIGHLIGHT1 cue + activate the
// cursor for the offset command. The fallback (any gate fails) drives the tab
// highlight directly. Single-exit, success-deepest (shared fallback tail).
// @early-stop
// ~89%: the whole hit-test / dispatch / gate chain / cue play / cursor activation
// is byte-correct; the residual is two documented walls - the bare `return 1`
// null-path tail-merges with the other identical `return 1` epilogues where retail
// duplicates them inline (identical-return-epilogue-tailmerge), and retail tests
// the tab via `dec; jne` where the recompile uses `cmp 1; jne`. Logic correct,
// deferred to the final sweep.
RVA(0x000ff850, 0x121)
i32 CSBI_RectOnly::ClickHilite(i32 a, i32 x, i32 y) {
    CSbiRect* r = HitTestRects(x, y);
    if (r == 0) {
        return 1;
    }
    r->Click1c(a, x, y);
    i32 cmd = r->m_cmd;
    if (r->m_tab == 1 && m_hitTestDisabled == 0 && g_gameReg->m_68->m_400 != 0 && cmd >= 0x13b
        && cmd <= 0x149) {
        CSbiMusicHost* host = g_gameReg->m_world->m_28;
        if (host->m_30 == 0) {
            void* found = 0;
            CSbiLookupMap* map = &host->m_map10;
            map->Lookup("GAME_TABHIGHLIGHT1", &found);
            if (found) {
                i32 gate = g_61ab20;
                i32 item = g_61ab24;
                if (gate != 0) {
                    CSbiCueRecord* p = (CSbiCueRecord*)found;
                    if (g_6bf3c0 - (u32)p->m_14 >= (u32)p->m_18) {
                        p->m_14 = g_6bf3c0;
                        p->m_10->ConfigureItem(item, 0, 0, 0);
                    }
                }
            }
        }
        ActivateCursor(cmd - 0x13b, 1);
        return 1;
    }
    UpdateStatusBarTabHighlight(a, x, y);
    return 1;
}

// Clear stat[idx]: disable its hit-rect (zero +0x44 + m_enabled); on the active
// (tab 1) screen notify the stat object and play the GAME_STATZTABTOGGLE cue on
// the draw-clock window; then clear the stat flag. Returns 1.
RVA(0x00104f90, 0xa8)
i32 CSBI_RectOnly::ClearStat(i32 idx) {
    CSbiRect* r = m_hitRects[idx];
    if (r != 0) {
        *(i32*)((char*)r + 0x44) = 0;
        r->m_enabled = 0;
        if (m_activeTab == 1) {
            m_statObj[idx]->Notify2(*(i32*)this, 1);
            CSbiMusicHost* host = g_gameReg->m_world->m_28;
            if (host->m_30 == 0) {
                void* found = 0;
                CSbiLookupMap* map = &host->m_map10;
                map->Lookup("GAME_STATZTABTOGGLE", &found);
                if (found) {
                    i32 gate = g_61ab20;
                    i32 item = g_61ab24;
                    if (gate != 0) {
                        CSbiCueRecord* p = (CSbiCueRecord*)found;
                        if (g_6bf3c0 - (u32)p->m_14 >= (u32)p->m_18) {
                            p->m_14 = g_6bf3c0;
                            p->m_10->ConfigureItem(item, 0, 0, 0);
                        }
                    }
                }
            }
        }
    }
    m_statFlags[idx] = 0;
    return 1;
}

// Position a falling-item status bar: bail unless a highlight row is pending; hit
// the rect under (x,y) and require it be a falling-item widget (cmd 0xce/0xd0);
// clamp x into the rect's inset span, convert to item-local coords, drive the
// falling-item bar, then arm highlight row arg2.
// @early-stop
// ~70%: the gate chain, the hit-test, the cmd filter, the x-clamp, the coord
// conversion and both calls are byte-correct, but retail reserves a 0x10 RECT
// frame and spills the rect's yLo/yHi into it (dead stores left by an inlined
// RECT-staging the optimizer half-removed). MSVC DCEs any unread local I add, so
// the spills are not reproducible from C - a dead-store/scheduling wall; deferred.
RVA(0x00107920, 0xb7)
i32 CSBI_RectOnly::SetFallRect(i32 x, i32 y, i32 item) {
    char* B = (char*)this;
    if (m_pendingHlRow == -1) {
        return 0;
    }
    CSbiRect* r = HitTestRects(x, y);
    if (r == 0) {
        return 0;
    }
    if (r->m_cmd != 0xce && r->m_cmd != 0xd0) {
        return 0;
    }
    i32* rc = (i32*)((char*)r + 0x14); // &m_xLo
    i32 cx = x;
    i32 lo = rc[0] + 0x1b;
    i32 xHi = rc[2];
    if (x < lo) {
        cx = lo;
    } else if (x > xHi - 0x1a) {
        cx = xHi - 0x1a;
    }
    i32 localX = cx - *(i32*)(B + 0x10);
    i32 localY = 0x1b3 - *(i32*)(B + 0x14);
    UpdateFallingItemStatusBar(item, localX, localY);
    EnterHlRow(1, item);
    return 1;
}

// Activate a ready slot: bail if the highlight sub-manager is busy. With idx == -1
// find the first slot whose state is 2 (ready); otherwise require slot idx to be
// ready. Validate handle 0x66, play the GAME_TABHIGHLIGHT1 cue on the draw-clock
// window, then latch the active slot, mark its value, and notify its pointer.
// @early-stop
// ~68%: both the find-first-ready and the indexed branch - the handle validation,
// the cue play, the activeSlot/value latch and the slot notify - are byte-correct;
// the residual is the identical-return-epilogue wall (the three `return 0` paths
// tail-merge where retail inlines them, flipping the busy-gate je/jne) plus a
// one-instruction eager read in the find-slot loop. Documented walls; deferred.
RVA(0x0010b930, 0x1a7)
i32 CSBI_RectOnly::ActivateSlot(i32 idx) {
    if (g_gameReg->m_curState->m_4f0 != 0) {
        return 0;
    }
    if (idx == -1) {
        i32 slot = 0;
        while (m_slots[slot].m_state != kSlotReady) {
            slot++;
            if (slot >= 5) {
                return 0;
            }
        }
        if (!ResolveHandle(0x66)) {
            return 0;
        }
        CSbiMusicHost* host = g_gameReg->m_world->m_28;
        if (host->m_30 == 0) {
            void* found = 0;
            CSbiLookupMap* map = &host->m_map10;
            map->Lookup("GAME_TABHIGHLIGHT1", &found);
            if (found) {
                i32 gate = g_61ab20;
                i32 item = g_61ab24;
                if (gate != 0) {
                    CSbiCueRecord* p = (CSbiCueRecord*)found;
                    if (g_6bf3c0 - (u32)p->m_14 >= (u32)p->m_18) {
                        p->m_14 = g_6bf3c0;
                        p->m_10->ConfigureItem(item, 0, 0, 0);
                    }
                }
            }
        }
        m_activeSlot = slot;
        m_slots[slot].m_value = 1;
        if (m_slotNotify[slot]) {
            m_slotNotify[slot]->Notify(1);
        }
        return 1;
    }
    if (m_slots[idx].m_state != kSlotReady) {
        return 0;
    }
    if (!ResolveHandle(0x66)) {
        return 0;
    }
    CSbiMusicHost* host = g_gameReg->m_world->m_28;
    if (host->m_30 == 0) {
        void* found = 0;
        CSbiLookupMap* map = &host->m_map10;
        map->Lookup("GAME_TABHIGHLIGHT1", &found);
        if (found) {
            i32 gate = g_61ab20;
            i32 item = g_61ab24;
            if (gate != 0) {
                CSbiCueRecord* p = (CSbiCueRecord*)found;
                if (g_6bf3c0 - (u32)p->m_14 >= (u32)p->m_18) {
                    p->m_14 = g_6bf3c0;
                    p->m_10->ConfigureItem(item, 0, 0, 0);
                }
            }
        }
    }
    m_activeSlot = idx;
    m_slots[idx].m_value = 1;
    if (m_slotNotify[idx]) {
        m_slotNotify[idx]->Notify(1);
    }
    return 1;
}

// CSbiLayer/CSbiRenderObj/CSbiPtrColl2/CSbiFreeNode moved to <Gruntz/SBI_RectOnly.h>.

// 0xfe3e0 - SetState(state): if the mode gate (m_hlBusy) is up, no-op (return 1);
// if already in `state`, return 1. For the subtype-2 cursor state, run the
// activation probe (bail 0 on failure) and mirror the subtype tag into m_4;
// otherwise fire the plain notify. Then latch the new state into slot 0 and tell
// the highlight sub-manager (new, old). Returns 1.
RVA(0x000fe3e0, 0x55)
i32 CSBI_RectOnly::SetState(i32 state) {
    if (m_hlBusy != 0) {
        return 1;
    }
    i32 old = *(i32*)this;
    if (old == state) {
        return 1;
    }
    if (state == 2) {
        if (StateProbe() == 0) {
            return 0;
        }
        m_4 = *(i32*)this;
    } else {
        StateNotify();
    }
    old = *(i32*)this;
    *(i32*)this = state;
    g_gameReg->m_curState->SetState(state, old);
    return 1;
}

// 0xfe520 - place the rect-only HUD panel: gated on the mode (m_hlBusy) and the
// offset-0 subtype tag; pre-teardown notify, set the right-anchored 0xa0-wide
// full-height rect (+0x10) from the view width (g_gameReg->m_modeW), notify, refresh
// the highlight sub-manager, then probe-and-apply (m_10c). On probe failure log
// the placement error and bail. Returns 1.
RVA(0x000fe520, 0xa9)
i32 CSBI_RectOnly::winapi_0fe520_SetRect() {
    if (m_hlBusy != 0) {
        return 1;
    }
    if (*(i32*)this == 0) {
        return 1;
    }
    TeardownNotify(1);
    // Retail reads the view extent (m_modeW,m_modeH) as a POINT but only uses x for the
    // rect; the y store survives as a dead 8-byte-frame spill. `volatile` reproduces
    // that preserved store (a plain local is dead-eliminated by MSVC5 /O2).
    i32 w = g_gameReg->m_modeW;
    volatile POINT pt;
    pt.y = g_gameReg->m_modeH;
    SetRect((LPRECT)&m_10, w - 0xa0, 0, w, 0x1e0);
    RectNotify(0);
    g_gameReg->m_curState->Refresh();
    if (RectProbe() == 0) {
        g_gameReg->ReportError(kActivateErrId, 0x449);
        return 0;
    }
    RectApply(m_activeTab, 3);
    return 1;
}

// 0xfe670 - RefreshState: gated on m_hlBusy (return 1) and the subtype-2 tag
// (return 1 when not cursor); for the cursor subtype, tail-call the armed (m_4==1)
// or idle refresh path.
RVA(0x000fe670, 0x2b)
i32 CSBI_RectOnly::RefreshState() {
    if (m_hlBusy != 0) {
        return 1;
    }
    if (*(i32*)this != 2) {
        return 1;
    }
    if (m_4 == 1) {
        return RefreshA();
    }
    return RefreshB();
}

// 0xfe860 - SetSpritePos(x, y): push the position into the render object
// (m_8->m_5c/m_60) and mirror it into m_24/m_28. Bails (0) if there is no render
// object. Returns 1.
// @early-stop
// ~96.7%: every store/offset is byte-correct; the residual is a regalloc choice -
// retail keeps `y` unloaded until after the m_8 reload (y in edx, m_8 in esi),
// while the recompile parks y in esi loaded early. Not source-steerable; deferred.
RVA(0x000fe860, 0x2d)
i32 CSBI_RectOnly::SetSpritePos(i32 x, i32 y) {
    CSbiRenderObj* r = (CSbiRenderObj*)m_8;
    if (r == 0) {
        return 0;
    }
    r->m_5c = x;
    ((CSbiRenderObj*)m_8)->m_60 = y;
    m_28 = y;
    m_24 = x;
    return 1;
}

// 0xfe8a0 - HitTestLayer(x, y): test the point against the render object's layer
// rect - origin (m_198->m_10/m_14) plus the position-relative inset
// (m_198->m_18/m_1c offset by m_5c/m_60). Returns 1 inside, 0 outside.
RVA(0x000fe8a0, 0x4e)
i32 CSBI_RectOnly::HitTestLayer(i32 x, i32 y) {
    CSbiRenderObj* r = (CSbiRenderObj*)m_8;
    CSbiLayer* L = r->m_198;
    i32 xlo = r->m_5c - L->m_18;
    i32 ylo = r->m_60 - L->m_1c;
    i32 xhi = L->m_10 + xlo;
    i32 yhi = L->m_14 + ylo;
    if (x >= xhi || x < xlo || y >= yhi || y < ylo) {
        return 0;
    }
    return 1;
}

// 0x108410 - InsertPtr(a, b): pull a node off the engine free-list, fill it with
// (a, b), then insert it into the m_ptrPool pooled-ptr collection - at the first slot
// whose key (m_4) exceeds b, or appended at the end. Returns 1.
// @early-stop
// ~75.9%: every operation/offset is byte-correct; the residual is regalloc + block
// layout - retail pins the free-list head in edx and reads arg `a` at the top
// (before the pushes), and falls through from the scan loop into the append block
// (jl loop), while the recompile colors the head into eax, loads `a` late, and
// reaches append via an extra jmp. Not source-steerable; deferred to the final sweep.
RVA(0x00108410, 0x8e)
i32 CSBI_RectOnly::InsertPtr(i32 a, i32 b) {
    CSbiFreeNode* head = (CSbiFreeNode*)g_freeList;
    CSbiFreeNode* node = 0;
    if (head->m_0 != 0) {
        node = (CSbiFreeNode*)&head->m_4;
        node->m_0 = a;
        node->m_4 = b;
        g_freeList = (void*)((CSbiFreeNode*)g_freeList)->m_0;
    }
    i32 n = m_ptrCount;
    i32 i = 0;
    if (n > 0) {
        CSbiFreeNode** t = (CSbiFreeNode**)m_ptrTable;
        do {
            CSbiFreeNode* e = *t;
            if (e != 0 && b < e->m_4) {
                ((CSbiPtrColl2*)&m_ptrPool)->InsertAt(i, node, 1);
                return 1;
            }
            i++;
            t++;
        } while (i < n);
    }
    ((CSbiPtrColl2*)&m_ptrPool)->Append(m_ptrCount, node);
    return 1;
}

// 0x10bb50 - ReportTab(tab): log the tab with the (0x4f, 0x1b3) id pair, then
// apply it on `this` as (1, tab). Both helpers are reloc-masked siblings.
RVA(0x0010bb50, 0x24)
void CSBI_RectOnly::ReportTab(i32 tab) {
    ReportLog(tab, 0x4f, 0x1b3);
    ReportApply(1, tab);
}

// -------------------------------------------------------------------------
// Engine-label backlog stubs.
// -------------------------------------------------------------------------
// SbiTabRect/CSbiTab/CSbiRectSub/CSBI_MenuItem/SbiTabFrame*/CTabList moved to
// <Gruntz/SBI_RectOnly.h>.

// 0xffde0 - build the five top-level status-bar tabs (STATZ/GRUNTZ/RESOURCE/
// MULTIPLAYER/GAMETAB) plus three rect-only sub-widgets, once (m_tabsBuilt gate). Each
// widget is heap-allocated, its retail vtable + type tag stamped, then Setup/
// Configure'd from the tab base coords (m_10/m_14) + a per-widget geometry rect
// (and, for the tabs, an asset key + type code); on success it is appended to the
// +0x2c list and stashed in its per-tab slot (m_tabSprite0..m_tabSprite4). Built under a /GX EH
// frame (a just-created item is deleted if its setup throws / returns 0). The
// MULTIPLAYER tab gets a mode-specific configure in a battlez game (m_134==1).
// Finishes with three validity probes; returns 0 on any failure, else latches
// m_tabsBuilt = 1 and returns 1.
// @early-stop
// /GX EH-frame wall - identical archetype to StatusBarGameMenu::BuildGameMenu (also
// @early-stop ~37%). The dominant residual is structural: retail carries a /GX frame +
// a per-item incrementing EH state machine because each item's operator-new + ctor is
// inlined and its setup call is a delete-on-throw region; modeled here with real
// polymorphic sub-widget classes (CSbiRectSub / CSBI_MenuItem, whose ctors MSVC
// auto-stamps - no manual vtable stamp), cl's EH bookkeeping + the by-value rect arg
// scheduling diverge, shifting the frame. The rect sub-widget's true class CSBI_RectOnly
// collides with the HOST name so its vtable stays reloc-masked; the CSBI_MenuItem vtable
// is now correctly named. Logic complete; deferred to the final sweep (re-attack once
// the CSBI_* item ctors land and the frame can be reproduced).
RVA(0x000ffde0, 0x5b1)
i32 CSBI_RectOnly::BuildStatusBarTabs() {
    if (m_tabsBuilt != 0) {
        return 1;
    }
    if (m_c == 0) {
        return 0;
    }
    i32 bx = m_10;
    i32 by = m_rect14.m_0;
    i32 code = m_c;
    CSbiTab* it;
    SbiTabRect r;

    // ---- rect-only sub-widget A (id 0x259) ----
    it = new CSbiRectSub;
    r.l = bx + 0x7c;
    r.t = by + 0xad;
    r.r = bx + 0x88;
    r.b = by + 0xb9;
    if (!it->Setup(this, code, 0x259, 0, r, 0, -1)) {
        if (it) {
            delete it;
        }
        return 0;
    }
    ((CTabList*)((char*)this + 0x2c))->AddTail(it);

    // ---- rect-only sub-widget B (id 0x25a) ----
    it = new CSbiRectSub;
    r.l = bx + 0x8a;
    r.t = by + 0xb9;
    r.r = bx + 0x96;
    r.b = by + 0xc7;
    if (!it->Setup(this, code, 0x25a, 0, r, 0, -1)) {
        if (it) {
            delete it;
        }
        return 0;
    }
    ((CTabList*)((char*)this + 0x2c))->AddTail(it);

    // ---- rect-only sub-widget C (id 0x25b) ----
    it = new CSbiRectSub;
    r.l = bx + 0x83;
    r.t = by + 0xbb;
    r.r = bx + 0x8f;
    r.b = by + 0xc7;
    if (!it->Setup(this, code, 0x25b, 0, r, 0, -1)) {
        if (it) {
            delete it;
        }
        return 0;
    }
    ((CTabList*)((char*)this + 0x2c))->AddTail(it);

    // ---- STATZTAB (menu item, type 1) ----
    it = new CSBI_MenuItem;
    r.l = bx + 0x42;
    r.t = by + 0x82;
    r.r = bx + 0x62;
    r.b = by + 0x99;
    if (!it->Configure(this, code, 1, 0, r, "GAME_STATUSBAR_TABZ_STATZTAB", -1, 0)) {
        if (it) {
            delete it;
        }
        return 0;
    }
    ((CTabList*)((char*)this + 0x2c))->AddTail(it);
    m_tabSprite0 = (CSbiSprite*)it;

    // ---- GRUNTZTAB (menu item, type 2) ----
    it = new CSBI_MenuItem;
    r.l = bx + 0x04;
    r.t = by + 0x82;
    r.r = bx + 0x24;
    r.b = by + 0x99;
    if (!it->Configure(this, code, 2, 0, r, "GAME_STATUSBAR_TABZ_GRUNTZTAB", -1, 0)) {
        if (it) {
            delete it;
        }
        return 0;
    }
    ((CTabList*)((char*)this + 0x2c))->AddTail(it);
    m_tabSprite2 = (CSbiSprite*)it;

    // ---- RESOURCETAB (menu item, type 3) ----
    it = new CSBI_MenuItem;
    r.l = bx + 0x24;
    r.t = by + 0x82;
    r.r = bx + 0x44;
    r.b = by + 0x99;
    if (!it->Configure(this, code, 3, 0, r, "GAME_STATUSBAR_TABZ_RESOURCETAB", -1, 0)) {
        if (it) {
            delete it;
        }
        return 0;
    }
    ((CTabList*)((char*)this + 0x2c))->AddTail(it);
    m_tabSprite1 = (CSbiSprite*)it;

    // ---- MULTIPLAYERTAB (menu item, type 4) ----
    it = new CSBI_MenuItem;
    r.l = bx + 0x60;
    r.t = by + 0x82;
    r.r = bx + 0x80;
    r.b = by + 0x99;
    if (!it->Configure(this, code, 4, 0, r, "GAME_STATUSBAR_TABZ_MULTIPLAYERTAB", -1, 0)) {
        if (it) {
            delete it;
        }
        return 0;
    }
    ((CTabList*)((char*)this + 0x2c))->AddTail(it);
    m_tabSprite3 = (CSbiSprite*)it;
    if (g_gameReg->m_134 == 1) {
        CSBI_MenuItem* mp = (CSBI_MenuItem*)it;
        mp->m_34 = 4;
        SbiTabFrame* f = (SbiTabFrame*)mp->m_38;
        i32 v;
        if (f != 0 && f->m_64 <= 4 && f->m_68 >= 4) {
            v = f->m_14->m_10;
        } else {
            v = 0;
        }
        mp->m_30 = v;
        mp->m_4 = 0;
        mp->v28();
    }

    // ---- GAMETAB (menu item, type 5; inline ctor) ----
    it = new CSBI_MenuItem;
    r.l = bx + 0x7e;
    r.t = by + 0x82;
    r.r = bx + 0x9e;
    r.b = by + 0x99;
    if (!it->Configure(this, code, 5, 0, r, "GAME_STATUSBAR_TABZ_GAMETAB", -1, 0)) {
        if (it) {
            delete it;
        }
        return 0;
    }
    ((CTabList*)((char*)this + 0x2c))->AddTail(it);
    m_tabSprite4 = (CSbiSprite*)it;

    if (Probe2e69() == 0) {
        return 0;
    }
    if (TabRefresh() == 0) {
        return 0;
    }
    if (Probe41a1() == 0) {
        return 0;
    }
    m_tabsBuilt = 1;
    return 1;
}

// The inlined game RNG (shared with BootyWalkAnim / CGruntSpawnConfig): the LCG
// seed + the seed-init flag (bit 0) + the timeGetTime entry pointer. Bound (DATA)
// by BootyWalkAnim / m5_SoundTickCtor; referenced here as reloc-masked externs.
extern "C" u32(WINAPI* g_pTimeGetTime)(); // 0x6c4650

// MSVC-style LCG rand() (x = x*214013 + 2531011), lazily seeded from timeGetTime.
// The range test is first so the divisor is proven non-zero (no idiv guard) and
// the rand-gen is duplicated across both arms exactly as retail inlines it: range
// 0 -> a 0/1 coin, else 1..range. Inlined at each of the roulette's four nodes.
static __inline i32 WapRand(i32 range) {
    u32 x;
    if (range == 0) {
        if (!(g_randSeeded & 1)) {
            g_randSeeded |= 1;
            x = g_pTimeGetTime();
        } else {
            x = g_randSeed;
        }
        g_randSeed = x * 214013 + 2531011;
        return ((u32)g_randSeed >> 16) & 1;
    }
    if (!(g_randSeeded & 1)) {
        g_randSeeded |= 1;
        x = g_pTimeGetTime();
    } else {
        x = g_randSeed;
    }
    g_randSeed = x * 214013 + 2531011;
    return (((i32)g_randSeed >> 16) & 0x7fff) % range + 1;
}

// 0x107d00 - the falling-item weighted-random roulette. When the game is over
// (m_134==1), return the first pooled item's id to the free-list (or notify the
// falling-item sink with 0). Otherwise pick an item type from the running-sum
// battlez percent table via a three-tier weighted random (a top-level toolz/toyz/
// brickz split, then a per-category item roll), map the roll to the item-type id,
// place the falling-item HUD rect (SetRect + the notify-object rect block), and
// tick the pending fall count. Returns 1.
// @early-stop
// 88.4%: inlined-LCG-rand + weighted-tree archetype (the twin left as a pure 0%
// stub at CGruntSpawnConfig::PickWeighted 0x11bee0). Logic + the whole three-tier
// tree + the LCG lea-chain are byte-faithful (verified llvm-objdump -dr). Residual
// is a callee-saved-reg CSE pick: retail caches g_randSeeded in bl (a 4th push ebx)
// and the g_pTimeGetTime ptr in edi (mov edi,[ptr]; call edi), while cl emits a
// 3-register solution (flag in cl, call [ptr] indirect) and a `mov ecx,[m_134];cmp`
// vs retail's `cmp [m_134],1` direct memory compare. Plus the ?g_gameReg vs
// _g_mgrSettings shared-global DIR32 naming tail. Not source-steerable under /O2.
RVA(0x00107d00, 0x591)
i32 CSBI_RectOnly::winapi_107d00_SetRect() {
    i32 result;
    if (g_gameReg->m_134 == 1) {
        if (m_ptrCount > 0) {
            void* p = m_ptrTable[0];
            result = *(i32*)p;
            void** node = (void**)((char*)p - g_freeListNodeBias);
            *node = g_freeList;
            g_freeList = node;
            ((CSbiPtrColl2*)&m_ptrPool)->RemoveAt(0, 1);
        } else {
            result = 0;
            if (m_extraNotify0) {
                m_extraNotify0->Notify(0);
            }
        }
    } else {
        i32 r1 = WapRand(m_battlezPct[2]);
        if (r1 <= m_battlezPct[0]) {
            i32 r = WapRand(m_battlezPct[37]);
            if (r <= m_battlezPct[17]) {
                result = 1;
            } else if (r <= m_battlezPct[18]) {
                result = 2;
            } else if (r <= m_battlezPct[19]) {
                result = 3;
            } else if (r <= m_battlezPct[20]) {
                result = 4;
            } else if (r <= m_battlezPct[21]) {
                result = 5;
            } else if (r <= m_battlezPct[22]) {
                result = 6;
            } else if (r <= m_battlezPct[23]) {
                result = 7;
            } else if (r <= m_battlezPct[24]) {
                result = 8;
            } else if (r <= m_battlezPct[25]) {
                result = 9;
            } else if (r <= m_battlezPct[26]) {
                result = 10;
            } else if (r <= m_battlezPct[27]) {
                result = 11;
            } else if (r <= m_battlezPct[28]) {
                result = 12;
            } else if (r <= m_battlezPct[29]) {
                result = 13;
            } else if (r <= m_battlezPct[30]) {
                result = 14;
            } else if (r <= m_battlezPct[31]) {
                result = 15;
            } else if (r <= m_battlezPct[32]) {
                result = 16;
            } else if (r <= m_battlezPct[33]) {
                result = 17;
            } else if (r <= m_battlezPct[34]) {
                result = 18;
            } else if (r <= m_battlezPct[35]) {
                result = 19;
            } else {
                result = 0x15 + (r > m_battlezPct[36]);
            }
        } else if (r1 <= m_battlezPct[1]) {
            i32 r = WapRand(m_battlezPct[16]);
            if (r <= m_battlezPct[7]) {
                result = 0x17;
            } else if (r <= m_battlezPct[8]) {
                result = 0x18;
            } else if (r <= m_battlezPct[9]) {
                result = 0x19;
            } else if (r <= m_battlezPct[10]) {
                result = 0x1a;
            } else if (r <= m_battlezPct[11]) {
                result = 0x1b;
            } else if (r <= m_battlezPct[12]) {
                result = 0x1c;
            } else if (r <= m_battlezPct[13]) {
                result = 0x1d;
            } else if (r <= m_battlezPct[14]) {
                result = 0x1e;
            } else {
                result = 0x1f + (r > m_battlezPct[15]);
            }
        } else {
            i32 r = WapRand(m_battlezPct[6]);
            if (r <= m_battlezPct[3]) {
                result = 0x23;
            } else if (r <= m_battlezPct[4]) {
                result = 0x24;
            } else {
                result = 0x25 + (r > m_battlezPct[5]);
            }
        }
        if (result == 0x14) {
            result = 5;
        }
    }
    m_extraNotifyArg1 = result;
    m_machinePhase = 1;
    SetRect((LPRECT)&m_itemRectL, 0x49, 0xd7, 0x61, 0xef);
    if (m_extraNotify0) {
        i32 x = m_10;
        i32 y = m_rect14.m_0;
        m_extraNotify0->m_rect14[0] = m_itemRectL + x;
        m_extraNotify0->m_rect14[1] = m_itemRectT + y;
        m_extraNotify0->m_rect14[2] = m_itemRectR + x;
        m_extraNotify0->m_rect14[3] = m_itemRectB + y;
    }
    RefreshFallRect();
    i32 c = m_rezTick;
    m_rezActive = 0;
    if (c > 0) {
        m_rezTick = c - 1;
        FallItemTick();
    }
    return 1;
}

// 0xfdc00 - reset the item to the multiplayer-item-percent (battlez) layout: place
// the right-anchored full-height rect from the view width, seed the cursor coords,
// reset the widgets, then (on a successful placement probe) fill the running-sum
// item-percent table from the Multiplayer config section (one GetInt per item, each
// summed with the prior entry), apply the (5,3) rect, and - if the placement query
// succeeds - refresh. Returns 1 (0 on probe failure).
// @early-stop
// ~93.7%: every store/offset/GetInt-sum is byte-exact vs retail. Residual is the
// zero-register-pinning wall (docs/patterns/zero-register-pinning.md): retail pins
// the constant 0 in edi + view-x/y in ebx/ebp, while MSVC5 here pins 0 in ebx +
// view-x in edi - a 1-instr phase shift cascading through every =0 store, the
// running-sum reload registers, and the trailing `push 0`. Plus the SetRect import
// (ff 15 -> __imp_SetRect vs PTR_SetRect) + g_gameReg DIR32 naming. Not source-
// steerable under /O2; deferred to the final sweep.
RVA(0x000fdc00, 0x5c2)
i32 CSBI_RectOnly::LoadBattlezItemConfig(i32 arg) {
    m_c = arg;
    m_4 = 0;
    *(i32*)this = 0;
    i32 vx = g_gameReg->m_modeW;
    i32 vy = g_gameReg->m_modeH;
    SetRect((LPRECT)((char*)this + 0x10), vx - 0xa0, 0, vx, 0x1e0);
    m_rect14.m_c = 0;
    m_24 = vx - 0x45;
    m_28 = vy - 0x30;
    m_itemKind = 5;
    m_tabCycle = g_644c54;
    ResetTabWidgets2b44();
    if (RectProbe() == 0) {
        return 0;
    }
    m_activeSlot = -1;
    m_pendingHlRow = -1;
    m_rezActive = 0;
    m_rezTick = 0;
    m_toggleActive = 0;
    m_toggleHandle = 0;
    m_battlezPct[0] = g_buteMgr.GetInt("Multiplayer", "ToolzPercent");
    m_battlezPct[1] = m_battlezPct[0] + g_buteMgr.GetInt("Multiplayer", "ToyzPercent");
    m_battlezPct[2] = m_battlezPct[1] + g_buteMgr.GetInt("Multiplayer", "BrickzPercent");
    m_battlezPct[3] = m_battlezPct[2] + g_buteMgr.GetInt("Multiplayer", "RedBrick");
    m_battlezPct[4] = m_battlezPct[3] + g_buteMgr.GetInt("Multiplayer", "BlueBrick");
    m_battlezPct[5] = m_battlezPct[4] + g_buteMgr.GetInt("Multiplayer", "GoldBrick");
    m_battlezPct[6] = m_battlezPct[5] + g_buteMgr.GetInt("Multiplayer", "BlackBrick");
    m_battlezPct[7] = m_battlezPct[6] + g_buteMgr.GetInt("Multiplayer", "BabyWalkerz");
    m_battlezPct[8] = m_battlezPct[7] + g_buteMgr.GetInt("Multiplayer", "BeachBallz");
    m_battlezPct[9] = m_battlezPct[8] + g_buteMgr.GetInt("Multiplayer", "BigWheelz");
    m_battlezPct[10] = m_battlezPct[9] + g_buteMgr.GetInt("Multiplayer", "GoKartz");
    m_battlezPct[11] = m_battlezPct[10] + g_buteMgr.GetInt("Multiplayer", "JackInTheBoxz");
    m_battlezPct[12] = m_battlezPct[11] + g_buteMgr.GetInt("Multiplayer", "JumpRopez");
    m_battlezPct[13] = m_battlezPct[12] + g_buteMgr.GetInt("Multiplayer", "PogoStickz");
    m_battlezPct[14] = m_battlezPct[13] + g_buteMgr.GetInt("Multiplayer", "Scrollz");
    m_battlezPct[15] = m_battlezPct[14] + g_buteMgr.GetInt("Multiplayer", "SqueakToyz");
    m_battlezPct[16] = m_battlezPct[15] + g_buteMgr.GetInt("Multiplayer", "Yoyoz");
    m_battlezPct[17] = m_battlezPct[16] + g_buteMgr.GetInt("Multiplayer", "Bombz");
    m_battlezPct[18] = m_battlezPct[17] + g_buteMgr.GetInt("Multiplayer", "Boomerangz");
    m_battlezPct[19] = m_battlezPct[18] + g_buteMgr.GetInt("Multiplayer", "Brickz");
    m_battlezPct[20] = m_battlezPct[19] + g_buteMgr.GetInt("Multiplayer", "Clubz");
    m_battlezPct[21] = m_battlezPct[20] + g_buteMgr.GetInt("Multiplayer", "Gauntletz");
    m_battlezPct[22] = m_battlezPct[21] + g_buteMgr.GetInt("Multiplayer", "Glovez");
    m_battlezPct[23] = m_battlezPct[22] + g_buteMgr.GetInt("Multiplayer", "Gooberz");
    m_battlezPct[24] = m_battlezPct[23] + g_buteMgr.GetInt("Multiplayer", "GravityBootz");
    m_battlezPct[25] = m_battlezPct[24] + g_buteMgr.GetInt("Multiplayer", "GunHatz");
    m_battlezPct[26] = m_battlezPct[25] + g_buteMgr.GetInt("Multiplayer", "NerfGunz");
    m_battlezPct[27] = m_battlezPct[26] + g_buteMgr.GetInt("Multiplayer", "Rockz");
    m_battlezPct[28] = m_battlezPct[27] + g_buteMgr.GetInt("Multiplayer", "Shieldz");
    m_battlezPct[29] = m_battlezPct[28] + g_buteMgr.GetInt("Multiplayer", "Shovelz");
    m_battlezPct[30] = m_battlezPct[29] + g_buteMgr.GetInt("Multiplayer", "Springz");
    m_battlezPct[31] = m_battlezPct[30] + g_buteMgr.GetInt("Multiplayer", "Spyz");
    m_battlezPct[32] = m_battlezPct[31] + g_buteMgr.GetInt("Multiplayer", "Swordz");
    m_battlezPct[33] = m_battlezPct[32] + g_buteMgr.GetInt("Multiplayer", "TimeBombz");
    m_battlezPct[34] = m_battlezPct[33] + g_buteMgr.GetInt("Multiplayer", "Toobz");
    m_battlezPct[35] = m_battlezPct[34] + g_buteMgr.GetInt("Multiplayer", "Wandz");
    m_battlezPct[36] = m_battlezPct[35] + g_buteMgr.GetInt("Multiplayer", "Welderz");
    m_battlezPct[37] = m_battlezPct[36] + g_buteMgr.GetInt("Multiplayer", "Wingz");
    RectApply(5, 3);
    if (g_gameReg->m_38->QueryPos("StatusBar Position", 0) == 1) {
        RefreshA();
    }
    return 1;
}

// CSbiMainSetup/CSbiMainL2/CSbiMainL1/CSbiFrameEntry/CSbiMainBarCfg + the
// MainBarDrawFrame decl moved to <Gruntz/SBI_RectOnly.h>.

// 0xfe6b0 - drive the main status-bar sprite. For the non-cursor subtype, when the
// countdown (+0x20) is live, tick it; if the frame gate (m_barFrameGate) exceeds the screen
// height push the current rect to the game-manager's setup chain; look up
// GAME_STATUSBAR_MAINBAR and, on the resolved frame, draw it at the item's origin. Then
// fire the +0x30 and active-tab notify lists (slot 0x14) and refresh the +0x54c object.
// The trailing +0xd8 list (slot 0x28 then 0x14) runs for every subtype. Returns 1.
// @early-stop
// the two stack-struct arg builds (the +0x14 rect handed to SetRectXY and the frame-draw
// args) are byte-correct in content but MSVC schedules the interleaved local stores /
// packs the 0x14 frame differently than retail (the statement-schedule wall shared with
// Setup); the three notify walks + the config lookup are byte-faithful. Residual is that
// scheduling plus the g_gameReg / GAME_STATUSBAR_MAINBAR DIR32 naming. Deferred.
// NEIGHBOR TRIGGER: the sibling cue functions (HlClickGroup*/HiCue*), and now the
// CSbiMusicHost::m_map10 real-member de-cast, reshuffle this function's
// MainBarDrawFrame arg-block register allocation (the documented MSVC5 cross-function
// codegen leak), dropping the byte-match 95.6%->88.6% with NO source change here; the
// frame-draw args are still byte-content-correct. Accepted per the de-cast mandate.
RVA(0x000fe6b0, 0x145)
i32 CSBI_RectOnly::LoadMainStatusBarSprite() {
    if (*(i32*)this != kSubtypeTag) {
        if (m_rect14.m_c > 0) {
            m_rect14.m_c--;
            i32 v = m_barFrameGate;
            if (v > 0x1e0) {
                CSbiMainSetup* tgt = g_gameReg->m_world->m_4->m_14->m_2c;
                struct {
                    i32 a, b, c, d;
                } rc;
                rc.a = m_10;
                rc.d = v;
                rc.b = m_rect14.m_8;
                rc.c = m_rect14.m_4;
                tgt->SetRectXY(&rc, 0);
            }
            char* mc = *(char**)((char*)this + 0xc);
            void* found = 0;
            CSbiLookupMap* map = (CSbiLookupMap*)(*(char**)(mc + 0x10) + 0x10);
            map->Lookup("GAME_STATUSBAR_MAINBAR", &found);
            if (found) {
                CSbiMainBarCfg* cfg = (CSbiMainBarCfg*)found;
                CSbiFrameEntry* entry = cfg->m_14[cfg->m_64];
                if (entry) {
                    CSbiMainL1* l1 = g_gameReg->m_world->m_4;
                    MainBarDrawFrame(l1->m_14, entry->m_18 + m_10, entry->m_1c + m_rect14.m_0, 0);
                }
            }
        }

        char* B = (char*)this;
        CSbiNotifyNode* n = *(CSbiNotifyNode**)(B + 0x30);
        while (n) {
            CSbiNotifyNode* cur = n;
            n = n->m_next;
            if (cur->m_payload) {
                ((CSbiNotifyPayload*)cur->m_payload)->Slot14();
            }
        }
        CSbiNotifyNode* m = *(CSbiNotifyNode**)(B + m_activeTab * 0x1c + 0x30);
        while (m) {
            CSbiNotifyNode* cur = m;
            m = m->m_next;
            if (cur->m_payload) {
                ((CSbiNotifyPayload*)cur->m_payload)->Slot14();
            }
        }
        if (m_retabNotify) {
            m_retabNotify->Refresh();
        }
    }

    CSbiNotifyNode* k = m_listD4.m_head;
    while (k) {
        CSbiNotifyNode* cur = k;
        k = k->m_next;
        CSbiNotifyPayload* p = (CSbiNotifyPayload*)cur->m_payload;
        if (p) {
            p->Slot28();
            p->Slot14();
        }
    }
    return 1;
}

// CSbiHiWidget moved to <Gruntz/SBI_RectOnly.h>.

// The cached PostMessageA entry point (game-owned fn pointer; the highlight
// dispatcher posts WM_COMMAND via it, not the direct import).
extern "C" {
    DATA(0x002c44c8)
    extern i32(WINAPI* g_pPostMessageA)(void*, u32, i32, i32); // 0x6c44c8
}

// Play GAME_TABHIGHLIGHT1 immediately (no clock gate) - variant 1: the record is
// resolved by a direct FindCue on the host (returns the record) and played.
static __inline void HiCueFind() {
    CSbiMusicHost* host = g_gameReg->m_world->m_28;
    if (host->m_30 == 0) {
        void* obj = host->FindCue("GAME_TABHIGHLIGHT1");
        if (obj) {
            ((CSbiCueRecord*)obj)->PlayNow(g_61ab24, 0, 0, 0);
        }
    }
}

// Variant 2: resolve via the +0x10 string map (Lookup out-param) then play now.
static __inline void HiCueLookup() {
    CSbiMusicHost* host = g_gameReg->m_world->m_28;
    if (host->m_30 == 0) {
        void* out = 0;
        (&host->m_map10)->Lookup("GAME_TABHIGHLIGHT1", &out);
        if (out) {
            ((CSbiCueRecord*)out)->PlayNow(g_61ab24, 0, 0, 0);
        }
    }
}

// Variant 3: the standard draw-clock-gated cue play (like LoadGooCookingSprite).
static __inline void HiCueTimed() {
    CSbiMusicHost* host = g_gameReg->m_world->m_28;
    if (host->m_30 == 0) {
        void* found = 0;
        (&host->m_map10)->Lookup("GAME_TABHIGHLIGHT1", &found);
        if (found && g_61ab20 != 0) {
            i32 item = g_61ab24;
            CSbiCueRecord* p = (CSbiCueRecord*)found;
            if (g_6bf3c0 - (u32)p->m_14 >= (u32)p->m_18) {
                p->m_14 = g_6bf3c0;
                p->m_10->ConfigureItem(item, 0, 0, 0);
            }
        }
    }
}

// Post WM_COMMAND(cmdId) to the game window via the cached PostMessageA pointer.
static __inline void HiPost(i32 cmdId) {
    g_pPostMessageA(g_gameReg->m_4->m_4, 0x111, cmdId, 0);
}

// 0xfe910 - UpdateStatusBarTabHighlight(a1, a2, a3). Resolve the hit tab-highlight
// widget, run its Update, then dispatch on the widget kind (m_10, 0..6) and command
// (m_c): play the GAME_TABHIGHLIGHT1 cue and either refresh a sub-panel, arm a tab,
// post a WM_COMMAND to the game window, or toggle the destruct-button warning. The
// resource/gruntz/game tabs gate on m_354 + the active object's tab-highlight-enable
// flag (m_68->m_400). Returns 1 (0 on an out-of-range command).
// @early-stop
// COMPLETE reconstruction of a 2958-byte 7-way widget-kind switch (jump table
// 0x4ff4a0) with three nested command sub-switches lowered by MSVC to byte-indexed
// jump tables (0x4ff4bc / 0x4ff4ec+0x4ff4e0 / 0x4ff51c+0x4ff50c / 0x4ff528). Walls:
// (1) each dense sub-switch is a separate-COMDAT jump table (docs/patterns/
// switch-jumptable-separate-comdat.md, ~75-80%) and the byte-index tables aren't
// source-steerable; (2) the ~15 inlined cue-play blocks + PostMessage tails
// tail-merge/schedule differently; (3) the shared-global DIR32 naming
// (?g_gameReg vs _g_mgrSettings, ?g_61ab24 vs ?g_sndCueTag, g_pPostMessageA,
// GAME_TABHIGHLIGHT1 $SG). Logic is byte-faithful; deferred to the final sweep.
RVA(0x000fe910, 0xb8e)
i32 CSBI_RectOnly::UpdateStatusBarTabHighlight(i32 a1, i32 a2, i32 a3) {
    CSbiHiWidget* w = HiResolve(a2, a3);
    if (w == 0) {
        return 1;
    }
    w->Update(a1, a2, a3);
    i32 cmd = w->m_c;
    switch (w->m_10) {
        case 0:
            if (m_hitTestDisabled != 0) {
                return 1;
            }
            if (g_gameReg->m_68->m_400 == 0) {
                return 1;
            }
            if (cmd > 0x259) {
                if (cmd == 0x25a) {
                    HiCueFind();
                    RefreshB();
                    return 1;
                }
                if (cmd == 0x25b) {
                    HiCueFind();
                    HiRefreshResource();
                    return 1;
                }
                return 0;
            }
            if (cmd == 0x259) {
                HiCueFind();
                RefreshA();
                return 1;
            }
            if (cmd <= 0 || cmd > 5) {
                return 0;
            }
            HiCueFind();
            RectApply(cmd, 3);
            return 1;

        case 1:
            if (m_hitTestDisabled != 0) {
                return 1;
            }
            if (g_gameReg->m_68->m_400 == 0) {
                return 1;
            }
            if (cmd < 0x12c || cmd > 0x149) {
                return 0;
            }
            if (cmd <= 0x13a) {
                HiCueLookup();
                HiTabA(cmd - 0x12c);
            } else {
                HiCueLookup();
                HiTabB(cmd - 0x13b, 0);
            }
            return 1;

        case 2:
            if (m_hitTestDisabled != 0) {
                return 1;
            }
            if (g_gameReg->m_68->m_400 == 0) {
                return 1;
            }
            if (cmd < 0x64 || cmd > 0x68) {
                return 0;
            }
            HiSelectStat(cmd - 0x64);
            return 1;

        case 3:
            if (m_hitTestDisabled != 0) {
                return 1;
            }
            if (g_gameReg->m_68->m_400 == 0) {
                return 1;
            }
            if (cmd < 0xd3 || cmd > 0xde) {
                return 1;
            }
            if (cmd <= 0xd6) {
                HiGrunt0(cmd - 0xd3);
            } else if (cmd <= 0xda) {
                HiGrunt1(cmd - 0xd7);
            } else {
                HiGrunt2(cmd - 0xdb);
            }
            return 1;

        case 4:
            if (m_hitTestDisabled != 0) {
                return 1;
            }
            if (g_gameReg->m_68->m_400 == 0) {
                return 1;
            }
            if (cmd < 0x190 || cmd > 0x193) {
                return 0;
            }
            HiCueLookup();
            m_tabCycle = cmd - 0x190;
            TeardownNotify(0);
            FinishReset16ea();
            TabCommit();
            return 1;

        case 5:
            if (m_toggleActive != 0) {
                return 1;
            }
            switch (cmd) {
                case 0x1f4:
                    HiCueFind();
                    HiPost(0x8007);
                    return 1;
                case 0x1f5:
                    HiCueFind();
                    HiPost(0x80ce);
                    return 1;
                case 0x1f6:
                    HiCueFind();
                    HiPost(0x80cf);
                    return 1;
                case 0x1f8:
                    HiCueFind();
                    HiPost(0x8035);
                    return 1;
                case 0x1f7:
                    HiCueLookup();
                    HiPost(0x80e2);
                    return 1;
                case 0x1f9:
                    HiCueLookup();
                    if (g_gameReg->m_c != 0) {
                        g_gameReg->m_c ^= 1;
                        g_gameReg->SetToggle(g_gameReg->m_c, 1);
                    }
                    g_gameReg->m_curState->HiToggle(1);
                    return 1;
                case 0x1fa:
                    HiCueLookup();
                    ArmTab(5, 0);
                    return 1;
                case 0x1fc:
                    if (g_gameReg->m_134 != 1) {
                        return 1;
                    }
                    if (m_modeArmed != 0) {
                        return 1;
                    }
                    if (m_hitTestDisabled != 0) {
                        return 1;
                    }
                    HiCueLookup();
                    {
                        CSbiSubMgr* sm = g_gameReg->m_curState;
                        if (m_destructWarnActive == 0) {
                            m_destructWarnActive = 1;
                            m_modeState = 2;
                            m_destructWarnDelay = g_buteMgr.GetIntDef(
                                "StatusBar",
                                "DestructButtonWarningDelay",
                                0x32
                            );
                            m_destructWarnDelayHi = 0;
                            m_destructWarnLast = g_dat645588;
                            m_destructWarnLastHi = 0;
                            sm->PostWarn(1, 0xbb7);
                        } else {
                            CSbiSlotPtr* n = m_modeNotify;
                            m_destructWarnActive = 0;
                            m_modeState = 1;
                            if (n) {
                                n->Notify(1);
                            }
                            sm->PostWarn(0, 0xbb7);
                        }
                    }
                    return 1;
                default:
                    return 0;
            }

        case 6:
            switch (cmd) {
                case 0x324:
                    if (g_gameReg->m_68->m_288 == 1) {
                        HiCueLookup();
                        g_gameReg->HiPump();
                    } else if (g_gameReg->m_134 == 1) {
                        HiCueLookup();
                        HiPost(0x806b);
                    } else {
                        HiCueLookup();
                        g_gameReg->m_curState->HiRefresh(0);
                    }
                    return 1;
                case 0x325:
                    if (g_gameReg->m_134 == 1) {
                        if (g_gameReg->m_68->m_288 == 1) {
                            g_gameReg->Fn29aa();
                        }
                        HiCueLookup();
                        HiPost(0x8023);
                    } else {
                        HiCueTimed();
                        g_gameReg->HiPump();
                    }
                    return 1;
                case 0x327:
                    if (g_gameReg->m_134 == 1) {
                        if (g_gameReg->m_68->m_288 == 1) {
                            g_gameReg->Fn29aa();
                        }
                        HiCueTimed();
                        HiPost(0x8023);
                    } else {
                        HiCueTimed();
                        g_gameReg->HiPump();
                    }
                    return 1;
                case 0x328:
                    HiCueTimed();
                    g_gameReg->m_curState->HiRefresh(0);
                    return 1;
                default:
                    return 0;
            }

        default:
            return 0;
    }
}

// 0xffb20 - build (or release) the DESTRUCT button display object. Only touches it
// while the presence gate (g_gameReg->m_10) is up: when the mode is armed (m_destructWarnActive!=0)
// and not held (m_574==0), and the object is not yet built, look up GAME_DESTRUCT and
// build+configure it; otherwise release any existing object. Then refresh the host and
// fire the notify value `arg` down the three per-tab notify lists (+0x30, active tab,
// +0xd8) plus the +0x54c notifier. Returns 1.
// @early-stop
// ~97.2%: the code bytes are byte-exact vs retail (verified llvm-objdump -dr). The only
// scored diffs are the two DIR32 g_gameReg operands (retail _g_mgrSettings) and the
// GAME_DESTRUCT $SG string; every other reloc (Lookup/Build/Configure/Release/RefreshHost/
// Notify0/TabCommit) is a reloc-masked REL32. TU-wide rename, matcher.md reloc artifact.
RVA(0x000ffb20, 0x13a)
i32 CSBI_RectOnly::LoadDestructButtonSprite(i32 arg) {
    if (g_gameReg->m_10 != 0) {
        if (m_destructWarnActive != 0 && m_modeArmed == 0) {
            if (m_destructButton == 0) {
                CSbiMusicHost* host = g_gameReg->m_world->m_28;
                void* found = 0;
                (&host->m_map10)->Lookup("GAME_DESTRUCT", &found);
                if (found) {
                    CSbiSpriteFactory* f = ((CSbiSpriteCfg*)found)->m_10;
                    if (f) {
                        CSbiDisplayObj* obj = (CSbiDisplayObj*)f->Build();
                        m_destructButton = obj;
                        if (obj) {
                            obj->Configure(g_gameReg->m_11c, 0, 0, 1);
                        }
                    }
                }
            }
        } else {
            if (m_destructButton) {
                m_destructButton->Release();
                m_destructButton = 0;
            }
        }
    }
    RefreshHost();

    char* B = (char*)this;
    CSbiNotifyNode* n = *(CSbiNotifyNode**)(B + 0x30);
    while (n) {
        CSbiNotifyNode* cur = n;
        n = n->m_next;
        if (cur->m_payload) {
            ((CSbiNotifyPayload*)cur->m_payload)->Slot10(arg);
        }
    }
    CSbiNotifyNode* m = *(CSbiNotifyNode**)(B + m_activeTab * 0x1c + 0x30);
    while (m) {
        CSbiNotifyNode* cur = m;
        m = m->m_next;
        if (cur->m_payload) {
            ((CSbiNotifyPayload*)cur->m_payload)->Slot10(arg);
        }
    }
    CSbiNotifyNode* k = m_listD4.m_head;
    while (k) {
        CSbiNotifyNode* cur = k;
        k = k->m_next;
        if (cur->m_payload) {
            ((CSbiNotifyPayload*)cur->m_payload)->Slot10(arg);
        }
    }
    if (m_retabNotify) {
        m_retabNotify->Notify0(arg);
        TabCommit();
    }
    return 1;
}

// 0x102180 - build the RESUME game-tab button. If this is the subtype-2 cursor item,
// refresh it; when shown and not already on the gauge tab (5), apply the (5,3) rect;
// then (if the RESUME slot exists) configure it with the RESUME asset key, commit, and
// refresh it. Latch the show-RESUME gate (m_354 = 1).
RVA(0x00102180, 0x5f)
void CSBI_RectOnly::BuildGameTabResumeButton(i32 show) {
    if (*(i32*)this == kSubtypeTag) {
        TabSubtypeRefresh();
    }
    if (show && m_activeTab != 5) {
        RectApply(5, 3);
    }
    if (m_tabSprite5) {
        m_tabSprite5->Configure("GAME_STATUSBAR_TABZ_GAMETAB_RESUME", 1);
        TabCommit();
        m_tabSprite5->Refresh();
    }
    m_hitTestDisabled = 1;
}

// 0x102200 - build the PAUSE game-tab button. If the RESUME/PAUSE slot exists, configure
// it with the PAUSE asset key, commit, and refresh it. Clear the show-RESUME gate.
RVA(0x00102200, 0x37)
void CSBI_RectOnly::BuildGameTabPauseButton() {
    if (m_tabSprite5) {
        m_tabSprite5->Configure("GAME_STATUSBAR_TABZ_GAMETAB_PAUSE", 1);
        TabCommit();
        m_tabSprite5->Refresh();
    }
    m_hitTestDisabled = 0;
}

// 0x1055b0 - build the GOOCOOKING1 status-bar sprite for stat slot `idx`. Bails (0)
// if the slot is already active; when the game is running (m_134==1) and the mode
// gate is down (m_hlBusy==0), refresh the subtype-2 cursor, apply the (2,3) rect off the
// gauge tab, and commit. Latch the gauge span for slot idx+0x17 (base g_dat645588, hi
// 0x7fffffff); then, on the gauge tab and not the cursor subtype, play the
// GAME_GOOCOOKING1 cue on the draw-clock window if the music gate is free. Returns 1.
// @early-stop
// ~96.7%: the code bytes are byte-exact vs retail (verified llvm-objdump -dr base vs
// target). The residual is purely the reloc-symbol-naming scoring tail - this TU models
// the shared singletons as ?g_gameReg@@... / ?g_dat645588@@... / ?g_61ab20@@... etc.
// while retail names them _g_mgrSettings / _g_645588 / ?g_sndEnabled@@... / ?g_sndCueTag@@
// ... / _g_killCueClock (+ the GAME_GOOCOOKING1 $SG string), so those DIR32 operands
// don't pair. A TU-wide rename, not a per-function fix; matcher.md reloc artifact.
RVA(0x001055b0, 0x109)
i32 CSBI_RectOnly::LoadGooCookingSprite(i32 idx) {
    CSbiSlot* sp = &m_slots[idx];
    if (sp->m_state != 0) {
        return 0;
    }
    if (g_gameReg->m_134 == 1 && m_hlBusy == 0) {
        if (*(i32*)this == kSubtypeTag) {
            TabSubtypeRefresh();
        }
        if (m_activeTab != 2) {
            RectApply(2, 3);
        }
        TabCommit();
    }
    sp->m_state = 1;
    CSbiSlot* g = (CSbiSlot*)this + (idx + 0x17);
    g->m_8 = 0x7fffffff;
    g->m_c = 0;
    g->m_state = g_dat645588;
    g->m_value = 0;
    if (m_activeTab == 2 && *(i32*)this != 2) {
        CSbiMusicHost* host = g_gameReg->m_world->m_28;
        if (host->m_30 == 0) {
            void* found = 0;
            CSbiLookupMap* map = &host->m_map10;
            map->Lookup("GAME_GOOCOOKING1", &found);
            if (found) {
                i32 gate = g_61ab20;
                i32 item = g_61ab24;
                if (gate != 0) {
                    CSbiCueRecord* p = (CSbiCueRecord*)found;
                    if (g_6bf3c0 - (u32)p->m_14 >= (u32)p->m_18) {
                        p->m_14 = g_6bf3c0;
                        p->m_10->ConfigureItem(item, 0, 0, 0);
                    }
                }
            }
        }
    }
    return 1;
}

// 0x105990 - tick the rez-conveyor belt animation across the three group-A phase
// slots (each a 24-byte phase record: state, counter, 64-bit last-clock, 64-bit
// interval). Per slot, a state machine (1..7) advances the belt counter on a
// draw-clock gate, reconfigures the belt/hold delays at phase transitions, and plays
// the REZBELTRETURN/REZBELTBACKUP cues on the gauge tab. After the switch each slot's
// notifier is fired with the current counter.
// @early-stop
// complete reconstruction; residual is the 64-bit draw-clock gate scheduling +
// zero/1-register pinning across the 3-slot loop + the shared-global DIR32 naming
// (g_gameReg/g_dat645588/g_sndEnabled...); documented regalloc/scheduling walls.
RVA(0x00105990, 0x398)
void CSBI_RectOnly::UpdateRezConveyorStatusBar() {
    i32 count = 3;
    CSbiSlotPtr** notify = m_groupNotify;
    SbiPhaseSlot* ph = (SbiPhaseSlot*)m_groupSlots;
    do {
        switch (ph->m_state) {
            case 1:
                if (++ph->m_counter > 9) {
                    ph->m_counter = 1;
                }
                break;
            case 2:
                if ((i64)(u32)g_dat645588 - ph->m_last >= ph->m_interval) {
                    if (++ph->m_counter >= 0x12) {
                        ph->m_counter = 0x12;
                        ph->m_state = 7;
                        ph->m_interval =
                            g_buteMgr.GetIntDef("StatusBar", "ConveyorBeltHoldDelay", 0x1f4);
                        ph->m_last = (u32)g_dat645588;
                        ReportLog(m_extraNotifyArg0, m_itemRectL + 0xc, m_itemRectT + 0xc);
                        ConveyorReturn();
                    }
                }
                break;
            case 3:
                if ((i64)(u32)g_dat645588 - ph->m_last >= ph->m_interval) {
                    if (--ph->m_counter < 0xa) {
                        ph->m_state = 0;
                        ph->m_counter = 1;
                    }
                }
                break;
            case 4:
                if ((i64)(u32)g_dat645588 - ph->m_last >= ph->m_interval) {
                    if (++ph->m_counter >= 0x18) {
                        ph->m_counter = 0x18;
                        ph->m_state = 6;
                        ph->m_interval =
                            g_buteMgr.GetIntDef("StatusBar", "ConveyorBeltHoldInDelay", 0x1f4);
                        ph->m_last = (u32)g_dat645588;
                        m_machinePhase = 8;
                        m_beltInterval = g_buteMgr.GetIntDef("StatusBar", "FallingItemDelay", 0x32);
                        m_beltLast = (u32)g_dat645588;
                    }
                }
                break;
            case 5:
                if ((i64)(u32)g_dat645588 - ph->m_last >= ph->m_interval) {
                    if (--ph->m_counter < 0x13) {
                        ph->m_state = 0;
                        ph->m_counter = 1;
                    }
                }
                break;
            case 6:
                if ((i64)(u32)g_dat645588 - ph->m_last >= ph->m_interval) {
                    if (m_activeTab == 3 && *(i32*)this != 2) {
                        CSbiMusicHost* host = g_gameReg->m_world->m_28;
                        if (host->m_30 == 0) {
                            void* found = 0;
                            (&host->m_map10)->Lookup("GAME_REZBELTRETURN", &found);
                            if (found && g_61ab20 != 0) {
                                i32 item = g_61ab24;
                                CSbiCueRecord* p = (CSbiCueRecord*)found;
                                if (g_6bf3c0 - (u32)p->m_14 >= (u32)p->m_18) {
                                    p->m_14 = g_6bf3c0;
                                    p->m_10->ConfigureItem(item, 0, 0, 0);
                                }
                            }
                        }
                    }
                    ph->m_state = 5;
                }
                break;
            case 7:
                if ((i64)(u32)g_dat645588 - ph->m_last >= ph->m_interval) {
                    if (m_activeTab == 3 && *(i32*)this != 2) {
                        CSbiMusicHost* host = g_gameReg->m_world->m_28;
                        if (host->m_30 == 0) {
                            void* found = 0;
                            (&host->m_map10)->Lookup("GAME_REZBELTBACKUP", &found);
                            if (found && g_61ab20 != 0) {
                                i32 item = g_61ab24;
                                CSbiCueRecord* p = (CSbiCueRecord*)found;
                                if (g_6bf3c0 - (u32)p->m_14 >= (u32)p->m_18) {
                                    p->m_14 = g_6bf3c0;
                                    p->m_10->ConfigureItem(item, 0, 0, 0);
                                }
                            }
                        }
                    }
                    ph->m_state = 3;
                }
                break;
        }
        if (*notify) {
            (*notify)->Notify(ph->m_counter);
        }
        notify++;
        ph++;
    } while (--count);
}

// 0x105e40 - drive the rez-machine (goo-machine) status bar: a two-record phase
// machine. Record A (+0x318) handles the spew/run phases (states 5/6) throttled on a
// 64-bit draw-clock gate; record B (+0x330) is a 1..4 switch cycling snooze / turning-
// wheel / (on phase 0x26) the machine-slot distribution: pick a machine column from
// the config percent (m_4cc), scan its highlight rows, and either RETRACT (empty slot
// found) or DROP (full), arming m_groupSlots[col] + playing the cue. Then set the
// conveyor-belt timer, feed the snooze/lever stat, and refresh the display object.
// @early-stop
// complete reconstruction; residual is the 64-bit draw-clock gates + MSVC cross-jump/
// tail-merge of the shared GetIntDef/SetStatBar sequences + zero-register pinning +
// shared-global DIR32 naming (g_gameReg/g_dat645588/g_buteMgr/g_sndEnabled). Walls.
RVA(0x00105e40, 0x62c)
void CSBI_RectOnly::LoadRezMachineConfig() {
    SbiPhaseSlot* pA = (SbiPhaseSlot*)&m_hudRectB_x;
    SbiPhaseSlot* pB = (SbiPhaseSlot*)&m_hudRectA_x;
    SbiPhaseSlot* g = (SbiPhaseSlot*)m_groupSlots;
    if (pA->m_state == 5) {
        if ((i64)(u32)g_dat645588 - pA->m_last >= pA->m_interval) {
            if (++pA->m_counter > 0x34) {
                SetGaugeSpan(
                    0x2b,
                    5,
                    g_buteMgr.GetIntDef("StatusBar", "RightMachineRunningDelay", 0x7d)
                );
            } else {
                pA->m_interval = g_buteMgr.GetIntDef("StatusBar", "RightMachineRunningDelay", 0x7d);
                pA->m_last = (u32)g_dat645588;
            }
        }
    } else if (pA->m_state == 6) {
        if ((i64)(u32)g_dat645588 - pA->m_last >= pA->m_interval) {
            if (++pA->m_counter > 0x44) {
                SetGaugeSpan(0x2b, 0, 0x7fffffff);
            } else {
                pA->m_interval = g_buteMgr.GetIntDef("StatusBar", "RightMachineSpewingDelay", 0x7d);
                pA->m_last = (u32)g_dat645588;
            }
        }
    }

    switch (pB->m_state) {
        case 1:
            if ((i64)(u32)g_dat645588 - pB->m_last >= pB->m_interval) {
                if (++pB->m_counter > 8) {
                    SetStatBar(
                        1,
                        1,
                        g_buteMgr.GetIntDef("StatusBar", "LeftMachineSnoozingDelay", 0x64)
                    );
                } else {
                    pB->m_interval =
                        g_buteMgr.GetIntDef("StatusBar", "LeftMachineSnoozingDelay", 0x64);
                    pB->m_last = (u32)g_dat645588;
                }
            }
            break;
        case 2:
            if ((i64)(u32)g_dat645588 - pB->m_last >= pB->m_interval) {
                if (++pB->m_counter > 0x13) {
                    SetStatBar(
                        0x14,
                        3,
                        g_buteMgr.GetIntDef("StatusBar", "LeftMachineTurningWheelDelay", 0x64)
                    );
                    SetGaugeSpan(
                        0x2b,
                        5,
                        g_buteMgr.GetIntDef("StatusBar", "RightMachineRunningDelay", 0x7d)
                    );
                    CSbiSlot* s = m_groupSlots;
                    for (i32 i = 0; i < 3; i++) {
                        s->m_state = 1;
                        s->m_value = 1;
                        s++;
                    }
                    m_machinePhase = 2;
                    m_beltInterval = g_buteMgr.GetIntDef("StatusBar", "NextItemDelay", 0x64);
                    m_beltLast = (u32)g_dat645588;
                    if (m_activeTab == 3 && *(i32*)this != 2) {
                        CSbiMusicHost* host = g_gameReg->m_world->m_28;
                        if (host->m_30 == 0) {
                            void* found = 0;
                            (&host->m_map10)->Lookup("GAME_REZMACHINE", &found);
                            if (found && g_61ab20 != 0) {
                                i32 item = g_61ab24;
                                CSbiCueRecord* p = (CSbiCueRecord*)found;
                                if (g_6bf3c0 - (u32)p->m_14 >= (u32)p->m_18) {
                                    p->m_14 = g_6bf3c0;
                                    p->m_10->ConfigureItem(item, 0, 0, 0);
                                }
                            }
                        }
                    }
                } else {
                    pB->m_interval =
                        g_buteMgr.GetIntDef("StatusBar", "LeftMachineWakingDelay", 0x64);
                    pB->m_last = (u32)g_dat645588;
                }
            }
            break;
        case 3:
            if ((i64)(u32)g_dat645588 - pB->m_last >= pB->m_interval) {
                if (++pB->m_counter > 0x1d) {
                    SetStatBar(
                        0x14,
                        3,
                        g_buteMgr.GetIntDef("StatusBar", "LeftMachineTurningWheelDelay", 0x64)
                    );
                } else {
                    pB->m_interval =
                        g_buteMgr.GetIntDef("StatusBar", "LeftMachineTurningWheelDelay", 0x64);
                    pB->m_last = (u32)g_dat645588;
                }
            }
            break;
        case 4:
            if ((i64)(u32)g_dat645588 - pB->m_last >= pB->m_interval) {
                if (++pB->m_counter == 0x26) {
                    i32 col;
                    i32 which = m_extraNotifyArg0;
                    if (which >= 0x22) {
                        col = 2;
                    } else {
                        col = (which >= 0x17) ? 1 : 0;
                    }
                    i32 found = 0;
                    for (i32 r = 3; r >= 0; r--) {
                        if (m_hlGrid[col * 4 + r].m_state == 0) {
                            found = 1;
                            break;
                        }
                    }
                    if (found) {
                        g[col].m_state = 4;
                        g[col].m_counter = 0x13;
                        if (m_activeTab == 3 && *(i32*)this != 2) {
                            CSbiMusicHost* host = g_gameReg->m_world->m_28;
                            if (host->m_30 == 0) {
                                void* fnd = 0;
                                (&host->m_map10)->Lookup("GAME_REZBELTRETRACT", &fnd);
                                if (fnd && g_61ab20 != 0) {
                                    i32 item = g_61ab24;
                                    CSbiCueRecord* p = (CSbiCueRecord*)fnd;
                                    if (g_6bf3c0 - (u32)p->m_14 >= (u32)p->m_18) {
                                        p->m_14 = g_6bf3c0;
                                        p->m_10->ConfigureItem(item, 0, 0, 0);
                                    }
                                }
                            }
                        }
                    } else {
                        g[col].m_state = 2;
                        g[col].m_counter = 0xa;
                        if (m_activeTab == 3 && *(i32*)this != 2) {
                            CSbiMusicHost* host = g_gameReg->m_world->m_28;
                            if (host->m_30 == 0) {
                                void* fnd = 0;
                                (&host->m_map10)->Lookup("GAME_REZBELTDROP", &fnd);
                                if (fnd && g_61ab20 != 0) {
                                    i32 item = g_61ab24;
                                    CSbiCueRecord* p = (CSbiCueRecord*)fnd;
                                    if (g_6bf3c0 - (u32)p->m_14 >= (u32)p->m_18) {
                                        p->m_14 = g_6bf3c0;
                                        p->m_10->ConfigureItem(item, 0, 0, 0);
                                    }
                                }
                            }
                        }
                    }
                    g[0].m_interval = g_buteMgr.GetIntDef("StatusBar", "ConveyorBeltDelay", 0x64);
                    g[0].m_last = (u32)g_dat645588;
                    if (pB->m_counter > 0x2a) {
                        SetStatBar(
                            1,
                            1,
                            g_buteMgr.GetIntDef("StatusBar", "LeftMachineSnoozingDelay", 0x64)
                        );
                    } else {
                        pB->m_interval =
                            g_buteMgr.GetIntDef("StatusBar", "LeftMachineLeverDelay", 0x64);
                        pB->m_last = (u32)g_dat645588;
                    }
                }
            }
            break;
    }

    if (m_348) {
        m_348->Update(pB->m_counter, pA->m_counter);
    }
}

// 0x106660 - snooze phase of the rez-machine status bar: pull the LeftMachineSnoozing
// delay from the StatusBar config, feed it to the stat bar (slot 1,1) and reset the
// gauge span, refresh the snooze display object (if present) from the HUD-rect A/B
// y-coords, then clear the snooze/wake state pair.
RVA(0x00106660, 0x68)
void CSBI_RectOnly::UpdateRezMachineSnoozeStatusBar() {
    SetStatBar(1, 1, g_buteMgr.GetIntDef("StatusBar", "LeftMachineSnoozingDelay", 100));
    SetGaugeSpan(0x2b, 0, 0x7fffffff);
    if (m_348) {
        m_348->Update(m_hudRectA_y, m_hudRectB_y);
    }
    m_rezActive = 0;
    m_rezTick = 0;
}

// 0x106bb0 - drive the chip-machine status bar: a 7-phase (m_machinePhase = 2..8) switch on a
// single 64-bit belt timer (+0x4d0). Each phase accumulates the NextItem/FallingItem
// speed/delay config into the rect-corner running sums (m_itemRectL/m_itemRectT/m_itemRectR/m_itemRectB),
// advances the phase on span thresholds, plays CHIPLAND/CHIPFALLOUT cues on the gauge
// tab, and picks a machine column from m_4cc. A rect-write flag and a refresh flag are
// latched per phase; the tail pushes the composed rect into the falling-item notifier
// and fires the fall-rect refresh.
// @early-stop
// complete reconstruction; residual is the 64-bit belt gates + heavy MSVC cross-jump/
// tail-merge of the shared GetSpeed/GetIntDef sequences and the two flag-set tails +
// zero-register pinning + jump-table reloc typing + shared-global DIR32 naming. Walls.
RVA(0x00106bb0, 0x7bc)
void CSBI_RectOnly::LoadChipMachineConfig() {
    i32 refreshFlag = 0;
    i32 rectFlag = 0;
    switch (m_machinePhase) {
        case 2:
            if ((i64)(u32)g_dat645588 - m_beltLast >= m_beltInterval) {
                m_itemRectL += g_buteMgr.GetIntDef("StatusBar", "NextItemSpeed", 2);
                m_itemRectR += g_buteMgr.GetIntDef("StatusBar", "NextItemSpeed", 2);
                m_beltInterval = g_buteMgr.GetIntDef("StatusBar", "NextItemDelay", 0x64);
                m_beltLast = (u32)g_dat645588;
            }
            if (m_itemRectL >= 0x6d) {
                m_itemRectL = 0x6d;
                m_itemRectR = 0x84;
                m_machinePhase = 3;
                m_beltInterval = g_buteMgr.GetIntDef("StatusBar", "NextItemInMachineTime", 0x7d0);
                m_beltLast = (u32)g_dat645588;
            }
            refreshFlag = 1;
            break;
        case 3:
            if ((i64)(u32)g_dat645588 - m_beltLast >= m_beltInterval) {
                SetGaugeSpan(
                    0x35,
                    6,
                    g_buteMgr.GetIntDef("StatusBar", "RightMachineSpewingDelay", 0x7d)
                );
                m_machinePhase = 4;
                m_beltInterval = g_buteMgr.GetIntDef("StatusBar", "NextItemWaitTime", 0x1f4);
                m_beltLast = (u32)g_dat645588;
            }
            break;
        case 4:
            if ((i64)(u32)g_dat645588 - m_beltLast >= m_beltInterval) {
                m_machinePhase = 5;
                if (m_activeTab == 3 && *(i32*)this != 2) {
                    CSbiMusicHost* host = g_gameReg->m_world->m_28;
                    if (host->m_30 == 0) {
                        void* found = 0;
                        (&host->m_map10)->Lookup("GAME_CHIPFALLOUT", &found);
                        if (found && g_61ab20 != 0) {
                            i32 item = g_61ab24;
                            CSbiCueRecord* p = (CSbiCueRecord*)found;
                            if (g_6bf3c0 - (u32)p->m_14 >= (u32)p->m_18) {
                                p->m_14 = g_6bf3c0;
                                p->m_10->ConfigureItem(item, 0, 0, 0);
                            }
                        }
                    }
                }
                m_beltInterval = g_buteMgr.GetIntDef("StatusBar", "FallingItemDelay", 0x32);
                m_beltLast = (u32)g_dat645588;
            }
            break;
        case 5:
            if ((i64)(u32)g_dat645588 - m_beltLast >= m_beltInterval) {
                m_itemRectT += g_buteMgr.GetIntDef("StatusBar", "FallingItemSpeed", 2);
                m_itemRectB += g_buteMgr.GetIntDef("StatusBar", "FallingItemSpeed", 2);
                m_beltInterval = g_buteMgr.GetIntDef("StatusBar", "FallingItemDelay", 0x32);
                m_beltLast = (u32)g_dat645588;
            }
            if (m_itemRectB >= 0x11c) {
                m_itemRectB = 0x11c;
                m_itemRectT = 0x104;
                rectFlag = 1;
                if (m_activeTab == 3 && *(i32*)this != 2) {
                    CSbiMusicHost* host = g_gameReg->m_world->m_28;
                    if (host->m_30 == 0) {
                        void* found = 0;
                        (&host->m_map10)->Lookup("GAME_CHIPLAND", &found);
                        if (found && g_61ab20 != 0) {
                            i32 item = g_61ab24;
                            CSbiCueRecord* p = (CSbiCueRecord*)found;
                            if (g_6bf3c0 - (u32)p->m_14 >= (u32)p->m_18) {
                                p->m_14 = g_6bf3c0;
                                p->m_10->ConfigureItem(item, 0, 0, 0);
                            }
                        }
                    }
                }
                m_machinePhase = 7;
                m_beltInterval = g_buteMgr.GetIntDef("StatusBar", "NextItemDelay", 0x64);
                m_beltLast = (u32)g_dat645588;
                if (m_extraNotifyArg0 >= 0x22) {
                    m_itemBaseX = 0x6d;
                } else if (m_extraNotifyArg0 >= 0x17) {
                    m_itemBaseX = 0x45;
                } else {
                    m_itemBaseX = 0x1d;
                }
            }
            refreshFlag = 1;
            break;
        case 7:
            if ((i64)(u32)g_dat645588 - m_beltLast >= m_beltInterval) {
                m_itemRectL -= g_buteMgr.GetIntDef("StatusBar", "NextItemSpeed", 2);
                m_itemRectR -= g_buteMgr.GetIntDef("StatusBar", "NextItemSpeed", 2);
                m_beltInterval = g_buteMgr.GetIntDef("StatusBar", "NextItemDelay", 0x64);
                m_beltLast = (u32)g_dat645588;
            }
            if (m_itemRectL <= m_itemBaseX) {
                m_itemRectL = m_itemBaseX;
                m_itemRectR = m_itemBaseX + 0x17;
                rectFlag = 1;
                ChipNotify27f7();
                SetStatBar(
                    0x1e,
                    4,
                    g_buteMgr.GetIntDef("StatusBar", "LeftMachineLeverDelay", 0x64)
                );
                m_machinePhase = 1;
            }
            refreshFlag = 1;
            break;
        case 8: {
            if ((i64)(u32)g_dat645588 - m_beltLast >= m_beltInterval) {
                m_itemRectT += g_buteMgr.GetIntDef("StatusBar", "FallingItemSpeed", 2);
                m_itemRectB += g_buteMgr.GetIntDef("StatusBar", "(FallingItemSpeed", 2);
                m_beltInterval = g_buteMgr.GetIntDef("StatusBar", "FallingItemDelay", 0x32);
                m_beltLast = (u32)g_dat645588;
            }
            i32 col;
            if (m_extraNotifyArg0 >= 0x22) {
                col = 2;
            } else {
                col = (m_extraNotifyArg0 >= 0x17) ? 1 : 0;
            }
            i32 row = 3;
            while (m_hlGrid[col * 4 + row].m_state == 1) {
                row--;
                if (row < 0) {
                    break;
                }
            }
            if (m_itemRectT >= row * 0x20 + 0x13e) {
                if (m_activeTab == 3 && *(i32*)this != 2) {
                    CSbiMusicHost* host = g_gameReg->m_world->m_28;
                    if (host->m_30 == 0) {
                        void* found = 0;
                        (&host->m_map10)->Lookup("GAME_CHIPLAND", &found);
                        if (found && g_61ab20 != 0) {
                            i32 item = g_61ab24;
                            CSbiCueRecord* p = (CSbiCueRecord*)found;
                            if (g_6bf3c0 - (u32)p->m_14 >= (u32)p->m_18) {
                                p->m_14 = g_6bf3c0;
                                p->m_10->ConfigureItem(item, 0, 0, 0);
                            }
                        }
                    }
                }
                ChipFinish(col, m_extraNotifyArg0, row);
                ConveyorReturn();
            }
            refreshFlag = 1;
            break;
        }
    }

    if (m_extraNotify0) {
        if (rectFlag) {
            m_extraNotify0->m_rect14[0] = m_itemRectL + m_10;
            m_extraNotify0->m_rect14[1] = m_itemRectT + m_rect14.m_0;
            m_extraNotify0->m_rect14[2] = m_itemRectR + m_10;
            m_extraNotify0->m_rect14[3] = m_itemRectB + m_rect14.m_0;
        }
        if (refreshFlag) {
            RefreshFallRect();
        }
    }
}

// 0x107590 - configure the falling-item HUD gauge from a center point (a2,a3). Latch
// the notify arg + active flag, pull the FallingItemDelay config, seed the rect base
// from g_dat645588, build the relative +/-0xc rect (m_fallRectL block), and - if the notify
// object exists - write the absolute rect (offset by the item base coords m_10/m_14)
// into its +0x14 slot. Finish with the fall-rect refresh helper.
// @early-stop
// ~77%: logic + every field store/arithmetic is byte-correct. Residual is a regalloc/
// store-schedule wall in the notify-object rect block: retail pins the notify ptr in
// ebp (callee-saved) + spills rect0 to reserve eax for m_10, then stores the 4 rect
// ints in index order 0/1/2/3; MSVC5 here keeps the ptr in eax with ebp free (no
// spill) and stores 0/2/1/3. A register-assignment coin-flip (compute-all-then-store
// spelling regressed it to 69%); not steerable from C. Deferred to the final sweep.
RVA(0x00107590, 0xc4)
i32 CSBI_RectOnly::UpdateFallingItemStatusBar(i32 a1, i32 a2, i32 a3) {
    m_extraNotifyArg1 = a1;
    m_fallActive = 1;
    m_fallDelay = g_buteMgr.GetIntDef("StatusBar", "FallingItemDelay", 0x32);
    m_fallDelayHi = 0;
    m_fallLast = g_dat645588;
    m_fallLastHi = 0;
    CSbiSlotPtr* n = m_extraNotify1;
    i32 l = a2 - 0xc;
    i32 t = a3 - 0xc;
    i32 rr = a2 + 0xc;
    i32 b = a3 + 0xc;
    m_fallRectL = l;
    m_fallRectT = t;
    m_fallRectR = rr;
    m_fallRectB = b;
    if (n) {
        i32 x = m_10;
        n->m_rect14[0] = l + x;
        n->m_rect14[2] = x + rr;
        i32 y = m_rect14.m_0;
        n->m_rect14[1] = t + y;
        n->m_rect14[3] = y + b;
    }
    RefreshFallRect();
    return 1;
}

// 0x107a10 - wake phase of the rez-machine status bar. On the first pass (state not
// yet set) bail unless the extra-notify arg is armed, else pull the LeftMachineWaking
// delay, feed the stat bar (slot 9,2), latch the state and return 1. On later passes
// just bump the wake tick counter and return 1.
// @early-stop
// ~96%: byte-exact except the `m_rezActive = 1; return 1;` tail - retail stores the
// immediate directly (mov [esi+0x528],1) then loads eax=1 separately, while MSVC5
// here reuses eax (mov eax,1; mov [esi+0x528],eax). A store-imm-vs-register-reuse
// regalloc coin-flip on the shared return constant; no C-level lever. Deferred.
RVA(0x00107a10, 0x62)
i32 CSBI_RectOnly::UpdateRezMachineWakeStatusBar() {
    if (m_rezActive == 0) {
        if (m_extraNotifyArg0 == 0) {
            return 0;
        }
        SetStatBar(9, 2, g_buteMgr.GetIntDef("StatusBar", "LeftMachineWakingDelay", 100));
        m_rezActive = 1;
        return 1;
    }
    m_rezTick++;
    return 1;
}

// 0x107ae0 - reset the item to the multiplayer/battlez starting layout: prep, refresh
// the subtype-2 cursor, force the tab to 5, arm it, clear the per-stat flags, reset the
// widgets; then (by game mode m_134) pre-fill the first N slots (N = StartingGruntz from
// the Multiplayer / Battlez config section) to (state=ready, value=0x1a); return each
// pooled ptr to the free-list, clear the +0x530 collection and the reset block, free the
// +0x54c notifier, and finish. The trailing i32 arg is ABI-accepted but unused.
// @early-stop
// ~90.5%: the prologue, tab-force, memset, both config loops, and the free-list return
// loop are byte-exact (the free-loop matched once m_ptrTable was typed void**). Residual:
// (1) the teardown tail reorders the m_2b0/2b8/2b4/2bc zero-block vs the m_retabNotify load / m_hlBusy
// store (a store-vs-load MSVC scheduling coin-flip; an explicit-temp rewrite was neutral)
// and (2) the g_gameReg + StartingGruntz/Multiplayer/Battlez string DIR32 naming. Not
// source-steerable; deferred to the final sweep.
RVA(0x00107ae0, 0x1aa)
void CSBI_RectOnly::LoadMultiplayerBattlezConfig(i32) {
    PrepMultiReset();
    if (*(i32*)this == kSubtypeTag) {
        TabSubtypeRefresh();
    }
    if (m_activeTab != 5) {
        NotifyTabExit();
        m_activeTab = 5;
    }
    ArmTab(5, 1);
    memset(m_statFlags, 0, sizeof(m_statFlags));
    ResetTabWidgets2b44();

    i32 mode = g_gameReg->m_134;
    if (mode == 2) {
        CSbiSlot* s = m_slots;
        for (i32 i = 0; i < g_buteMgr.GetIntDef("Multiplayer", "StartingGruntz", 0); i++) {
            s->m_value = kSlotCommitLevel;
            s->m_state = kSlotReady;
            s++;
        }
    } else if (mode == 3) {
        CSbiSlot* s = m_slots;
        for (i32 i = 0; i < g_buteMgr.GetIntDef("Battlez", "StartingGruntz", 0); i++) {
            s->m_value = kSlotCommitLevel;
            s->m_state = kSlotReady;
            s++;
        }
    }

    for (i32 j = 0; j < m_ptrCount; j++) {
        void* p = m_ptrTable[j];
        if (p) {
            void** node = (void**)((char*)p - g_freeListNodeBias);
            *node = g_freeList;
            g_freeList = node;
        }
    }
    m_ptrPool.RemoveAll(0, -1);
    m_2b0 = 0;
    m_2b8 = 0;
    m_2b4 = 0;
    m_2bc = 0;
    m_hlBusy = 0;
    if (m_retabNotify) {
        free(m_retabNotify);
        m_retabNotify = 0;
    }
    FinishReset1f6e();
    m_578 = 0;
    m_modeArmed = 0;
    FinishReset16ea();
}
