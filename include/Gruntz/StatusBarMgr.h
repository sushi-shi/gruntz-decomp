#ifndef GRUNTZ_CSTATUSBARMGR_H
#define GRUNTZ_CSTATUSBARMGR_H

class CSBI_ImageSet; // notify-field element (slot-12 Notify receiver)
class CWarpStoneFly;
class CSBI_MenuItem;
class CSBI_GruntMachine; // <Gruntz/SBI_GruntMachine.h> - m_machineDisplay's real type
class DSoundCloneInst;   // <Dsndmgr/DirectSoundMgr.h> - pooled cue play-factory
class DirectSoundMgr; // <Dsndmgr/DirectSoundMgr.h> - the DirectSound clone (destruct-button voice)

#include <Ints.h>
#include <rva.h>
#include <Mfc.h>                  // CPtrList (the eight embedded tab lists) / CPtrArray
#include <Bute/ButeMgr.h>         // canonical CButeMgr (one shape)
#include <Gruntz/GameRegistry.h>  // canonical CGameRegistry (the one *0x24556c singleton)
#include <Gruntz/SbRect.h>        // the geometry rect passed by value into the configure virtuals
#include <Gruntz/SbiConfig.h>     // canonical config-host family (one shape)
#include <Gruntz/SerialArchive.h> // the shared CSerialArchive stream (Read @+0x2c / Write @+0x30)
#include <Gruntz/StatusBarItem.h>

SIZE(CSbiSlot, 0x18);
struct CSbiSlot {
    // Inline default ctor (retail inlines it as the 5-iteration loop at +0x228).
    CSbiSlot() {
        m_8 = 0;
        m_10 = 0;
        m_c = 0;
        m_14 = 0;
    }
    i32 m_state; // +0x00 (rel +0x208)
    i32 m_value; // +0x04 (rel +0x20c)  the value handed to the slot's notify sink
    i32 m_8;     // +0x08 (gauge-span lo / the 64-bit timer with m_c)
    i32 m_c;     // +0x0c
    i32 m_10;    // +0x10 (the ctor zeroes it)
    i32 m_14;    // +0x14 (the ctor zeroes it)
};

SIZE(CSbiHlRow, 0x18);
struct CSbiHlRow {
    CSbiHlRow(); // 0x0c86d0 (the address-taken COMDAT; def in ModeObjInit.cpp)
    i32 m_state; // +0x00 state
    // +0x04: ONE field, two proven names - the hl-grid code calls it the handle it
    // hands to the notify pointer, the group-slot code calls it the value it hands to
    // the same notify pointer. Same role, same offset. The anonymous union keeps BOTH
    // names live (name-preserving fold; layout unchanged) rather than silently dropping
    // one side's knowledge.
    union {
        i32 m_handle;
        i32 m_value;
    };
    i32 m_8;  // +0x08
    i32 m_c;  // +0x0c
    i32 m_10; // +0x10
    i32 m_14; // +0x14
};

class CSBI_SideTab; // <Gruntz/SBI_SideTab.h> - the m_hitRects element
SIZE_UNKNOWN(CSbiRect);

// (CSbiStatObj DISSOLVED 2026-07-20: m_statObj holds CStatusBarMgr sub-managers - the
// SetState clear path already casts m_statObj[idx] to CStatusBarMgr for ResetGroupA, and
// its Toggle notifier (FUN_004ea170, unreconstructed) is that same class's method. Both
// call sites now agree on CStatusBarMgr; Toggle is declared on it below.)

class CSbiNotifyTarget {
public:
    virtual void Notify(i32 on); // slot 0 (__thiscall void(int))
};
SIZE_UNKNOWN(CSbiNotifyTarget);
struct CSbiNotifyNode {
    CSbiNotifyNode* m_next; // +0
    char m_pad4[0x8 - 0x4];
    CSbiNotifyTarget* m_payload; // +8
};
SIZE_UNKNOWN(CSbiNotifyNode);

class CSbiNotifyPayload {
public:
    virtual void Destroy();     // slot 0  scalar-deleting dtor
    virtual void Serialize();   // slot 1
    virtual void Setup();       // slot 2
    virtual void ClearFrame();  // slot 3
    virtual void Poll(i32 arg); // +0x10 (slot 4)
    virtual void Tick();        // +0x14 (slot 5)
    virtual void HitHandlerA(); // slot 6
    virtual void HitHandlerB(); // slot 7
    virtual void HitHandlerC(); // slot 8
    virtual void HitHandlerD(); // slot 9
    virtual void Refresh();     // +0x28 (slot 10)
};
SIZE_UNKNOWN(CSbiNotifyPayload);

struct CSbiSpriteCfg {
    char m_pad0[0x10];
    DSoundCloneInst* m_playFactory; // +0x10  pooled cue play-factory (GetItem -> DirectSoundMgr)
};
SIZE_UNKNOWN(CSbiSpriteCfg);

SIZE(CWarpStoneFly, 0x40);

class CSbiGaugeNotify {
public:
    virtual void Destroy();     // slot 0  scalar-deleting dtor
    virtual void Serialize();   // slot 1
    virtual void Setup();       // slot 2
    virtual void ClearFrame();  // slot 3
    virtual void Poll();        // slot 4
    virtual void Tick();        // slot 5
    virtual void HitHandlerA(); // slot 6
    virtual void HitHandlerB(); // slot 7
    virtual void HitHandlerC(); // slot 8
    virtual void HitHandlerD(); // slot 9
    virtual void Refresh();     // +0x28 (slot 10) refresh
    char m_pad4[0x44 - 0x4];
    i32 m_gaugeReading; // +0x44  latched gauge reading
};
SIZE_UNKNOWN(CSbiGaugeNotify);

extern CButeMgr g_buteMgr;

struct SbiPhaseSlot {
    i32 m_state;    // +0x00
    i32 m_counter;  // +0x04
    i64 m_last;     // +0x08  last draw-clock (64-bit)
    i64 m_interval; // +0x10  wait interval (64-bit)
};
SIZE_UNKNOWN(SbiPhaseSlot);

enum SbiSlotState {
    kSlotArmed = 0,
    kSlotReady = 2,
};

const i32 kSlotCommitLevel = 0x1a;

const i32 kSubtypeTag = 2;

const i32 kActivateErrId = 0x80e4;
const i32 kActivateErrTag = 0x44b;

const i32 kSetTabErrTag = 0x44a;

class CStatusBarMgr {
public:
    // The REAL inline default ctor. Retail has no out-of-line ??0: it
    // INLINES this whole body at both `new`-sites (CPlay::LoadGameAssetNamespaces 0xc7fea and
    // the CMulti slot-1 driver 0xb5931, both `push 0x630; call ??2`). Body is
    // below the class (it needs the complete type). This replaces the ~100 raw
    // `*reinterpret_cast<i32*>(p + 0xNN) = ...` stores that BOTH call sites used to hand-roll through a
    // local `Worker630` view - the last view in ModeObjInit.cpp - and the two
    // out-of-band stamps Multi.cpp had to bolt on after `new` (m_barFrameGate/m_544).
    CStatusBarMgr();
    i32 BuildSideTabs(); // 0x105070 (ex 'CStatzTabBuilder::Build' - the builder WAS this mgr)

    // The REAL inline destructor: Teardown() then the compiler-generated member
    // teardown, in reverse declaration order - ~CPtrArray on m_ptrPool, then the
    // eh-vector-dtor over the eight m_tabLists CPtrLists. PROVEN at CPlay::LoadGameAssetNamespaces's
    // fail path (0xc82b6), where `delete` inlines exactly that sequence under /GX
    // states 3/2.
    ~CStatusBarMgr() {
        Teardown();
    }

    // ----- the per-tab widget builders ---
    i32 LoadTabSprites(); // 0x102250
    void BuildGameMenu(); // 0x101580 (the GAMETAB menu builder; called at the Game-tab tail)

    void UpdateDestructButton(i32 arg); // 0x10bc30 (arms the destruct-button warning)
    i32 EnsureSub(i32 a, i32 b, i32 c); // 0x109ad0 (lazily create the +0x54c sub-object)
    void DtorMembers();                 // 0xc8980  member teardown (/GX member-array dtor)
    void ResetCounters();
    void ResetSlots();
    void ArmSlot(i32 idx);
    i32 AnySlotActive();
    void AdvanceGauge(i32 delta);
    void SetGauge(i32 value);
    void RefreshAll();
    void Reset();
    i32 Place(i32 a, i32 b, i32 c); // reloc-masked
    void Run();                     // reloc-masked
    void ToggleStat(i32 idx);
    void SetHudRectA(i32 y0, i32 x0, i32 z);
    void SetHudRectB(i32 y0, i32 x0, i32 z);
    void CommitSlot(i32 active);
    void ClearHlCell(i32 row, i32 group);
    i32 SetHlCell(i32 row, i32 handle, i32 group);
    i32 SetHlCellByTier(i32 handle, i32 group); // 0x106af0  pick the hl row from the handle tier
    i32 FindReadySlot();
    void SetMode(i32 mode);

    // Engine-label backlog stubs.
    i32 BuildStatusBarTabs();
    // 0x10a340 (body in SBI_TabzDialogEh.cpp - its own retail obj; that TU needs the
    // out-of-line CStatusBarItem base ctor, this one inlines it). PROVEN a CStatusBarMgr
    // method, not the ex-"CTabzBuilder" view: BuildStatusBarTabs (0xffde0) takes its own
    // `this` into edi (`mov edi,ecx` @0xffdfc) and calls it unchanged (`mov ecx,edi;
    // call 0x41a1` @0x100353) - same object, so the receiver is a CStatusBarMgr. The
    // view's 16 `(CStatusBarMgr*)this` casts at its SetupImage arg1 were that identity
    // spelling itself out.
    i32 BuildTabzDialog();
    i32 Probe2e69(); // 0x2e69 (post-build validity probe)
    i32 Probe41a1(); // 0x41a1 (post-build validity probe)
    i32 winapi_107d00_SetRect();
    i32 LoadBattlezItemConfig(CDDrawSurfaceMgr* world); // stores the world holder into m_c
    i32 LoadMainStatusBarSprite();
    i32 UpdateStatusBarTabHighlight(i32, i32, i32);
    i32 LoadDestructButtonSprite(i32);
    void BuildGameTabResumeButton(i32);
    void BuildGameTabPauseButton();

    // ----- siblings the tab-highlight dispatcher (0xfe910) drives (reloc-masked ILT) -----
    struct CSbiHiWidget* HiResolve(i32 a, i32 b); // 0x2dc4 (resolve the hit widget)
    void HiRefreshResource();                     // 0x3d91 (call 0x3d91)
    void HiSelectStat(i32 idx);                   // 0x264e (call 0x264e, 1 arg)
    void HiTabA(i32 idx);                         // 0x4179 (1 arg)
    void HiTabB(i32 idx, i32 flag);               // 0x20b8 (2 args)
    void HiGrunt0(i32 idx);                       // 0x42a5 (1 arg)
    void HiGrunt1(i32 idx);                       // 0x4151 (1 arg)
    void HiGrunt2(i32 idx);                       // 0x37ce (1 arg)
    i32 LoadGooCookingSprite(i32);
    void UpdateRezConveyorStatusBar();
    void LoadRezMachineConfig();
    void UpdateRezMachineSnoozeStatusBar();
    void LoadChipMachineConfig();
    i32 UpdateFallingItemStatusBar(i32, i32, i32);
    i32 UpdateRezMachineWakeStatusBar();
    void LoadMultiplayerBattlezConfig(i32);

    // ----- sibling methods called by the reconstructed bodies (declared so the
    // ILT call targets resolve; bodies live elsewhere / are stubbed) -----
    void ResetGroupA();
    // 0xea170 (FUN_004ea170, unreconstructed): the per-stat (stateId, on) toggle notifier
    // fired on the m_statObj sub-managers (ex the CSbiStatObj view). Reloc-masked.
    void Toggle(i32 stateId, i32 on);
    // 0x10bbe0: the rez-machine active-value getter (body in SBI_RectOnly.cpp -
    // m_extraNotifyArg0 / m_ptrPool active cell).
    i32 GetActiveValue();
    i32 LoadStatzTabToggleSprite(i32 value, i32 idx); // 0x104e60 (body in SBI_RectOnly.cpp)
    void UpdateGruntOvenStatusBar();
    void TickGauge();
    i32 GaugeComplete();   // call 0x3e2c - gauge-at-100 completion test
    void GaugeFinish(i32); // call 0x39ef - completion hook
    void UpdateChipGrinderStatusBar();
    void ChipGrinderFinishStep(); // 0x106a00 (grinder finish-step; reloc-masked)
    void UpdateDestructButtonStatusBar();
    i32 Activate();
    i32 SetTabState(i32 tab, i32 state);

    void Teardown();
    i32 TryActivate();
    i32 Deactivate();
    i32 HlClickGroup0(i32 row);
    i32 HlClickGroup1(i32 row);
    i32 HlClickGroup2(i32 row);
    void* ResolveHandle(i32 handle);  // call 0x17a8 - validity probe
    void SetCursorRect(i32 x, i32 y); // call 0x3878 (__thiscall, 2 args)
    i32 SetTab(i32 tab, i32 flag);
    i32 ClearTabSprites(i32 idx);
    i32 HitTest(i32 x, i32 y);
    i32 Serialize(CSerialArchive* s);
    i32 Deserialize(CSerialArchive* s);
    void NotifyAllSlots();

    i32 ConfigureRect(
        i32 sub,
        CDDrawSurfaceMgr* host,
        i32 cmd,
        i32 obj,
        i32 r0,
        i32 r1,
        i32 r2,
        i32 r3,
        i32 key,
        i32 frame,
        i32 extra
    );
    i32 ClickHilite(i32 x, i32 y, i32 z);
    // 0xff9d0 (unbound; between ClickHilite 0xff850 and ClickToggle 0xff9f0): the
    // HUD click-at-point dispatch CPlay::DispatchHudClick runs on m_guts (thunk
    // 0x2559). Reloc-masked until reconstructed.
    i32 ClickAt_ff9d0(i32 a, i32 x, i32 y);
    i32 ClickToggle(i32 x, i32 y, i32 z);
    CStatusBarItem* HitTestRects(i32 x, i32 y);
    void ResetWidgets(i32 keepLists);
    void ClearTabGroup();
    i32 ClearStat(i32 idx);
    i32 ActivateCursor(i32 idx, i32 on);
    void EnterHlRow(i32 row, i32 group);
    void InitTabRects();
    i32 SetFallRect(i32 a, i32 b, i32 c);
    void ExitMode();
    i32 ActivateSlot(i32 idx);
    i32 PlaceCursorTarget(i32 row, i32 commit);

    i32 SetState(i32 state);
    i32 RefreshState();
    i32 SetSpritePos(i32 x, i32 y);
    i32 HitTestLayer(i32 x, i32 y);
    i32 InsertPtr(i32 a, i32 b);
    void ReportTab(i32 tab);
    // siblings dispatched (reloc-masked ILT thunks / bodies elsewhere)
    i32 RefreshA(); // 0xfe460  armed-refresh rect-setup variant
    i32 HideRect(); // 0xfe600  hide/off-screen rect-setup variant

    // ----- third batch -----
    void AdvanceTab(i32 reverse); // 0x10b4f0 periodic highlight-cursor tick

    // ----- fourth batch: the rect-only HUD placement (0xfe520) + its siblings ---
    i32 winapi_0fe520_SetRect();
    void RefreshFallRect();                       // call 0x1cbc (__thiscall, no args)
    void ConveyorReturn();                        // call 0x26a3 (__thiscall, no args)
    i32 FallItemTick();                           // call 0x2130 (__thiscall, no args)
    void ChipNotify27f7();                        // call 0x27f7 (__thiscall, no args)
    void ChipFinish(i32 col, i32 which, i32 row); // call 0x3968 (__thiscall, 3 args)

    // ----- fifth batch: item-config-loader siblings (reloc-masked ILT thunks) -----
    void RefreshHost();         // call 0x2577 (__thiscall, no args)
    void PrepMultiReset();      // call 0x367a (__thiscall, no args)
    void NotifyTabExit();       // call 0x2329 (__thiscall, no args)
    void ArmTab(i32 a, i32 b);  // call 0x427d (__thiscall, 2 args)
    void ResetTabWidgets2b44(); // call 0x2b44 (__thiscall, no args)
    void FinishReset1f6e();     // call 0x1f6e (__thiscall, no args)
    void FinishReset16ea();     // call 0x16ea (__thiscall, no args)

    // ----- layout (placeholders; offsets are the load-bearing fact) -----
    // +0x00 is a DATA member, not a vptr: this class has no vtable, and the code both
    // reads it (`*(i32*)this == 2` subtype gate) and WRITES it (SetState), then persists
    // it to the registry under "StatusBar Position".
    i32 m_position; // +0x00  status-bar side/position selector (registry "StatusBar Position")
    i32 m_4;        // +0x04
    // +0x08: the status-bar render sprite - the "StatusBarSprite" CreateSprite result.
    // The ex CSbiRenderObj/CSbiSeqHolder views of this slot were CGameObject facets:
    // +0x5c/+0x60 m_screenX/Y, +0x188 the archive-cue id, +0x198 m_layer (the CImage).
    class CWwdGameObjectA* m_barSprite; // the bound A-kind bar sprite
    // +0x0c: the config host every widget-setup call takes as arg2. Typed from the retail
    // callee: CSBI_Image::SetupImage (0xe6c80) DEREFERENCES this arg at +0x10
    // (`mov eax,[esp+8]` ... `mov ecx,[eax+0x10]`) to reach the lookup map - it is a
    // CDDrawSurfaceMgr*, not the `i32 code` the builders used to pass it as.
    CDDrawSurfaceMgr* m_c; // +0x0c
    i32 m_10;              // +0x10  tab base x
    // +0x14..0x23: a 4-int block. The tab builder reads m_rect14.m_0 as the tab base y.
    SbiRect m_rect14; // +0x14
    i32 m_24;         // +0x24
    i32 m_28;         // +0x28
    // +0x2c: EIGHT CPtrLists, built by the EH-vector-ctor in CPlay::LoadGameAssetNamespaces
    // (`lea edx,[esi+0x2c]; push 0x1c; push 8`; 0x1c == sizeof(CPtrList)). Elements
    // [1]..[5] are the per-tab widget lists the tab selector (m_activeTab, 1..5) picks:
    // [1] Statz  [2] Gruntz  [3] Resource  [4] Multiplayer  [5] Game.
    // [0] is the always-present widget list BuildStatusBarTabs appends to; [6]/[7] are
    // the trailing notify/hit-test lists.
    CPtrList m_tabLists[8];       // +0x2c .. +0x10c
    i32 m_activeTab;              // +0x10c  active tab index (1..5)
    i32 m_itemKind;               // +0x110  item-kind tag (LoadBattlezItemConfig sets 5)
    i32 m_statFlags[15];          // +0x114  per-stat toggle flag array
    CSBI_SideTab* m_hitRects[15]; // +0x150  the statz side-tab widgets (hit-test targets)
    CStatusBarMgr* m_statObj[15]; // +0x18c  per-stat sub-manager array (notified on clear; ex CSbiStatObj view)
    CSBI_MenuItem* m_tabSprite0;  // +0x1c8  per-tab sprite widgets (cleared by
    CSBI_MenuItem* m_tabSprite1;  // +0x1cc  ClearTabSprites in declaration order)
    CSBI_MenuItem* m_tabSprite2;  // +0x1d0
    CSBI_MenuItem* m_tabSprite3;  // +0x1d4
    CSBI_MenuItem* m_tabSprite4;  // +0x1d8
    CSBI_MenuItem* m_tabSprite5;  // +0x1dc
    CSBI_MenuItem* m_tabSprite6;  // +0x1e0
    CSBI_MenuItem* m_tabSprite7;  // +0x1e4
    CSBI_MenuItem* m_tabSprite8;  // +0x1e8
    CSBI_MenuItem* m_tabSprite9;  // +0x1ec
    CSBI_MenuItem* m_tabSprite10; // +0x1f0
    CSBI_MenuItem* m_tabSprite11; // +0x1f4
    CSBI_MenuItem* m_tabSprite12; // +0x1f8
    CSBI_MenuItem* m_tabSprite13; // +0x1fc
    CSBI_MenuItem* m_tabSprite14; // +0x200
    // +0x204: the five slot notifiers (ArmSlot indexes `[ecx+eax*4+0x204]`, idx 0..4).
    CSBI_ImageSet* m_slotNotify[5];   // +0x204 .. +0x218
    CSbiGaugeNotify* m_gaugeNotify; // +0x218  gauge notifier (vfunc 0x28)
    CSbiGaugeNotify* m_gaugeSink;   // +0x21c  gauge value sink (m_44 = gauge; vfunc 0x28)
    // +0x220: the five 0x18-byte slot records. ArmSlot: `lea edx,[eax+eax*2];
    // lea edx,[ecx+edx*8]; mov [edx+0x220],0; mov [edx+0x224],1` => base 0x220, stride
    // 0x18, five elements ending exactly at m_gauge (0x220 + 5*0x18 == 0x298).
    CSbiSlot m_slots[5]; // +0x220 .. +0x298
    // +0x298 - SETTLED (it is ONE role, not the two the split-era comment hedged over).
    // TickGauge (0x105480) eases m_gauge one step per tick toward m_gaugeTarget, latches on
    // `m_gauge == 100`, and pushes the value into m_gaugeSink->m_44: a 0..100 percentage.
    // LoadTabSprites hands that SAME field to Configure() for the
    // "GAME_STATUSBAR_TABZ_GRUNTZTAB_WELLGOO" widget - i.e. it seeds the goo-well gauge
    // widget with the current fill level. The builder's "WELLGOO config-d source" reading
    // was just the construction-side view of the gauge. This IS the goo-well gauge.
    i32 m_gauge;       // +0x298  goo-well gauge, 0..100 (current)
    i32 m_gaugeTarget; // +0x29c  goo-well gauge target (TickGauge eases m_gauge to it)
    // +0x2a0..+0x2ac: NOT padding - the ctor zeroes these four dwords, and they are the
    // head of the same 8-dword multiplayer/battlez reset block as m_2b0..m_2bc below.
    i32 m_2a0; // +0x2a0
    i32 m_2a4; // +0x2a4
    i32 m_2a8; // +0x2a8
    i32 m_2ac; // +0x2ac
    i32 m_2b0; // +0x2b0  (multiplayer/battlez reset block)
    i32 m_2b4; // +0x2b4
    i32 m_2b8; // +0x2b8
    i32 m_2bc; // +0x2bc
    // +0x2c0: group-A 24-byte slot records. CSbiHlRow, NOT CSbiSlot - it shares the
    // out-of-line element ctor 0xc86d0 with m_hlGrid (the vector-ctor iterator takes
    // that one function pointer for BOTH arrays). Layout is unchanged.
    CSbiHlRow m_groupSlots[3];     // +0x2c0
    CSBI_ImageSet* m_groupNotify[3]; // +0x308  group-A notify pointers
    char m_pad314[0x318 - 0x314];
    i32 m_hudRectB_x;                    // +0x318  HUD-rect group B (x0)
    i32 m_hudRectB_y;                    // +0x31c  (y0)
    i32 m_hudRectB_clock;                // +0x320  (latched dword from g_dat645588)
    i32 m_hudRectB_clockHi;              // +0x324
    i32 m_hudRectB_z;                    // +0x328
    i32 m_hudRectB_zHi;                  // +0x32c
    i32 m_hudRectA_x;                    // +0x330  HUD-rect group A (x0)
    i32 m_hudRectA_y;                    // +0x334  (y0)
    i32 m_hudRectA_clock;                // +0x338
    i32 m_hudRectA_clockHi;              // +0x33c
    i32 m_hudRectA_z;                    // +0x340
    i32 m_hudRectA_zHi;                  // +0x344
    CSBI_GruntMachine* m_machineDisplay; // +0x348  the Resource-tab MACHINE widget (SetFrames)
    i32 m_34c;                           // +0x34c
    i32 m_350;                           // +0x350
    i32 m_hitTestDisabled;               // +0x354  hit-test disable flag
    i32 m_tabsBuilt;                     // +0x358  tab-widgets-built flag
    i32 m_activeSlot;                    // +0x35c  active-slot index (-1 = none)
    i32 m_pendingHlRow;                  // +0x360  pending highlight row index (-1 none)
    CStatusBarItem* m_notify0;              // +0x364  notify targets (slot 0x28)
    CStatusBarItem* m_notify1;              // +0x368
    CStatusBarItem* m_notify2;              // +0x36c
    CStatusBarItem* m_notify3;              // +0x370
    char m_pad374[0x378 - 0x374];
    CSbiHlRow m_hlGrid[12];      // +0x378  3 groups x 4 highlight rows (24B each)
    CSBI_ImageSet* m_hlNotify[12]; // +0x498  3 groups x 4 notify pointers
    i32 m_machinePhase;          // +0x4c8  (set to 1 by InitTabRects)
    i32 m_extraNotifyArg0;       // +0x4cc  arg for (*m_extraNotify0)->Notify
    i64 m_beltLast;              // +0x4d0  belt-drop timer last draw-clock (64-bit)
    i64 m_beltInterval;          // +0x4d8  belt-drop timer interval (64-bit)
    CSBI_ImageSet* m_extraNotify0; // +0x4e0
    char m_pad4e4[0x4e8 - 0x4e4];
    i32 m_fallActive;            // +0x4e8  falling-item active flag
    i32 m_extraNotifyArg1;       // +0x4ec  arg for (*m_extraNotify1)->Notify
    i64 m_fallLast;              // +0x4f0  falling-item timer last draw-clock = g_dat645588
    i64 m_fallDelay;             // +0x4f8  falling-item config delay
    CSBI_ImageSet* m_extraNotify1; // +0x500
    i32 m_fallRectL;             // +0x504  falling-item rect A (relative)
    i32 m_fallRectT;             // +0x508
    i32 m_fallRectR;             // +0x50c
    i32 m_fallRectB;             // +0x510
    i32 m_itemRectL;             // +0x514  streamed rect block (report origin)
    i32 m_itemRectT;             // +0x518
    i32 m_itemRectR;             // +0x51c
    i32 m_itemRectB;             // +0x520
    i32 m_itemBaseX;             // +0x524
    i32 m_rezActive;             // +0x528  rez-machine snooze/wake active flag
    i32 m_rezTick;               // +0x52c  rez-machine wake tick counter
    // +0x530  the pooled-ptr collection: a REAL MFC ::CPtrArray (0x14 -> +0x530..+0x543).
    // Its internals: m_pData @+0x534, m_nSize @+0x538, m_nMaxSize @+0x53c, m_nGrowBy @+0x540.
    ::CPtrArray m_ptrPool;
    i32 m_544;    // +0x544  stamped 1 at the session new-site (the CMulti slot-1 driver
                  //         0xb5460, with m_barFrameGate = 0x1e0); role unrecovered
    i32 m_hlBusy; // +0x548
    CWarpStoneFly* m_retabNotify; // +0x54c  a notifier object (freed on retab; Refresh()/Notify0())
    i32 m_toggleActive;           // +0x550  toggle-mode active flag
    i32 m_toggleHandle;           // +0x554  toggle-mode tab handle
    i32 m_destructWarnActive;     // +0x558
    i32 m_modeState;              // +0x55c
    i64 m_destructWarnLast;       // +0x560  destruct-warning last draw-clock
    i64 m_destructWarnDelay;      // +0x568  destruct-warning delay (config)
    CSBI_ImageSet* m_modeNotify;    // +0x570  notify target
    i32 m_modeArmed;              // +0x574
    i32 m_578;                    // +0x578  (cleared on multiplayer/battlez reset)
    i32 m_battlezPct[38];         // +0x57c  running-sum item-percent table (battlez cfg)
    i32 m_barFrameGate;           // +0x614  main-status-bar frame gate
    DirectSoundMgr*
        m_destructButton; // +0x618  destruct-button warning voice (ApplyAndPlay/StopAndRewind)
    i32 m_61c[4];         // +0x61c  trailing dword block (cleared on reset)
    i32 m_tabCycle;       // +0x62c  4-state highlight cursor (AdvanceTab cycles 0..3)
};
SIZE(CStatusBarMgr, 0x630);

struct CSbiCueRecord {
    char m_pad0[0x10];
    DSoundCloneInst* m_10; // +0x10  player (ConfigureItem this)
    i32 m_14;              // +0x14  last draw-clock
    i32 m_18;              // +0x18  interval
};
SIZE_UNKNOWN(CSbiCueRecord);

struct CSbiSeqMap {}; // MFC CMapPtrToPtr (Lookup @0x1b8760); cast at the call
SIZE_UNKNOWN(CSbiSeqMap);

class CSbiSeqObj {
public:
    virtual void Destroy();     // slot 0  scalar-deleting dtor
    virtual void Serialize();   // slot 1
    virtual void Setup();       // slot 2
    virtual void ClearFrame();  // slot 3
    virtual void Poll();        // slot 4
    virtual void Tick();        // slot 5
    virtual void HitHandlerA(); // slot 6
    virtual void HitHandlerB(); // slot 7
    virtual i32 TypeTag();      // +0x20 (slot 8)
};
SIZE_UNKNOWN(CSbiSeqObj);

struct CSbiResetHost {
    char m_pad0[0x8];
    i32 m_8; // +0x08  status flags (|= 0x10000)
    char m_padc[0x40 - 0xc];
    i32 m_40; // +0x40  control flags (|= 1)
};
SIZE_UNKNOWN(CSbiResetHost);

struct CSbiFreeNode {
    i32 m_0, m_4;
};
SIZE_UNKNOWN(CSbiFreeNode);

struct SbiTabFrameSub {
    char m_pad0[0x10];
    i32 m_10;
};
SIZE_UNKNOWN(SbiTabFrameSub);
struct SbiTabFrame {
    char m_pad0[0x14];
    SbiTabFrameSub* m_14;
    char m_pad18[0x64 - 0x18];
    i32 m_64;
    i32 m_68;
};
SIZE_UNKNOWN(SbiTabFrame);

struct CTabList {};
SIZE_UNKNOWN(CTabList);

struct CSbiFrameEntry {
    char m_pad0[0x18];
    i32 m_18; // +0x18
    i32 m_1c; // +0x1c
};
SIZE_UNKNOWN(CSbiFrameEntry);
struct CSbiMainBarCfg {
    char m_pad0[0x14];
    CSbiFrameEntry** m_14; // +0x14  frame-entry table
    char m_pad18[0x64 - 0x18];
    i32 m_64; // +0x64  frame index
};
SIZE_UNKNOWN(CSbiMainBarCfg);
class CDDrawSurfacePair; // the real main-bar draw receiver (ex the CSbiMainL2 facet)
void __stdcall MainBarDrawFrame(CDDrawSurfacePair* obj, i32 x, i32 y, i32 flag); // 0x153790 (NOTE:

class CSbiHiWidget {
public:
    virtual void Destroy();                   // slot 0  scalar-deleting dtor
    virtual void Serialize();                 // slot 1
    virtual void Setup();                     // slot 2
    virtual void ClearFrame();                // slot 3
    virtual void Poll();                      // slot 4
    virtual void Tick();                      // slot 5
    virtual void Update(i32 a, i32 b, i32 c); // +0x18 (slot 6)
    char m_pad4[0xc - 0x4];
    i32 m_c;  // +0xc  command id
    i32 m_10; // +0x10  widget kind (outer switch key, 0..6)
};
SIZE_UNKNOWN(CSbiHiWidget);

inline CStatusBarMgr::CStatusBarMgr() {
    // Scalar body, in retail's source order. NOTE what is DELIBERATELY absent: the
    // ctor does NOT initialise m_position/m_4/m_10/m_24/m_28/m_itemKind,
    // m_activeSlot/m_pendingHlRow, m_machinePhase, m_extraNotifyArg0/1, the
    // m_fall*/m_item* rects, m_rezActive/m_rezTick, m_578/m_battlezPct,
    // m_destructWarnActive/m_modeState or m_34c/m_350. That is the BINARY, not an
    // omission - the 0x630 block comes from the Rez heap and the devs seeded only the
    // fields whose zero/one value is load-bearing. Do NOT "complete" this list; every
    // extra store is a byte of drift.
    m_2a0 = 0; // +0x2a0..+0x2bc  the multiplayer/battlez reset block (8 dwords)
    m_2a4 = 0;
    m_2a8 = 0;
    m_2ac = 0;
    m_2b0 = 0;
    m_2b4 = 0;
    m_2b8 = 0;
    m_2bc = 0;
    m_hudRectB_clock = 0; // +0x320  HUD-rect group B 64-bit clock + z
    m_hudRectB_clockHi = 0;
    m_hudRectB_z = 0;
    m_hudRectB_zHi = 0;
    m_hudRectA_clock = 0; // +0x338  HUD-rect group A
    m_hudRectA_clockHi = 0;
    m_hudRectA_z = 0;
    m_hudRectA_zHi = 0;
    m_beltLast = 0;     // +0x4d0  (64-bit)
    m_beltInterval = 0; // +0x4d8  (64-bit)
    m_fallLast = 0;     // +0x4f0
    m_fallDelay = 0;
    m_destructWarnLast = 0; // +0x560
    m_destructWarnDelay = 0;
    // +0x1c8..+0x200: the fifteen per-tab sprite widgets, as FIFTEEN INDIVIDUAL stores
    // (retail schedules the m_destructButton store between m_tabSprite10/m_tabSprite11).
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
    m_destructButton = 0; // +0x618
    m_tabSprite11 = 0;
    m_tabSprite12 = 0;
    m_tabSprite13 = 0;
    m_tabSprite14 = 0;
    m_barSprite = 0;                                     // +0x08
    m_c = 0;                                     // +0x0c
    m_rect14.m_c = 0;                            // +0x20  (the rect block's 4th int; only this one)
    m_activeTab = 0;                             // +0x10c
    m_hitTestDisabled = 0;                       // +0x354
    m_tabsBuilt = 0;                             // +0x358
    m_toggleActive = 0;                          // +0x550
    m_toggleHandle = 0;                          // +0x554
    m_barFrameGate = 0x1e0;                      // +0x614
    m_tabCycle = 0;                              // +0x62c
    memset(m_statFlags, 0, sizeof(m_statFlags)); // +0x114 (0xf dwords, rep stosd)
    memset(m_hitRects, 0, sizeof(m_hitRects));   // +0x150 (0xf, rep stosd)
    memset(m_statObj, 0, sizeof(m_statObj));     // +0x18c (0xf, rep stosd)
    memset(m_slotNotify, 0, sizeof(m_slotNotify)); // +0x204 (5, unrolled)
    memset(m_hlNotify, 0, sizeof(m_hlNotify));     // +0x498 (0xc, rep stosd)
    m_groupNotify[0] = 0;                          // +0x308 x3 (individual stores)
    m_groupNotify[1] = 0;
    m_groupNotify[2] = 0;
    m_61c[0] = 0; // +0x61c x4 (individual stores)
    m_61c[1] = 0;
    m_61c[2] = 0;
    m_61c[3] = 0;
    m_notify0 = 0; // +0x364..+0x370, in retail's 364/36c/370/368 order
    m_notify2 = 0;
    m_notify3 = 0;
    m_notify1 = 0;
    m_extraNotify0 = 0;   // +0x4e0
    m_extraNotify1 = 0;   // +0x500
    m_machineDisplay = 0; // +0x348
    m_modeNotify = 0;     // +0x570
    m_gaugeNotify = 0;    // +0x218
    m_gaugeSink = 0;      // +0x21c
    m_gaugeTarget = 0;    // +0x29c  (stored BEFORE m_gauge in retail)
    m_gauge = 0;          // +0x298
    m_544 = 1;            // +0x544
    m_hlBusy = 0;         // +0x548
    m_retabNotify = 0;    // +0x54c
    m_modeArmed = 0;      // +0x574
}

#endif // GRUNTZ_SBI_RECTONLY_H
