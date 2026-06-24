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
    int m_state; // +0x00 (rel +0x208)
    int m_value; // +0x04 (rel +0x20c)
    char m_pad8[0x18 - 0x8];
};

// A 24-byte highlight-row record (the +0x378 array element).
struct CSbiHlRow {
    int m_state;  // +0x00 state
    int m_handle; // +0x04 handle value passed to the notify pointer
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
    virtual int Read(void* buf, int n);      // +0x2c (slot 11)
    virtual void Transfer(void* buf, int n); // +0x30 (slot 12)
};

// The +0x8 object carries a sequence id at +0x188 (read during serialize).
struct CSbiSeqHolder {
    char m_pad0[0x188];
    int m_188; // +0x188
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
    int m_enabled; // +0x04 enabled
    char m_pad8[0x14 - 0x8];
    int m_xLo; // +0x14 x lo
    int m_yLo; // +0x18 y lo
    int m_xHi; // +0x1c x hi
    int m_yHi; // +0x20 y hi
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
    virtual void Notify(int on); // +0x30 (slot 12)
};

// Slot state values (CSbiSlot::m_state) named from how every site reads/writes
// them: ArmSlot/ResetGroupA store kSlotArmed; FindReadySlot looks for kSlotReady.
enum SbiSlotState {
    kSlotArmed = 0,
    kSlotReady = 2,
};

// CommitSlot stores this cooked level into the active slot's value and forwards
// it to the slot's notifier.
const int kSlotCommitLevel = 0x1a;

// Offset-0 subtype tag TryActivate gates on (read raw from slot 0; see the note
// at the use-site and the de-hack flag in the report).
const int kSubtypeTag = 2;

// The error-report id pair TryActivate passes to the game registry when the
// activation probe fails (resource/message id + source-line tag; raw ids).
const int kActivateErrId = 0x80e4;
const int kActivateErrTag = 0x44b;

// base vftable (CStatusBarItem) anchored out-of-line in this TU.
class CSBI_RectOnly : public CStatusBarItem {
public:
    CSBI_RectOnly();
    virtual int SbiVfunc0() OVERRIDE;

    // ----- reconstructed CSBI_RectOnly methods (RVA-ascending) -----
    void ResetCounters();
    void ResetSlots();
    void ArmSlot(int idx);
    int AnySlotActive();
    void AdvanceGauge(int delta);
    void SetGauge(int value);
    void RefreshAll();
    void Reset();
    void ToggleStat(int idx);
    void SetHudRectA(int y0, int x0, int z);
    void SetHudRectB(int y0, int x0, int z);
    void CommitSlot(int active);
    int FindReadySlot();
    void SetMode(int mode);

    // Engine-label backlog stubs.
    void Stub_0ffde0();
    int winapi_107d00_SetRect();
    void LoadBattlezItemConfig(int);
    void LoadMainStatusBarSprite();
    void UpdateStatusBarTabHighlight(int, int, int);
    void LoadDestructButtonSprite(int);
    void BuildGameTabResumeButton(int);
    void BuildGameTabPauseButton();
    void LoadGooCookingSprite(int);
    void UpdateRezConveyorStatusBar();
    void LoadRezMachineConfig();
    void UpdateRezMachineSnoozeStatusBar();
    void LoadChipMachineConfig();
    void UpdateFallingItemStatusBar(int, int, int);
    void UpdateRezMachineWakeStatusBar();
    void LoadMultiplayerBattlezConfig(int);

    // ----- sibling methods called by the reconstructed bodies (declared so the
    // ILT call targets resolve; bodies live elsewhere / are stubbed) -----
    int ProbeSlot(int idx);
    void RebuildGroupA();
    void ResetGroupA();
    void ClearStatToggle(int);
    void LoadStatzTabToggleSprite(int, int);
    void UpdateGruntOvenStatusBar();
    void TickGauge();
    void UpdateChipGrinderStatusBar();
    void UpdateDestructButtonStatusBar();
    int Activate();
    int Stub_0ffde0_probe();
    void SetTabState(int tab, int state);

    int TryActivate();
    int ClearTabSprites(int idx);
    int HitTest(int x, int y);
    int Serialize(CSbiStream* s);
    void NotifyAllSlots();

    // ----- layout (placeholders; offsets are the load-bearing fact) -----
    char m_pad2c[0x30 - 0x2c];
    int m_30; // +0x30
    int m_34; // +0x34
    char m_pad38[0x10c - 0x38];
    int m_activeTab; // +0x10c  active tab index
    char m_pad110[0x114 - 0x110];
    int m_statFlags[1]; // +0x114  per-stat toggle flag array
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
    char m_pad208[0x220 - 0x208];
    CSbiSlot m_slots[1]; // +0x220  24-byte slot records
    char m_pad238[0x298 - (0x220 + 0x18)];
    int m_gauge;       // +0x298  gauge current
    int m_gaugeTarget; // +0x29c  gauge target
    char m_pad2a0[0x2c0 - 0x2a0];
    CSbiSlot m_groupSlots[3];      // +0x2c0  group-A 24-byte slot records
    CSbiSlotPtr* m_groupNotify[3]; // +0x308  group-A notify pointers
    char m_pad314[0x318 - 0x314];
    int m_hudRectB_x; // +0x318  HUD-rect group B (x0)
    int m_hudRectB_y; // +0x31c  (y0)
    int m_320;        // +0x320  (latched dword from g_dat645588)
    int m_324;        // +0x324
    int m_hudRectB_z; // +0x328
    int m_32c;        // +0x32c
    int m_hudRectA_x; // +0x330  HUD-rect group A (x0)
    int m_hudRectA_y; // +0x334  (y0)
    int m_338;        // +0x338
    int m_33c;        // +0x33c
    int m_hudRectA_z; // +0x340
    int m_344;        // +0x344
    char m_pad348[0x354 - 0x348];
    int m_hitTestDisabled; // +0x354  hit-test disable flag
    char m_pad358[0x35c - 0x358];
    int m_activeSlot;       // +0x35c  active-slot index (-1 = none)
    int m_360;              // +0x360  pending highlight row index (-1 none)
    CSbiSlotPtr* m_notify0; // +0x364  notify targets (slot 0x28)
    CSbiSlotPtr* m_notify1; // +0x368
    CSbiSlotPtr* m_notify2; // +0x36c
    CSbiSlotPtr* m_notify3; // +0x370
    char m_pad374[0x378 - 0x374];
    CSbiHlRow m_hlGrid[12];       // +0x378  3 groups x 4 highlight rows (24B each)
    CSbiSlotPtr* m_hlNotify[12];  // +0x498  3 groups x 4 notify pointers
    char m_pad4c8[0x4cc - 0x4c8]; // 0x498 + 12*4 = 0x4c8
    int m_extraNotifyArg0;        // +0x4cc  arg for (*m_extraNotify0)->Notify
    char m_pad4d0[0x4e0 - 0x4d0];
    CSbiSlotPtr* m_extraNotify0; // +0x4e0
    char m_pad4e4[0x4ec - 0x4e4];
    int m_extraNotifyArg1; // +0x4ec  arg for (*m_extraNotify1)->Notify
    char m_pad4f0[0x500 - 0x4f0];
    CSbiSlotPtr* m_extraNotify1; // +0x500
    char m_pad504[0x534 - 0x504];
    void* m_ptrTable[1]; // +0x534  pointer table (elements streamed 8B)
    int m_ptrCount;      // +0x538  count for m_ptrTable
    char m_pad53c[0x558 - 0x53c];
    int m_558;       // +0x558
    int m_modeState; // +0x55c
    char m_pad560[0x570 - 0x560];
    CSbiSlotPtr* m_modeNotify; // +0x570  notify target
    int m_modeArmed;           // +0x574
    char m_pad578[0x630 - 0x578];
};

// An unnamed engine DWORD global read by the HUD-rect group setters.
DATA(0x00245588)
extern int g_dat645588;

// The CGameReg singleton (?g_gameReg@@3PAUWwdGameReg@@A @ VA 0x64556c). Modeled
// minimally - only the members/methods the reconstructed bodies touch.
struct CGameReg {
    char m_pad0[0x30];
    void* m_30;                     // +0x30  active game-manager (null-guard in Serialize)
    void ReportError(int a, int b); // __thiscall
};
DATA(0x0024556c)
extern CGameReg* g_gameReg;

// Global serialize-sequence counter (bumped once per Serialize).
DATA(0x00229ad0)
extern int g_serialCounter;

CStatusBarItem::~CStatusBarItem() {}
int CStatusBarItem::SbiVfunc0() {
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

int CSBI_RectOnly::SbiVfunc0() {
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
    for (int i = 0; i < 5; i++) {
        ArmSlot(i);
    }
    m_activeSlot = -1;
}

// Arm slot[idx]: state = armed, value = 1; notify the slot's pointer (vfunc 0x30).
RVA(0x00105560, 0x33)
void CSBI_RectOnly::ArmSlot(int idx) {
    m_slots[idx].m_state = kSlotArmed;
    m_slots[idx].m_value = 1;
    if (m_slotNotify[idx]) {
        m_slotNotify[idx]->Notify(1);
    }
}

// Probe slots 0..4; return 1 on first hit, else 0.
RVA(0x00105710, 0x23)
int CSBI_RectOnly::AnySlotActive() {
    for (int i = 0; i < 5; i++) {
        if (ProbeSlot(i)) {
            return 1;
        }
    }
    return 0;
}

// m_gaugeTarget = min(m_gauge + delta, 100).
RVA(0x00105750, 0x1f)
void CSBI_RectOnly::AdvanceGauge(int delta) {
    int v = m_gauge + delta;
    if (v >= 100) {
        v = 100;
    }
    m_gaugeTarget = v;
}

// m_gaugeTarget = m_gauge = value.
RVA(0x001057d0, 0x13)
void CSBI_RectOnly::SetGauge(int value) {
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
void CSBI_RectOnly::ToggleStat(int idx) {
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
int CSBI_RectOnly::HitTest(int x, int y) {
    if (m_hitTestDisabled == 0) {
        for (int i = 0; i < 15; i++) {
            CSbiRect* p = m_hitRects[i];
            if (p && p->m_enabled) {
                int hit =
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
    for (int i = 0; i < 3; i++) {
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
void CSBI_RectOnly::SetHudRectA(int y0, int x0, int z) {
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
void CSBI_RectOnly::SetHudRectB(int y0, int x0, int z) {
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
void CSBI_RectOnly::CommitSlot(int active) {
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
    int* h = &m_hlGrid[4].m_handle;   // anchor on the handle field (+0x3dc)
    for (int n = 0; n < 4; n++) {
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
int CSBI_RectOnly::Serialize(CSbiStream* s) {
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
    int seq = 0;
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
    for (int i = 0; i < 15; i++) {
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
    for (int j = 0; j < 5; j++) {
        s->Transfer(q - 4, 4);
        s->Transfer(q, 4);
        q += 0x18;
    }
    char* r = B + 0x2c4;
    for (int k = 0; k < 3; k++) {
        s->Transfer(r - 4, 4);
        s->Transfer(r, 4);
        r += 0x18;
    }
    char* nb = B + 0x378;
    int outer = 3;
    do {
        for (int m = 0; m < 4; m++) {
            s->Transfer(nb, 4);
            s->Transfer(nb + 4, 4);
            nb += 0x18;
        }
    } while (--outer);

    int count = m_ptrCount;
    s->Transfer(&count, 4);
    for (unsigned int n = 0; n < (unsigned int)count; n++) {
        s->Transfer(m_ptrTable[n], 8);
    }
    return 1;
}

// Find the first slot whose state is ready; re-arm it and report found.
RVA(0x00109a90, 0x25)
int CSBI_RectOnly::FindReadySlot() {
    for (int i = 0; i < 5; i++) {
        if (m_slots[i].m_state == kSlotReady) {
            ArmSlot(i);
            return 1;
        }
    }
    return 0;
}

// Release the per-tab sprite widgets for the given tab group (idx, with -1 = all).
RVA(0x00101420, 0x110)
int CSBI_RectOnly::ClearTabSprites(int idx) {
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

// Activate the rect-only item; gate on the offset-0 subtype tag and a probe.
RVA(0x00104d60, 0x48)
int CSBI_RectOnly::TryActivate() {
    // Offset-0 read: in retail this object's slot 0 holds a small integer
    // subtype tag (the manual-vtable-stamp device shared with CStatusBarMgr's
    // g_vtbl_t* tags), not a real C++ vptr. We model the vtable via `virtual`,
    // so slot 0 cannot also be a named field here without dropping the vtable;
    // the raw `*(int*)this` read is the faithful model. See report (flagged).
    if (*(int*)this == kSubtypeTag) {
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
void CSBI_RectOnly::SetMode(int mode) {
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
int CSBI_RectOnly::winapi_107d00_SetRect() {
    return 0;
}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x000fdc00, 0x5c2)
void CSBI_RectOnly::LoadBattlezItemConfig(int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x000fe6b0, 0x145)
void CSBI_RectOnly::LoadMainStatusBarSprite() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x000fe910, 0xb8e)
void CSBI_RectOnly::UpdateStatusBarTabHighlight(int, int, int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x000ffb20, 0x13a)
void CSBI_RectOnly::LoadDestructButtonSprite(int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x00102180, 0x5f)
void CSBI_RectOnly::BuildGameTabResumeButton(int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x00102200, 0x37)
void CSBI_RectOnly::BuildGameTabPauseButton() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x001055b0, 0x109)
void CSBI_RectOnly::LoadGooCookingSprite(int) {}

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
void CSBI_RectOnly::UpdateFallingItemStatusBar(int, int, int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x00107a10, 0x62)
void CSBI_RectOnly::UpdateRezMachineWakeStatusBar() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x00107ae0, 0x1aa)
void CSBI_RectOnly::LoadMultiplayerBattlezConfig(int) {}
