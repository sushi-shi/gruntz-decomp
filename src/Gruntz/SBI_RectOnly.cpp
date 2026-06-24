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
// of the CStatusBarItem family. Field names are placeholders (m_<hexoffset>);
// only the offsets + code bytes are load-bearing. The class carries a 24-byte
// (0x18) slot-struct array at +0x208, a pointer array at +0x204, a tab-index at
// +0x10c, an int gauge pair at +0x298/0x29c, and toggle flags at +0x558/0x55c.
//
// MEMBERSHIP NOTE: the this-pointer tracer assigned ~48 RVAs to CSBI_RectOnly.
// They all operate on this same large layout (offsets up to ~0x630) and the
// dtor at 0x102000 walks the vtable chain that ends at CSBI_RectOnly's vtable
// 0x5eab8c - so they are reconstructed here as one class. Sibling-method calls
// go through the ILT, so their call rel32 is reloc-masked regardless of name.
// ---------------------------------------------------------------------------

// A 24-byte (0x18) slot record: the +0x208 array element.
struct CSbiSlot {
    int m_0; // +0x00 (rel +0x208)
    int m_4; // +0x04 (rel +0x20c)
    char m_pad8[0x18 - 0x8];
};

// A 24-byte highlight-row record (the +0x378 array element): m_0 = state,
// m_4 = handle value passed to the notify pointer.
struct CSbiHlRow {
    int m_0; // +0x00 state
    int m_4; // +0x04 handle
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

// A hit-test rect widget held in m_150[]: m_4 = enabled flag, m_14/m_1c = x
// span, m_18/m_20 = y span.
struct CSbiRect {
    char m_pad0[0x4];
    int m_4; // +0x04 enabled
    char m_pad8[0x14 - 0x8];
    int m_14; // +0x14 x lo
    int m_18; // +0x18 y lo
    int m_1c; // +0x1c x hi
    int m_20; // +0x20 y hi
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

// base vftable (CStatusBarItem) anchored out-of-line in this TU.
class CSBI_RectOnly : public CStatusBarItem {
public:
    CSBI_RectOnly();
    virtual int SbiVfunc0() OVERRIDE;

    // ----- reconstructed CSBI_RectOnly methods (RVA-ascending) -----
    void Sbi_0e7400_ResetCounters();
    void Sbi_105520_ClearGauge();
    void Sbi_105560_SetSlotState(int idx);
    int Sbi_105710_AnyTabActive();
    void Sbi_105750_AdvanceGauge(int delta);
    void Sbi_1057d0_SetGauge(int value);
    void Sbi_1058d0_RefreshAll();
    void Sbi_105920_ResetGauge();
    void Sbi_107aa0_ToggleStat(int idx);
    void Sbi_1066f0_SetRectA(int y0, int x0, int z);
    void Sbi_106740_SetRectB(int y0, int x0, int z);
    void Sbi_106790_CommitSlot(int active);
    int Sbi_109a90_FindReadySlot();
    void Sbi_10bb90_SetMode(int mode);

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
    int Sbi_105710_probe(int idx);
    void Sbi_106900_RebuildA();
    void Sbi_106610_ResetGroupA();
    void Sbi_104f90b(int);
    void LoadStatzTabToggleSprite(int, int);
    void UpdateGruntOvenStatusBar();
    void Sbi_105480_TickGauge();
    void UpdateChipGrinderStatusBar();
    void UpdateDestructButtonStatusBar();
    int Sbi_104dd0_Activate();
    int Stub_0ffde0_probe();
    void Sbi_100d70_SetTabState(int tab, int state);

    int Sbi_104d60_TryActivate();
    int Sbi_101420_ClearTabSprites(int idx);
    int Sbi_105280_HitTest(int x, int y);
    int Sbi_1090a0_Serialize(CSbiStream* s);
    void Sbi_106a00_NotifyAllSlots();

    // ----- layout (placeholders; offsets are the load-bearing fact) -----
    char m_pad2c[0x30 - 0x2c];
    int m_30; // +0x30
    int m_34; // +0x34
    char m_pad38[0x10c - 0x38];
    int m_10c; // +0x10c  active tab index
    char m_pad110[0x114 - 0x110];
    int m_114[1]; // +0x114  per-stat toggle flag array
    char m_pad118[0x150 - 0x118];
    CSbiRect* m_150[15]; // +0x150  hit-test rect widgets
    char m_pad18c[0x1c8 - 0x18c];
    CSbiSprite* m_1c8;     // +0x1c8  per-tab sprite widgets (cleared by
    CSbiSprite* m_1cc;     // +0x1cc  Sbi_101420 in declaration order)
    CSbiSprite* m_1d0;     // +0x1d0
    CSbiSprite* m_1d4;     // +0x1d4
    CSbiSprite* m_1d8;     // +0x1d8
    CSbiSprite* m_1dc;     // +0x1dc
    CSbiSprite* m_1e0;     // +0x1e0
    CSbiSprite* m_1e4;     // +0x1e4
    CSbiSprite* m_1e8;     // +0x1e8
    CSbiSprite* m_1ec;     // +0x1ec
    CSbiSprite* m_1f0;     // +0x1f0
    CSbiSprite* m_1f4;     // +0x1f4
    CSbiSprite* m_1f8;     // +0x1f8
    CSbiSprite* m_1fc;     // +0x1fc
    CSbiSprite* m_200;     // +0x200
    CSbiSlotPtr* m_204[1]; // +0x204  slot pointer array (4-byte stride)
    char m_pad208[0x220 - 0x208];
    CSbiSlot m_220[1]; // +0x220  24-byte slot records
    char m_pad238[0x298 - (0x220 + 0x18)];
    int m_298; // +0x298  gauge current
    int m_29c; // +0x29c  gauge target
    char m_pad2a0[0x2c0 - 0x2a0];
    CSbiSlot m_2c0[3];     // +0x2c0  group-A 24-byte slot records
    CSbiSlotPtr* m_308[3]; // +0x308  group-A notify pointers
    char m_pad314[0x318 - 0x314];
    int m_318; // +0x318  HUD-rect group B (x0)
    int m_31c; // +0x31c  (y0)
    int m_320; // +0x320  (latched dword from g_dat645588)
    int m_324; // +0x324
    int m_328; // +0x328
    int m_32c; // +0x32c
    int m_330; // +0x330  HUD-rect group A (x0)
    int m_334; // +0x334  (y0)
    int m_338; // +0x338
    int m_33c; // +0x33c
    int m_340; // +0x340
    int m_344; // +0x344
    char m_pad348[0x354 - 0x348];
    int m_354; // +0x354  hit-test disable flag
    char m_pad358[0x35c - 0x358];
    int m_35c;          // +0x35c  active-slot index (-1 = none)
    int m_360;          // +0x360  pending highlight row index (-1 none)
    CSbiSlotPtr* m_364; // +0x364  notify targets (slot 0x28)
    CSbiSlotPtr* m_368; // +0x368
    CSbiSlotPtr* m_36c; // +0x36c
    CSbiSlotPtr* m_370; // +0x370
    char m_pad374[0x378 - 0x374];
    CSbiHlRow m_378[12];          // +0x378  3 groups x 4 highlight rows (24B each)
    CSbiSlotPtr* m_498[12];       // +0x498  3 groups x 4 notify pointers
    char m_pad4c8[0x4cc - 0x4c8]; // 0x498 + 12*4 = 0x4c8
    int m_4cc;                    // +0x4cc  arg for (*m_4e0)->Notify
    char m_pad4d0[0x4e0 - 0x4d0];
    CSbiSlotPtr* m_4e0; // +0x4e0
    char m_pad4e4[0x4ec - 0x4e4];
    int m_4ec; // +0x4ec  arg for (*m_500)->Notify
    char m_pad4f0[0x500 - 0x4f0];
    CSbiSlotPtr* m_500; // +0x500
    char m_pad504[0x534 - 0x504];
    void* m_534[1]; // +0x534  pointer table (elements streamed 8B)
    int m_538;      // +0x538  count for m_534
    char m_pad53c[0x558 - 0x53c];
    int m_558; // +0x558
    int m_55c; // +0x55c
    char m_pad560[0x570 - 0x560];
    CSbiSlotPtr* m_570; // +0x570  notify target
    int m_574;          // +0x574
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
void CSBI_RectOnly::Sbi_0e7400_ResetCounters() {
    m_34 = 0;
    m_30 = 0;
}

// Reset slots 0..4, then m_35c = -1.
RVA(0x00105520, 0x21)
void CSBI_RectOnly::Sbi_105520_ClearGauge() {
    for (int i = 0; i < 5; i++) {
        Sbi_105560_SetSlotState(i);
    }
    m_35c = -1;
}

// Slot[idx].f0 = 0; slot[idx].f4 = 1; notify the slot's pointer (vfunc 0x30).
RVA(0x00105560, 0x33)
void CSBI_RectOnly::Sbi_105560_SetSlotState(int idx) {
    m_220[idx].m_0 = 0;
    m_220[idx].m_4 = 1;
    if (m_204[idx]) {
        m_204[idx]->Notify(1);
    }
}

// Probe slots 0..4; return 1 on first hit, else 0.
RVA(0x00105710, 0x23)
int CSBI_RectOnly::Sbi_105710_AnyTabActive() {
    for (int i = 0; i < 5; i++) {
        if (Sbi_105710_probe(i)) {
            return 1;
        }
    }
    return 0;
}

// m_29c = min(m_298 + delta, 100).
RVA(0x00105750, 0x1f)
void CSBI_RectOnly::Sbi_105750_AdvanceGauge(int delta) {
    int v = m_298 + delta;
    if (v >= 100) {
        v = 100;
    }
    m_29c = v;
}

// m_29c = m_298 = value.
RVA(0x001057d0, 0x13)
void CSBI_RectOnly::Sbi_1057d0_SetGauge(int value) {
    m_29c = value;
    m_298 = value;
}

// Run the seven per-stat refresh updaters in sequence.
RVA(0x001058d0, 0x34)
void CSBI_RectOnly::Sbi_1058d0_RefreshAll() {
    UpdateGruntOvenStatusBar();
    Sbi_105480_TickGauge();
    UpdateRezConveyorStatusBar();
    LoadRezMachineConfig();
    LoadChipMachineConfig();
    UpdateChipGrinderStatusBar();
    UpdateDestructButtonStatusBar();
}

// Clear the gauge, zero m_298/m_29c, run two updaters, set toggle flags.
RVA(0x00105920, 0x47)
void CSBI_RectOnly::Sbi_105920_ResetGauge() {
    Sbi_105520_ClearGauge();
    m_29c = 0;
    m_298 = 0;
    Sbi_106610_ResetGroupA();
    UpdateRezMachineSnoozeStatusBar();
    Sbi_106900_RebuildA();
    m_55c = 1;
    m_558 = 0;
}

// Toggle stat[idx]: if already set, clear it; else set it on.
RVA(0x00107aa0, 0x23)
void CSBI_RectOnly::Sbi_107aa0_ToggleStat(int idx) {
    if (m_114[idx]) {
        Sbi_104f90b(idx);
    } else {
        LoadStatzTabToggleSprite(idx, 1);
    }
}

// Find the index of the first enabled hit-test rect containing (x,y); -1 none.
// @early-stop
// bool-materialization wall: retail keeps the redundant inner `p->m_4` test and
// materializes the point-in-rect predicate via `mov ecx,1 / xor ecx,ecx / test`;
// MSVC5 folds our `&&` chain into direct control flow (~80%). Logic byte-correct.
RVA(0x00105280, 0x61)
int CSBI_RectOnly::Sbi_105280_HitTest(int x, int y) {
    if (m_354 == 0) {
        for (int i = 0; i < 15; i++) {
            CSbiRect* p = m_150[i];
            if (p && p->m_4) {
                int hit = p->m_4 && x < p->m_1c && x >= p->m_14 && y < p->m_20 && y >= p->m_18;
                if (hit) {
                    return i;
                }
            }
        }
    }
    return -1;
}

// Reset the three group-A slots (arm f0=0/f4=1) and notify each pointer.
RVA(0x00106610, 0x3b)
void CSBI_RectOnly::Sbi_106610_ResetGroupA() {
    for (int i = 0; i < 3; i++) {
        m_2c0[i].m_0 = 0;
        m_2c0[i].m_4 = 1;
        if (m_308[i]) {
            m_308[i]->Notify(-1);
        }
    }
}

// Latch HUD-rect group A from three args + a global dword.
// @early-stop
// scheduling wall: logic byte-correct, but MSVC defers the m_330 store and
// reorders the m_33c/m_338 zero+global pair vs retail; ~72%, not steerable from
// C (see docs/patterns/u64-store-clock-hi-zero.md, statement-schedule-faithful).
RVA(0x001066f0, 0x3b)
void CSBI_RectOnly::Sbi_1066f0_SetRectA(int y0, int x0, int z) {
    m_334 = y0;
    m_330 = x0;
    m_340 = z;
    m_344 = 0;
    m_338 = g_dat645588;
    m_33c = 0;
}

// Latch HUD-rect group B from three args + a global dword.
// @early-stop
// scheduling wall: same store-reorder as Sbi_1066f0_SetRectA; ~72%.
RVA(0x00106740, 0x3b)
void CSBI_RectOnly::Sbi_106740_SetRectB(int y0, int x0, int z) {
    m_31c = y0;
    m_318 = x0;
    m_328 = z;
    m_32c = 0;
    m_320 = g_dat645588;
    m_324 = 0;
}

// Commit the active slot: either re-arm it, or push a cooked value (0x1a) and
// notify its pointer; then clear the active-slot index.
RVA(0x00106790, 0x62)
void CSBI_RectOnly::Sbi_106790_CommitSlot(int active) {
    if (active) {
        Sbi_105560_SetSlotState(m_35c);
        m_35c = -1;
    } else {
        m_220[m_35c].m_4 = 0x1a;
        if (m_204[m_35c]) {
            m_204[m_35c]->Notify(m_220[m_35c].m_4);
        }
        m_35c = -1;
    }
}

// Fire every live slot's notifier: the four singletons, the 3x4 group grid
// (each pointer notified with its row's handle), then two trailing singletons.
RVA(0x00106a00, 0xbf)
void CSBI_RectOnly::Sbi_106a00_NotifyAllSlots() {
    if (m_364) {
        m_364->v28();
    }
    if (m_36c) {
        m_36c->v28();
    }
    if (m_370) {
        m_370->v28();
    }
    if (m_4e0 && m_4cc) {
        m_4e0->Notify(m_4cc);
    }

    CSbiSlotPtr** p = &m_498[4]; // group B base; ±4 elements reach groups A / C
    int* h = &m_378[4].m_4;      // anchor on the handle field (+0x3dc)
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

    if (m_368) {
        m_368->v28();
    }
    if (m_500) {
        m_500->Notify(m_4ec);
    }
}

// Stream the full rect-only item state through the archive (stream slot 0x30).
// Returns 0 if the stream or the active game-manager is null; bumps the global
// serialize counter; ends with a variable-length loop over the m_534[] pointer
// table (count m_538), each element streamed as 8 bytes. Field buffers are
// addressed by offset (the codegen is naming-independent here).
// @early-stop
// ~95.6%: the entire ~70-field transfer body is byte-exact; the residual is a
// regalloc/frame-size difference in the trailing 3x4 nested loop (retail pins
// the inner counter in ebp + reserves 1 stack dword via `push ecx`; the
// recompile spills the inner counter and reserves 3 via `sub esp,0xc`). Not
// steerable from C (docs/patterns regalloc/scheduling walls); deferred.
RVA(0x001090a0, 0x38f)
int CSBI_RectOnly::Sbi_1090a0_Serialize(CSbiStream* s) {
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

    int count = m_538;
    s->Transfer(&count, 4);
    for (unsigned int n = 0; n < (unsigned int)count; n++) {
        s->Transfer(m_534[n], 8);
    }
    return 1;
}

// Find the first slot whose state == 2; re-arm it and report found.
RVA(0x00109a90, 0x25)
int CSBI_RectOnly::Sbi_109a90_FindReadySlot() {
    for (int i = 0; i < 5; i++) {
        if (m_220[i].m_0 == 2) {
            Sbi_105560_SetSlotState(i);
            return 1;
        }
    }
    return 0;
}

// Release the per-tab sprite widgets for the given tab group (idx, with -1 = all).
RVA(0x00101420, 0x110)
int CSBI_RectOnly::Sbi_101420_ClearTabSprites(int idx) {
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

// Activate the rect-only item; gate on the mode field at offset 0 and a probe.
RVA(0x00104d60, 0x48)
int CSBI_RectOnly::Sbi_104d60_TryActivate() {
    if (*(int*)this == 2) {
        return Sbi_104dd0_Activate();
    }
    if (!Stub_0ffde0_probe()) {
        g_gameReg->ReportError(0x80e4, 0x44b);
        return 0;
    }
    Sbi_100d70_SetTabState(m_10c, 3);
    return 1;
}

// Enter mode: latch m_574, conditionally reset the toggle pair, notify m_570.
RVA(0x0010bb90, 0x3f)
void CSBI_RectOnly::Sbi_10bb90_SetMode(int mode) {
    m_574 = 1;
    if (mode && m_55c != 7) {
        m_558 = 0;
        m_55c = 1;
        if (m_570) {
            m_570->Notify(1);
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
