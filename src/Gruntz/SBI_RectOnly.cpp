#include <rva.h>
#include <Gruntz/StatusBarItem.h>
// SBI_RectOnly.cpp - Gruntz CSBI_RectOnly (C:\Proj\Gruntz).
// The constructor is matched byte-exact.
//
// CSBI_RectOnly derives from CStatusBarItem. The retail ctor inlines the base
// ctor (zeroing m_4/m_24/m_28; the base's m_8=0 store is dropped as dead because
// the derived ctor sets m_8=1), then stores its own vptr, then m_8=1. That fold
// is exactly why CStatusBarItem's ctor is inline in the shared header (MSVC 5.0
// will not fold an out-of-line base ctor).

// ---------------------------------------------------------------------------
// CSBI_RectOnly - the rect-only status-bar item. A large, deeply-derived member
// of the CStatusBarItem family. Fields are named from how the matched methods
// use them; the remaining m_<hexoffset> placeholders are the ones whose role no
// reconstructed use-site pins. The class carries a 24-byte (0x18) slot-struct
// array (m_slots) at +0x208, a notify-pointer array (m_slotNotify) at +0x204, a
// tab index (m_activeTab) at +0x10c, the gauge pair (m_gauge/m_gaugeTarget) at
// +0x298/0x29c, and the mode toggle pair at +0x558/0x55c (m_modeState). Offsets
// + code bytes remain the load-bearing fact; names are codegen-neutral.
//
// MEMBERSHIP NOTE: the this-pointer tracer assigned ~48 RVAs to CSBI_RectOnly.
// They all operate on this same large layout (offsets up to ~0x630) and the
// dtor at 0x102000 walks the vtable chain that ends at CSBI_RectOnly's vtable
// 0x5eab8c - so they are reconstructed here as one class. Sibling-method calls
// go through the ILT, so their call rel32 is reloc-masked regardless of name.
// ---------------------------------------------------------------------------

// A 24-byte (0x18) slot record: the +0x208 array element.
// m_state: 0 = armed, 2 = ready (see kSlotArmed/kSlotReady).
struct CSbiSlot {
    i32 m_state; // +0x00 (rel +0x208)
    i32 m_value; // +0x04 (rel +0x20c)
    char m_pad8[0x18 - 0x8];
};

// A 24-byte highlight-row record (the +0x378 array element).
struct CSbiHlRow {
    i32 m_state;  // +0x00 state
    i32 m_handle; // +0x04 handle value passed to the notify pointer
    char m_pad8[0x18 - 0x8];
};

// The CSBI serialization stream (archive). Slot 0x30 (index 12) transfers n
// bytes to/from buf; slot 0x2c (index 11) reads n bytes and returns a status.
// Only the slot offsets are load-bearing (the virtual call is reloc-masked), as
// in the already-matched CTileGridCommand::Serialize.
class CSbiStream {
public:
    virtual void Slot00();
    virtual void Slot04();
    virtual void Slot08();
    virtual void Slot0C();
    virtual void Slot10();
    virtual void Slot14();
    virtual void Slot18();
    virtual void Slot1C();
    virtual void Slot20();
    virtual void Slot24();
    virtual void Slot28();
    virtual i32 Read(void* buf, i32 n);      // +0x2c (slot 11)
    virtual void Transfer(void* buf, i32 n); // +0x30 (slot 12)
};

// The +0x8 object carries a sequence id at +0x188 (read during serialize).
struct CSbiSeqHolder {
    char m_pad0[0x188];
    i32 m_188; // +0x188
};

// A per-tab sprite widget: the reconstructed bodies only call its Release().
class CSbiSprite {
public:
    void Release(); // __thiscall, no args (sibling thunk_FUN_004e84f0)
};

// A hit-test rect widget held in m_hitRects[]: m_enabled gate, m_xLo/m_xHi the
// x span, m_yLo/m_yHi the y span.
struct CSbiRect {
    char m_pad0[0x4];
    i32 m_enabled; // +0x04 enabled
    char m_pad8[0x14 - 0x8];
    i32 m_xLo; // +0x14 x lo
    i32 m_yLo; // +0x18 y lo
    i32 m_xHi; // +0x1c x hi
    i32 m_yHi; // +0x20 y hi
};

// The element type of the +0x204 pointer array: an engine object whose vtable
// slot 0x30 (index 12) is a __thiscall void(int) notifier.
class CSbiSlotPtr {
public:
    virtual void v0();
    virtual void v4();
    virtual void v8();
    virtual void vc();
    virtual void v10();
    virtual void v14();
    virtual void v18();
    virtual void v1c();
    virtual void v20();
    virtual void v24();
    virtual void v28();
    virtual void v2c();
    virtual void Notify(i32 on); // +0x30 (slot 12)
};

// A singly-linked notify node (the +0xbc list element): next ptr at +0, the
// payload object at +8. The payload's vtable slot 0 is a __thiscall void(int).
class CSbiNotifyTarget {
public:
    virtual void Notify(i32 on); // slot 0 (__thiscall void(int))
};
struct CSbiNotifyNode {
    CSbiNotifyNode* m_next; // +0
    char m_pad4[0x8 - 0x4];
    CSbiNotifyTarget* m_payload; // +8
};

// A minimal MFC-style CPtrList view (head node at +4); only RemoveAll is called.
struct CSbiPtrList {
    char m_pad0[0x4];
    CSbiNotifyNode* m_head; // +0x04 head node pointer
    void RemoveAll();       // __thiscall (sibling thunk)
};

// The pooled-ptr collection embedded at +0x530; teardown calls RemoveAll(-1, 0).
struct CSbiPtrCollection {
    char m_pad0[0x4];
    void RemoveAll(i32 a, i32 b); // __thiscall, 2 args
};

// The gauge notifier (m_218/m_21c): the value sink carries m_44 (set to the gauge
// reading) and a refresh slot at vtable index 0x28 (slot 10).
class CSbiGaugeNotify {
public:
    virtual void v0();
    virtual void v4();
    virtual void v8();
    virtual void vc();
    virtual void v10();
    virtual void v14();
    virtual void v18();
    virtual void v1c();
    virtual void v20();
    virtual void v24();
    virtual void v28(); // +0x28 (slot 10) refresh
    char m_pad4[0x44 - 0x4];
    i32 m_44; // +0x44  latched gauge reading
};

// Slot state values (CSbiSlot::m_state) named from how every site reads/writes
// them: ArmSlot/ResetGroupA store kSlotArmed; FindReadySlot looks for kSlotReady.
enum SbiSlotState {
    kSlotArmed = 0,
    kSlotReady = 2,
};

// CommitSlot stores this cooked level into the active slot's value and forwards
// it to the slot's notifier.
const i32 kSlotCommitLevel = 0x1a;

// Offset-0 subtype tag TryActivate gates on (read raw from slot 0; see the note
// at the use-site and the de-hack flag in the report).
const i32 kSubtypeTag = 2;

// The error-report id pair TryActivate passes to the game registry when the
// activation probe fails (resource/message id + source-line tag; raw ids).
const i32 kActivateErrId = 0x80e4;
const i32 kActivateErrTag = 0x44b;

// SetTab's own report tag (same error id, line tag 0x44a).
const i32 kSetTabErrTag = 0x44a;

// base vftable (CStatusBarItem) anchored out-of-line in this TU.
class CSBI_RectOnly : public CStatusBarItem {
public:
    CSBI_RectOnly();
    virtual i32 SbiVfunc0() OVERRIDE;

    // ----- reconstructed CSBI_RectOnly methods (RVA-ascending) -----
    void ResetCounters();
    void ResetSlots();
    void ArmSlot(i32 idx);
    i32 AnySlotActive();
    void AdvanceGauge(i32 delta);
    void SetGauge(i32 value);
    void RefreshAll();
    void Reset();
    void ToggleStat(i32 idx);
    void SetHudRectA(i32 y0, i32 x0, i32 z);
    void SetHudRectB(i32 y0, i32 x0, i32 z);
    void CommitSlot(i32 active);
    void ClearHlCell(i32 row, i32 group);
    i32 SetHlCell(i32 row, i32 handle, i32 group);
    i32 FindReadySlot();
    void SetMode(i32 mode);

    // Engine-label backlog stubs.
    void Stub_0ffde0();
    i32 winapi_107d00_SetRect();
    void LoadBattlezItemConfig(i32);
    void LoadMainStatusBarSprite();
    void UpdateStatusBarTabHighlight(i32, i32, i32);
    void LoadDestructButtonSprite(i32);
    void BuildGameTabResumeButton(i32);
    void BuildGameTabPauseButton();
    void LoadGooCookingSprite(i32);
    void UpdateRezConveyorStatusBar();
    void LoadRezMachineConfig();
    void UpdateRezMachineSnoozeStatusBar();
    void LoadChipMachineConfig();
    void UpdateFallingItemStatusBar(i32, i32, i32);
    void UpdateRezMachineWakeStatusBar();
    void LoadMultiplayerBattlezConfig(i32);

    // ----- sibling methods called by the reconstructed bodies (declared so the
    // ILT call targets resolve; bodies live elsewhere / are stubbed) -----
    i32 ProbeSlot(i32 idx);
    void RebuildGroupA();
    void ResetGroupA();
    void ClearStatToggle(i32);
    void LoadStatzTabToggleSprite(i32, i32);
    void UpdateGruntOvenStatusBar();
    void TickGauge();
    i32 GaugeComplete();   // call 0x3e2c - gauge-at-100 completion test
    void GaugeFinish(i32); // call 0x39ef - completion hook
    void UpdateChipGrinderStatusBar();
    void UpdateDestructButtonStatusBar();
    i32 Activate();
    i32 Stub_0ffde0_probe();
    void SetTabState(i32 tab, i32 state);

    void Teardown();
    void TeardownNotify(i32); // call 0x12fd - pre-teardown notify (1 arg)
    i32 TryActivate();
    i32 Deactivate();
    i32 HlClickGroup0(i32 row);
    i32 HlClickGroup1(i32 row);
    i32 HlClickGroup2(i32 row);
    void* ResolveHandle(i32 handle);  // call 0x17a8 - validity probe
    void SetCursorRect(i32 x, i32 y); // call 0x3878 (__thiscall, 2 args)
    i32 SetTab(i32 tab, i32 flag);
    i32 TabRefresh(); // call 0x1690 - activation probe (returns int)
    void TabCommit(); // call 0x125d - success helper
    i32 ClearTabSprites(i32 idx);
    i32 HitTest(i32 x, i32 y);
    i32 Serialize(CSbiStream* s);
    void NotifyAllSlots();

    // ----- layout (placeholders; offsets are the load-bearing fact) -----
    char m_pad2c[0x30 - 0x2c];
    i32 m_30; // +0x30
    i32 m_34; // +0x34
    char m_pad38[0xb8 - 0x38];
    CSbiPtrList m_listB8; // +0xb8  per-tab notify list (RemoveAll'd on retab)
    char m_padc0[0x10c - 0xc0];
    i32 m_activeTab; // +0x10c  active tab index
    char m_pad110[0x114 - 0x110];
    i32 m_statFlags[1]; // +0x114  per-stat toggle flag array
    char m_pad118[0x150 - 0x118];
    CSbiRect* m_hitRects[15]; // +0x150  hit-test rect widgets
    char m_pad18c[0x1c8 - 0x18c];
    CSbiSprite* m_1c8;            // +0x1c8  per-tab sprite widgets (cleared by
    CSbiSprite* m_1cc;            // +0x1cc  ClearTabSprites in declaration order)
    CSbiSprite* m_1d0;            // +0x1d0
    CSbiSprite* m_1d4;            // +0x1d4
    CSbiSprite* m_1d8;            // +0x1d8
    CSbiSprite* m_1dc;            // +0x1dc
    CSbiSprite* m_1e0;            // +0x1e0
    CSbiSprite* m_1e4;            // +0x1e4
    CSbiSprite* m_1e8;            // +0x1e8
    CSbiSprite* m_1ec;            // +0x1ec
    CSbiSprite* m_1f0;            // +0x1f0
    CSbiSprite* m_1f4;            // +0x1f4
    CSbiSprite* m_1f8;            // +0x1f8
    CSbiSprite* m_1fc;            // +0x1fc
    CSbiSprite* m_200;            // +0x200
    CSbiSlotPtr* m_slotNotify[1]; // +0x204  slot pointer array (4-byte stride)
    char m_pad208[0x218 - 0x208];
    CSbiGaugeNotify* m_218; // +0x218  gauge notifier (vfunc 0x28)
    CSbiGaugeNotify* m_21c; // +0x21c  gauge value sink (m_44 = gauge; vfunc 0x28)
    CSbiSlot m_slots[1];    // +0x220  24-byte slot records
    char m_pad238[0x298 - (0x220 + 0x18)];
    i32 m_gauge;       // +0x298  gauge current
    i32 m_gaugeTarget; // +0x29c  gauge target
    char m_pad2a0[0x2c0 - 0x2a0];
    CSbiSlot m_groupSlots[3];      // +0x2c0  group-A 24-byte slot records
    CSbiSlotPtr* m_groupNotify[3]; // +0x308  group-A notify pointers
    char m_pad314[0x318 - 0x314];
    i32 m_hudRectB_x; // +0x318  HUD-rect group B (x0)
    i32 m_hudRectB_y; // +0x31c  (y0)
    i32 m_320;        // +0x320  (latched dword from g_dat645588)
    i32 m_324;        // +0x324
    i32 m_hudRectB_z; // +0x328
    i32 m_32c;        // +0x32c
    i32 m_hudRectA_x; // +0x330  HUD-rect group A (x0)
    i32 m_hudRectA_y; // +0x334  (y0)
    i32 m_338;        // +0x338
    i32 m_33c;        // +0x33c
    i32 m_hudRectA_z; // +0x340
    i32 m_344;        // +0x344
    char m_pad348[0x354 - 0x348];
    i32 m_hitTestDisabled; // +0x354  hit-test disable flag
    char m_pad358[0x35c - 0x358];
    i32 m_activeSlot;       // +0x35c  active-slot index (-1 = none)
    i32 m_360;              // +0x360  pending highlight row index (-1 none)
    CSbiSlotPtr* m_notify0; // +0x364  notify targets (slot 0x28)
    CSbiSlotPtr* m_notify1; // +0x368
    CSbiSlotPtr* m_notify2; // +0x36c
    CSbiSlotPtr* m_notify3; // +0x370
    char m_pad374[0x378 - 0x374];
    CSbiHlRow m_hlGrid[12];       // +0x378  3 groups x 4 highlight rows (24B each)
    CSbiSlotPtr* m_hlNotify[12];  // +0x498  3 groups x 4 notify pointers
    char m_pad4c8[0x4cc - 0x4c8]; // 0x498 + 12*4 = 0x4c8
    i32 m_extraNotifyArg0;        // +0x4cc  arg for (*m_extraNotify0)->Notify
    char m_pad4d0[0x4e0 - 0x4d0];
    CSbiSlotPtr* m_extraNotify0; // +0x4e0
    char m_pad4e4[0x4ec - 0x4e4];
    i32 m_extraNotifyArg1; // +0x4ec  arg for (*m_extraNotify1)->Notify
    char m_pad4f0[0x500 - 0x4f0];
    CSbiSlotPtr* m_extraNotify1; // +0x500
    char m_pad504[0x530 - 0x504];
    CSbiPtrCollection m_530; // +0x530  pooled-ptr collection (RemoveAll on teardown)
    void* m_ptrTable[1];     // +0x534  pointer table (elements streamed 8B)
    i32 m_ptrCount;          // +0x538  count for m_ptrTable
    char m_pad53c[0x558 - 0x53c];
    i32 m_558;       // +0x558
    i32 m_modeState; // +0x55c
    char m_pad560[0x570 - 0x560];
    CSbiSlotPtr* m_modeNotify; // +0x570  notify target
    i32 m_modeArmed;           // +0x574
    char m_pad578[0x630 - 0x578];
};

// An unnamed engine DWORD global read by the HUD-rect group setters.
DATA(0x00245588)
extern i32 g_dat645588;

// The cue-lookup string map embedded at host->m_28 + 0x10 (CMapStringToOb).
struct CSbiLookupMap {
    i32 Lookup(char* key, void** out); // CMapStringToOb::Lookup (ret 8)
};

// A resolved cue record: a player at +0x10 plus a draw-clock gate (+0x14 last,
// +0x18 interval). Same shape as GameMode's CBootyFound.
struct CSbiCueRecord {
    char m_pad0[0x10];
    void* m_10; // +0x10  player (ConfigureItem this)
    i32 m_14;   // +0x14  last draw-clock
    i32 m_18;   // +0x18  interval
};

// The cue player (ConfigureItem == FUN_005360d0, __thiscall, ret 0x10).
struct CSbiCuePlayer {
    void ConfigureItem(i32 item, i32 a, i32 b, i32 c);
};

// The music host chain: g_gameReg->m_gmgr->m_28->{m_30 gate, Lookup map @+0x10}.
struct CSbiMusicHost {
    char m_pad0[0x30];
    void* m_30; // +0x30  gate (non-null => skip the cue play)
};

// The active game manager (g_gameReg->m_gmgr): carries the music host at +0x28.
struct CSbiGameMgr {
    char m_pad0[0x28];
    CSbiMusicHost* m_28; // +0x28  music host
};

// The sub-manager at g_gameReg+0x2c that carries the highlight-busy gate at +0x4f0.
struct CSbiSubMgr {
    char m_pad0[0x4f0];
    i32 m_4f0; // +0x4f0  highlight-busy flag (non-zero => bail)
};

// The diagnostics logger at g_gameReg+0x38 (a position-string sink).
struct CSbiLogger {
    void LogPos(char* tag, i32 subtype); // __thiscall, 2 args
};

// The CGameReg singleton (?g_gameReg@@3PAUWwdGameReg@@A @ VA 0x64556c). Modeled
// minimally - only the members/methods the reconstructed bodies touch.
struct CGameReg {
    char m_pad0[0x2c];
    CSbiSubMgr* m_2c;  // +0x2c  highlight sub-manager
    CSbiGameMgr* m_30; // +0x30  active game-manager (null-guard in Serialize)
    char m_pad34[0x38 - 0x34];
    CSbiLogger* m_38; // +0x38  diagnostics logger
    char m_pad3c[0x8c - 0x3c];
    i32 m_8c; // +0x8c  cursor/view x (offset by -0x45 into the rect-only item)
    i32 m_90; // +0x90  cursor/view y (offset by -0x30)
    void ReportError(i32 a, i32 b); // __thiscall
};
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
// Reconstructed CSBI_RectOnly methods (RVA-ascending).
// ---------------------------------------------------------------------------

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
    m_558 = 0;
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
// reorders the m_33c/m_338 zero+global pair vs retail; ~72%, not steerable from
// C (see docs/patterns/u64-store-clock-hi-zero.md, statement-schedule-faithful).
RVA(0x001066f0, 0x3b)
void CSBI_RectOnly::SetHudRectA(i32 y0, i32 x0, i32 z) {
    m_hudRectA_y = y0;
    m_hudRectA_x = x0;
    m_hudRectA_z = z;
    m_344 = 0;
    m_338 = g_dat645588;
    m_33c = 0;
}

// Latch HUD-rect group B from three args + a global dword.
// @early-stop
// scheduling wall: same store-reorder as SetHudRectA; ~72%.
RVA(0x00106740, 0x3b)
void CSBI_RectOnly::SetHudRectB(i32 y0, i32 x0, i32 z) {
    m_hudRectB_y = y0;
    m_hudRectB_x = x0;
    m_hudRectB_z = z;
    m_32c = 0;
    m_320 = g_dat645588;
    m_324 = 0;
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
    if (g_gameReg->m_30 == 0) {
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
    if (g_gameReg->m_2c->m_4f0 == 0 && m_hlGrid[row].m_state == 1) {
        i32 handle = m_hlGrid[row].m_handle;
        i32* slot = &m_hlGrid[row].m_handle;
        if (ResolveHandle(handle)) {
            CSbiMusicHost* host = g_gameReg->m_30->m_28;
            if (host->m_30 == 0) {
                void* found = 0;
                CSbiLookupMap* map = (CSbiLookupMap*)((char*)host + 0x10);
                map->Lookup("GAME_TABHIGHLIGHT1", &found);
                if (found) {
                    i32 gate = g_61ab20;
                    i32 item = g_61ab24;
                    if (gate != 0) {
                        CSbiCueRecord* p = (CSbiCueRecord*)found;
                        if (g_6bf3c0 - (u32)p->m_14 >= (u32)p->m_18) {
                            p->m_14 = g_6bf3c0;
                            ((CSbiCuePlayer*)p->m_10)->ConfigureItem(item, 0, 0, 0);
                        }
                    }
                }
            }
            m_360 = row;
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
    if (g_gameReg->m_2c->m_4f0 == 0 && m_hlGrid[row + 4].m_state == 1) {
        i32 handle = m_hlGrid[row + 4].m_handle;
        i32* slot = &m_hlGrid[row + 4].m_handle;
        if (ResolveHandle(handle)) {
            CSbiMusicHost* host = g_gameReg->m_30->m_28;
            if (host->m_30 == 0) {
                void* found = 0;
                CSbiLookupMap* map = (CSbiLookupMap*)((char*)host + 0x10);
                map->Lookup("GAME_TABHIGHLIGHT1", &found);
                if (found) {
                    i32 gate = g_61ab20;
                    i32 item = g_61ab24;
                    if (gate != 0) {
                        CSbiCueRecord* p = (CSbiCueRecord*)found;
                        if (g_6bf3c0 - (u32)p->m_14 >= (u32)p->m_18) {
                            p->m_14 = g_6bf3c0;
                            ((CSbiCuePlayer*)p->m_10)->ConfigureItem(item, 0, 0, 0);
                        }
                    }
                }
            }
            m_360 = row;
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
    if (g_gameReg->m_2c->m_4f0 == 0 && m_hlGrid[row + 8].m_state == 1) {
        i32 handle = m_hlGrid[row + 8].m_handle;
        i32* slot = &m_hlGrid[row + 8].m_handle;
        if (ResolveHandle(handle)) {
            CSbiMusicHost* host = g_gameReg->m_30->m_28;
            if (host->m_30 == 0) {
                void* found = 0;
                CSbiLookupMap* map = (CSbiLookupMap*)((char*)host + 0x10);
                map->Lookup("GAME_TABHIGHLIGHT1", &found);
                if (found) {
                    i32 gate = g_61ab20;
                    i32 item = g_61ab24;
                    if (gate != 0) {
                        CSbiCueRecord* p = (CSbiCueRecord*)found;
                        if (g_6bf3c0 - (u32)p->m_14 >= (u32)p->m_18) {
                            p->m_14 = g_6bf3c0;
                            ((CSbiCuePlayer*)p->m_10)->ConfigureItem(item, 0, 0, 0);
                        }
                    }
                }
            }
            m_360 = row;
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
// ~95.7%: byte-exact except a 1-instr regalloc coin-flip in the m_21c->m_44 store
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
        if (m_21c && m_218) {
            m_218->v28();
            m_21c->m_44 = m_gauge;
            m_21c->v28();
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
        if (m_1c8) {
            m_1c8->Release();
        }
        if (m_1d0) {
            m_1d0->Release();
        }
        if (m_1cc) {
            m_1cc->Release();
        }
        if (m_1d4) {
            m_1d4->Release();
        }
        if (m_1d8) {
            m_1d8->Release();
        }
    }
    if (idx == 5 || idx == -1) {
        if (m_1dc) {
            m_1dc->Release();
        }
        if (m_1e0) {
            m_1e0->Release();
        }
        if (m_1e4) {
            m_1e4->Release();
        }
        if (m_1e8) {
            m_1e8->Release();
        }
        if (m_1ec) {
            m_1ec->Release();
        }
        if (m_1f0) {
            m_1f0->Release();
        }
    }
    if (idx == 6 || idx == -1) {
        if (m_1f4) {
            m_1f4->Release();
        }
        if (m_1f8) {
            m_1f8->Release();
        }
        if (m_1fc) {
            m_1fc->Release();
        }
        if (m_200) {
            m_200->Release();
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
        i32 a = g->m_8c;
        i32 b = g->m_90;
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
    m_1dc = 0;
    m_1e0 = 0;
    m_1e4 = 0;
    m_1e8 = 0;
    m_1ec = 0;
    m_1f0 = 0;
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
    m_530.RemoveAll(0, -1);
}

// Activate the rect-only item; gate on the offset-0 subtype tag and a probe.
RVA(0x00104d60, 0x48)
i32 CSBI_RectOnly::TryActivate() {
    // Offset-0 read: in retail this object's slot 0 holds a small integer
    // subtype tag (the manual-vtable-stamp device shared with CStatusBarMgr's
    // g_vtbl_t* tags), not a real C++ vptr. We model the vtable via `virtual`,
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
        m_558 = 0;
        m_modeState = 1;
        if (m_modeNotify) {
            m_modeNotify->Notify(1);
        }
    }
}

// -------------------------------------------------------------------------
// Engine-label backlog stubs.
// -------------------------------------------------------------------------
// @confidence: med
// @source: rtti-vptr
// @stub
RVA(0x000ffde0, 0x5b1)
void CSBI_RectOnly::Stub_0ffde0() {}

// @confidence: low
// @source: winapi:SetRect
// @stub
RVA(0x00107d00, 0x591)
i32 CSBI_RectOnly::winapi_107d00_SetRect() {
    return 0;
}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x000fdc00, 0x5c2)
void CSBI_RectOnly::LoadBattlezItemConfig(i32) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x000fe6b0, 0x145)
void CSBI_RectOnly::LoadMainStatusBarSprite() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x000fe910, 0xb8e)
void CSBI_RectOnly::UpdateStatusBarTabHighlight(i32, i32, i32) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x000ffb20, 0x13a)
void CSBI_RectOnly::LoadDestructButtonSprite(i32) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x00102180, 0x5f)
void CSBI_RectOnly::BuildGameTabResumeButton(i32) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x00102200, 0x37)
void CSBI_RectOnly::BuildGameTabPauseButton() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x001055b0, 0x109)
void CSBI_RectOnly::LoadGooCookingSprite(i32) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x00105990, 0x398)
void CSBI_RectOnly::UpdateRezConveyorStatusBar() {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x00105e40, 0x62c)
void CSBI_RectOnly::LoadRezMachineConfig() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x00106660, 0x68)
void CSBI_RectOnly::UpdateRezMachineSnoozeStatusBar() {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x00106bb0, 0x7bc)
void CSBI_RectOnly::LoadChipMachineConfig() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x00107590, 0xc4)
void CSBI_RectOnly::UpdateFallingItemStatusBar(i32, i32, i32) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x00107a10, 0x62)
void CSBI_RectOnly::UpdateRezMachineWakeStatusBar() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x00107ae0, 0x1aa)
void CSBI_RectOnly::LoadMultiplayerBattlezConfig(i32) {}
