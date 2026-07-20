// StatusBarMgr.h - the in-game status-bar manager (C:\Proj\Gruntz).
//
// ONE canonical CStatusBarMgr: the 0x630-byte, NON-POLYMORPHIC status-bar host that
// CPlay owns through its +0x2dc pointer, plus every engine-referent view its ~70
// reconstructed methods drive.
//
// This file models a CONFLATION the binary separates into two genuinely different
// classes:
//
//   * the REAL CSBI_RectOnly: a 0x30-byte POLYMORPHIC sub-widget (RTTI
//     .?AVCSBI_RectOnly@@, vtable 0x1eab8c, ctor 0x101fa0, dtor 0x100700), the
//     empty intermediate CSBI_Image : CSBI_RectOnly : CStatusBarItem. It is now
//     modeled - correctly and alone - in <Gruntz/SBI_Image.h>.
//   * the HOST modeled here: 0x630 bytes, no vtable, ~70 tab/gauge/slot/hit-rect
//     methods. It is NOT a CStatusBarItem and never was.
//
// WHY THE HOST IS CStatusBarMgr (evidence, not a guess). The tree already carried a
// SECOND, independently-reconstructed view of this same object under the name
// CStatusBarMgr (the LoadTabSprites builder TU). The two views are the same class:
//   1. CPlay::LoadGameAssetNamespaces @0xc7ec0 does `push 0x630; call ??2@YAPAXI@Z` and stores the
//      result at CPlay+0x2dc. The ctor is INLINED there. It contains NO vptr store
//      and NO ??_7 relocation anywhere => the host has no vtable.
//   2. That inlined ctor runs an EH-vector-ctor over the +0x2c region:
//      `lea edx,[esi+0x2c]; push 0x1c; push 8` = 8 x 0x1c = 8 x sizeof(CPtrList),
//      spanning 0x2c..0x10c. Those are exactly the five per-tab CPtrLists the
//      CStatusBarMgr view had at 0x48/0x64/0x80/0x9c/0xb8 - i.e. elements [1]..[5] -
//      with its tab selector m_10c landing immediately after the array. (The tab
//      selector runs 1..5, and element(i) = 0x2c + i*0x1c gives precisely those five.)
//   3. CStatusBarMgr::LoadTabSprites @0x102250 reads `[esi+0x10c]` (the tab selector)
//      and appends through `lea ecx,[esi+0x64]` / `[esi+0x80]` - the SAME +0x2c array
//      and SAME +0x10c field this host uses. Same `this`, same object.
//   4. Our own source had already been casting one pointer to BOTH names on adjacent
//      lines (SBI_MenuItem.cpp: `((CSBI_RectOnly*)host)->ClearTabGroup();` then
//      `((CStatusBarMgr*)host)->LoadTabSprites();`), and SBI_RectOnly.cpp's SetTab
//      called `((CStatusBarMgr*)this)->LoadTabSprites()` - a cross-cast of `this`.
//   5. Offset 0 is WRITTEN (`*(i32*)this = state`) and persisted to the registry as
//      "StatusBar Position". A polymorphic class cannot have its vptr overwritten;
//      +0x00 is the i32 m_position -
//      which is what the CStatusBarMgr view independently called "m_00, status-bar
//      side/mode selector".
// Both reconstructions also agree, offset for offset, at 0x10c / 0x218 / 0x21c /
// 0x298 / 0x308 / 0x348 / 0x364..0x370 / 0x4cc / 0x4e0 / 0x4ec / 0x500..0x520 /
// 0x61c..0x62c, and the last field (0x62c) puts the size at exactly the 0x630 the
// allocation site pushes.
//
// THE NAME IS NOT RTTI-PROVEN, AND CANNOT BE. RTTI type descriptors hang off vtables;
// this class has no vtable, so it has no RTTI record and there is no name to recover
// from the file (`.?AVCStatusBarMgr@@` is absent from the binary's 231 RTTI names -
// checked). `CStatusBarMgr` is therefore a RECONSTRUCTION PLACEHOLDER inherited from
// the pre-existing view, kept because it is the tree's established name for this exact
// object and because the alternative - minting a third name - would be a fabrication.
// Treat the NAME as unproven; the LAYOUT and the identity-with-CStatusBarMgr are proven.
#ifndef GRUNTZ_CSTATUSBARMGR_H
#define GRUNTZ_CSTATUSBARMGR_H

class CSBI_ImageSet; // notify-field element (slot-12 Notify receiver)
class CWarpStoneFly;
// The per-tab menu-item widget. Only pointers are needed here, so it is forward-
// declared: the real chain class lives in <Gruntz/SBI_MenuItem.h>, and the host TU's
// widget-builder view is <Gruntz/StatusBarTabWidgets.h>.
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

// The 0x18-byte slot record. SIX dwords - the trailing "pad" is really m_10/m_14,
// recovered from the element default ctor, which zeroes +0x08/+0x10/+0x0c/+0x14 in
// exactly that store order (and leaves m_state/m_value alone - ArmSlot seeds those).
//
// TWO CLASSES, and the split is BINARY-EVIDENCED, not a modeling choice: retail
// constructs m_slots[5] (+0x220) with an INLINED 5-iteration zero loop, but hands
// m_groupSlots[3] (+0x2c0) and m_hlGrid[12] (+0x378) to the vector-ctor iterator with
// a POINTER to the out-of-line COMDAT ctor 0xc86d0 (`mov eax,ecx; xor ecx,ecx; mov
// [eax+8],ecx; mov [eax+0x10],ecx; mov [eax+0xc],ecx; mov [eax+0x14],ecx; ret`).
// One class cannot have both ctor linkages, so the element type whose ctor is inline
// (CSbiSlot) is distinct from the one whose ctor is out-of-line (CSbiHlRow) - even
// though their LAYOUTS are identical.
//
// The fold at 0xc86d0 proves m_groupSlots and m_hlGrid are ONE class - they share that
// single out-of-line ctor; m_groupSlots is CSbiHlRow.
// (m_slots is NOT part of this fold - see the ctor-linkage evidence above.)

// m_state: 0 = armed, 2 = ready (see kSlotArmed/kSlotReady).
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

// The out-of-line-ctor twin (m_groupSlots[3] @+0x2c0 and m_hlGrid[12] @+0x378).
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

// The CSBI serialization stream (archive) is the shared WAP32 CSerialArchive (Read @
// vtable +0x2c / Write @ +0x30 - the store/transfer slot), now the one modeled class in
// <Gruntz/SerialArchive.h>.

// The +0x8 object carries a sequence id at +0x188 (read during serialize).
struct CSbiSeqHolder {
    char m_pad0[0x188];
    i32 m_188; // +0x188
};
SIZE_UNKNOWN(CSbiSeqHolder);

// The per-tab sprite/menu widgets (m_tabSprite0..14) are the real CSBI_MenuItem
// (vtable 0x1eab4c; defined below): ClearTabSprites drives Blit() (0xe84f0),
// SetTabState shows the selected tab via SetState(state,1) (0xe8310) and hides the
// rest via ProbeState(state) (0xe8480), the game-tab button builders resolve the
// asset frame via ResolveFrame(key,on) (0xe81e0), and Refresh() is the slot-10
// virtual (CSbiTab). The four non-virtual methods are declared on CSBI_MenuItem below
// (reloc-masked
// call rel32 to the real rvas bound in SBI_MenuItem.cpp).

// A hit-test rect widget held in m_hitRects[] / the hit-test lists: a polymorphic
// object (vptr at +0) with the m_enabled gate at +4, m_xLo/m_xHi the x span, and
// m_yLo/m_yHi the y span. Two click handlers dispatch through vtable slots 7/9.
class CSbiRect {
public:
    virtual void Destroy();                    // slot 0  scalar-deleting dtor
    virtual void Serialize();                  // slot 1
    virtual void Setup();                      // slot 2
    virtual void ClearFrame();                 // slot 3
    virtual void Poll();                       // slot 4
    virtual void Tick();                       // slot 5
    virtual void HitHandlerA();                // slot 6
    virtual void Click1c(i32 a, i32 b, i32 c); // +0x1c (slot 7)
    virtual void HitHandlerC();                // slot 8
    virtual void Click24(i32 a, i32 b, i32 c); // +0x24 (slot 9)
    i32 m_enabled;                             // +0x04 enabled (== the toggle "active" flag)
    i32 m_kind;                                // +0x08 widget kind tag
    i32 m_cmd;                                 // +0x0c command id
    i32 m_tab;                                 // +0x10 owning tab index
    i32 m_xLo;                                 // +0x14 x lo
    i32 m_yLo;                                 // +0x18 y lo
    i32 m_xHi;                                 // +0x1c x hi
    i32 m_yHi;                                 // +0x20 y hi
    char m_pad24[0x44 - 0x24];                 // +0x24
    i32 m_toggleValue; // +0x44  latched statz-toggle value (LoadStatzTabToggleSprite;
                       //        ex the CStatzTabItem view of this same +0x150 element)
};
SIZE_UNKNOWN(CSbiRect);

// The ConfigureRect host (its arg2), its lookup map + record come from the shared
// canonical family (<Gruntz/SbiConfig.h>): CDDrawSurfaceMgr / CSbiConfigMap /
// CSbiConfigRecord (host->m_10 carries the CMapWordToOb map at its +0x10).

// A per-stat widget object (m_statObj[]): a sibling thunk drives its (tag,on)
// notifier; the call is reloc-masked, so only the arg shape is load-bearing.
struct CSbiStatObj {
    // Notify2 @0x27f7 IS CSBI_RectOnly::ResetGroupA (args reloc-masked); cast at the call.
    // The (stateId, on) toggle notifier (thunk FUN_004ea170; ex CStatzTabSub::Toggle).
    void Toggle(i32 stateId, i32 on);
};
SIZE_UNKNOWN(CSbiStatObj);

// (CSbiSlotPtr is GONE - the +0x204 elements are the real CSBI_ImageSet family;
// its "slot 12" Notify(i32) is CSBI_ImageSet's own slot-12 virtual, slot 10 is
// CStatusBarItem::SetSubtype. See docs task #22 / the vtable atlas.)

// A singly-linked notify node (the +0xbc list element): next ptr at +0, the
// payload object at +8. The payload's vtable slot 0 is a __thiscall void(int).
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

// A wider view of the notify payload: +0x10 takes the notify value (the destruct-
// button walks call it as void(int)); +0x14 / +0x28 are argless refreshers (the
// main-status-bar walks). The vtable slots are the load-bearing fact; the non-
// virtual arg shapes are reloc-masked.
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

// The GAME_DESTRUCT sound-cue config record (looked up in the music-host cue map): +0x10
// is the pooled cue play-factory DSoundCloneInst, whose GetItem() mints/pulls a
// DirectSoundMgr clone that ApplyAndPlay's the warning tone. (The ex CSbiSpriteFactory /
// CSbiDisplayObj empty views were the real Dsndmgr types - proven by the ApplyAndPlay /
// StopAndRewind / GetItem calls in LoadDestructButtonSprite.)
struct CSbiSpriteCfg {
    char m_pad0[0x10];
    DSoundCloneInst* m_playFactory; // +0x10  pooled cue play-factory (GetItem -> DirectSoundMgr)
};
SIZE_UNKNOWN(CSbiSpriteCfg);

// The lazily-created 0x40-byte object at CSBI_RectOnly+0x54c: EnsureSub `new`s and
// Init()s it on first use; the retab teardown drives Refresh()/Notify0() and frees it.
// One object, one type (same offset, same lazy-create/free-on-retab lifecycle).
SIZE(CWarpStoneFly, 0x40);

// (the eight lists at +0x2c are real MFC CPtrLists -
// the EH-vector-ctor's 8 x 0x1c is sizeof(CPtrList) - so the host walks them through
// CPtrList::GetHeadPosition()/GetNext(), and RemoveAll() needs no cast.)

// The pooled-ptr collection embedded at +0x530 is an MFC CPtrArray whose head (vptr)
// sits at +0x530 and whose m_pData/m_nSize are the m_ptrTable/m_ptrCount fields of
// CSBI_RectOnly; teardown frees it via CPtrArray::SetSize(0,-1) (0x1b4f75, cast at call).
// (CSbiPtrCollection / CSbiPtrColl2 are GONE: the +0x530 pooled-ptr collection IS MFC
//  ::CPtrArray.  PROVEN from the binary - the methods it called (0x1b5144 SetAtGrow /
//  0x1b516b InsertAt / 0x1b5200 RemoveAt) sit in the CPtrArray band [0x1b4f0b, 0x1b527e),
//  whose ctor 0x1b4f0b stamps a vtable the MFC CRuntimeClass names "CPtrArray"; and the
//  hand-named m_ptrTable(+0x534) / m_ptrCount(+0x538) ARE its m_pData / m_nSize.)

// The gauge notifier (m_gaugeNotify/m_gaugeSink): the value sink carries m_44 (set to the gauge
// reading) and a refresh slot at vtable index 0x28 (slot 10).
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

// The global attribute-config manager (?g_buteMgr, VA 0x6453d8). GetIntDef/GetInt
// (the StatusBar delay/speed lookups) are on the canonical CButeMgr
// (include/Bute/ButeMgr.h); the reloc-masked __thiscall object's DIR32 name is
// load-bearing (mangles ...@@3VCButeMgr@@A). Extern only (bound by another TU).
extern CButeMgr g_buteMgr;

// A phase-timer record overlaid on a 24-byte slot (m_groupSlots element / the HUD-
// rect blocks reused as timers by the rez-machine/conveyor state machines): a phase
// state + counter, then a 64-bit last-draw-clock and a 64-bit interval. The gate is
// `(i64)(u32)g_dat645588 - m_last >= m_interval` (the sub/sbb + signed 64-bit compare).
struct SbiPhaseSlot {
    i32 m_state;    // +0x00
    i32 m_counter;  // +0x04
    i64 m_last;     // +0x08  last draw-clock (64-bit)
    i64 m_interval; // +0x10  wait interval (64-bit)
};
SIZE_UNKNOWN(SbiPhaseSlot);

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

// ---------------------------------------------------------------------------
// CStatusBarMgr - the 0x630 status-bar host (CPlay+0x2dc). NON-POLYMORPHIC: no vptr,
// no vtable, no RTTI (see the file banner for the proof). Its first word is a real
// data member (m_position), not a vptr - the code writes it.
//
// It owns the five per-tab widget lists (Statz / Gruntz / Resource / Multiplayer /
// Game) as elements [1]..[5] of an eight-element CPtrList array at +0x2c, built by
// the EH-vector-ctor in CPlay::LoadGameAssetNamespaces. LoadTabSprites() (0x102250) is the big per-tab
// builder: it dispatches on the current tab index (m_activeTab, 1..5) and, for the
// selected tab, creates each widget, configures it from a named sprite-asset key + a
// geometry CRect, and appends it to that tab's list.
//
// Only offsets + code bytes are load-bearing; field names are placeholders for the
// engine identities recovered from the member writes.
// ---------------------------------------------------------------------------
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
    CSbiRect* HitTestRects(i32 x, i32 y);
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
    i32 m_8;        // +0x08
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
    CSbiRect* m_hitRects[15];     // +0x150  hit-test rect widgets
    CSbiStatObj* m_statObj[15];   // +0x18c  per-stat object array (notified on clear)
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
    i32 m_fallLast;              // +0x4f0  falling-item timer last draw-clock = g_dat645588
    i32 m_fallLastHi;            // +0x4f4
    i32 m_fallDelay;             // +0x4f8  falling-item config delay
    i32 m_fallDelayHi;           // +0x4fc
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
    i32 m_destructWarnLast;       // +0x560  destruct-warning last draw-clock
    i32 m_destructWarnLastHi;     // +0x564
    i32 m_destructWarnDelay;      // +0x568  destruct-warning delay (config)
    i32 m_destructWarnDelayHi;    // +0x56c
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
// 0x630 - the allocation site, not an inference: CPlay::LoadGameAssetNamespaces @0xc7fea does
// `push 0x630; call ??2@YAPAXI@Z` and stores the result at CPlay+0x2dc.
SIZE(CStatusBarMgr, 0x630);

// The cue-lookup string map embedded at host->m_28 + 0x10 (CMapStringToOb).
// (The member is the real MFC CMapStringToOb; Lookup @0x1b8438.)

// A resolved cue record: a player at +0x10 plus a draw-clock gate (+0x14 last,
// +0x18 interval). Same shape as GameMode's CBootyFound. (DSoundCloneInst fwd-declared above.)
struct CSbiCueRecord {
    char m_pad0[0x10];
    DSoundCloneInst* m_10; // +0x10  player (ConfigureItem this)
    i32 m_14;              // +0x14  last draw-clock
    i32 m_18;              // +0x18  interval
};
SIZE_UNKNOWN(CSbiCueRecord);

// The cue player (ConfigureItem == FUN_005360d0, __thiscall, ret 0x10).

// CONSOLIDATION NOTE (g_gameReg->m_world world/resource-mgr views): CSbiGameMgr is the
// canonical CDDrawSurfaceMgr (<Gruntz/ResMgr.h>) - g_gameReg->m_world, whose m_8 (CDDrawChildGroup, the
// Deserialize seq map @+0x48) and m_28 (the sound object) were verified as the same
// slots CDDrawSurfaceMgr models. CSbiMusicHost is the +0x28 sound object viewed as its CUE
// facet (the same shape as SBI_MenuItem's CMiMusicHost: +0x30 gate, cue map @+0x10),
// a SIBLING of CDDrawSurfaceMgr::m_28 (CDDrawSubMgrLeafScan, the install facet). The fold to CDDrawSurfaceMgr
// is DEFERRED here (documented, not fabricated): this 4469-line, mostly-@early-stop TU
// reaches m_30 through its own CGameReg singleton, so pulling in <Gruntz/ResMgr.h>
// risks broad codegen shifts across its ~70 functions, and the m_28 cue access needs a
// multi-view cast at ~7 sites (the CDDrawSubMgrLeafScan-vs-cue-host facet split is not
// settled by the delinked bytes). Kept as the per-TU view for the final sweep.

// The music host chain: g_gameReg->m_world->m_soundRegistry->{m_30 gate, Lookup map @+0x10}
// (== CDDrawSurfaceMgr::m_28 viewed as its cue facet; see the consolidation note above).
struct CSbiMusicHost {
    char m_pad0[0x10];         // +0x00..0x0f
    CMapStringToOb m_map10;    // +0x10  cue lookup map (CMapStringToOb view)
    char m_pad11[0x30 - 0x11]; // +0x11..0x2f
    i32 m_30; // +0x30  reentrancy gate flag (opaque; only null-tested => skip the cue play)
};
SIZE_UNKNOWN(CSbiMusicHost);

// The active game manager (g_gameReg->m_world == CDDrawSurfaceMgr): carries the main
// status-bar draw chain at +0x04 and the sound object / music host at +0x28. See
// the consolidation note above (fold to CDDrawSurfaceMgr deferred).
struct CSbiMainL1; // +0x04 draw chain (defined below); the main-bar sprite path
struct CSbiGameMgr {
    char m_pad0[0x4];
    CSbiMainL1* m_4; // +0x04  main-status-bar draw chain (m_4->m_14->m_2c setup)
    char m_pad8[0x28 - 0x8];
    CSbiMusicHost* m_musicHost; // +0x28  music host (CDDrawSurfaceMgr::m_28 cue facet)
};
SIZE_UNKNOWN(CSbiGameMgr);

// The current play-state at g_gameReg->m_curState
// (+0x2c) is the real CPlay (Play.h). Its highlight methods are CPlay methods, reached
// via ((CPlay*)g_gameReg->m_curState) - a genuine CState->CPlay downcast (m_curState is
// canonically CState*): ScrollTo->ResetGoals (0xd5f00), Refresh->ResetViewport (0xd8c60),
// PostWarn->ArmSnapshot (0xd9240), HiToggle->EnterOverlayDrag (0xd6440), SetState (0xd5b20),
// HiRefresh (0xd6560), and the +0x4f0 highlight-busy gate (CPlay::m_4f0).

// (the m_grid[] cell
//  is the real ::CGrunt (the CTmCell typedef in TriggerMgr.h) and its +0x10
//  sub-object the real CGruntHud, whose m_screenX/m_screenY (+0x5c/+0x60) are the
//  origin pair PlaceCursorTarget forwards to ResetGoals - the same pair every
//  other CGruntHud consumer reads. SBI_RectOnly.cpp reaches them cast-free.)

// The single-player active object at
// g_gameReg->m_68 (+0x68) is the real CTriggerMgr (TriggerMgr.h), reached via
// ((CTriggerMgr*)g_gameReg->m_68) - a genuine per-mode-reused void* slot (grid/goo-
// well/light-fx in other modes). ProbeXY->ResetCell (0x46bfd0), ScrollProbe->
// RecordListHas (0x4784d0), LoadCameraSprite (0x478960); the placed-cursor latch trio
// is CTriggerMgr m_230/m_recX/m_recY, m_288 == m_288, m_400 == m_groupFlag. The grid
// element type is CTmCell (still an unmatched engine cell).

// The settings/registry writer at
// g_gameReg->m_settings (+0x38) is the real Utils::RegistryHelper (RegistryHelper.h),
// reached via ((Utils::RegistryHelper*)g_gameReg->m_settings). LogPos->SetValueDword,
// QueryPos->GetValueDword (0x1395d0). (m_settings stays void* in the canonical header -
// its MFC-side dual-view types it CSettingsWriter*, owned by a parallel worker.)

// The window host at g_gameReg->m_gameWnd (+0x4): carries the game HWND at +0x4 (the
// PostMessage target). Reached as ((CSbiWndHost*)g_gameReg->m_gameWnd) - the +0x4
// sub-object of the canonical CGameRegistry, not a facet of the singleton itself.
struct CSbiWndHost {
    char m_pad0[0x4];
    HWND m_4;  // +0x4  game window handle
};
SIZE_UNKNOWN(CSbiWndHost);

// The *0x24556c singleton is the canonical CGameRegistry (<Gruntz/GameRegistry.h>).
// The scalar slots are canonical members reached cast-free (m_c -> m_frameGate +0xc,
// m_10 -> m_soundEnabled +0x10, m_modeW/m_modeH/m_11c/m_134); the pointer slots are the
// canonical sub-object members reached through evidence-backed downcasts to the REAL
// concrete classes (all sema-proven; matching-neutral - a downcast of a same-offset slot
// is a free reinterpret). The former per-TU facet VIEWS are all removed in favour of the
// real classes (SBI_RectOnly.cpp includes their headers):
//   m_curState (+0x2c) -> (CPlay*)               the current play-state (Play.h)
//   m_settings (+0x38) -> (Utils::RegistryHelper*) the registry writer (RegistryHelper.h)
//   m_68 (+0x68)       -> (CTriggerMgr*)          the single-player trigger grid (TriggerMgr.h)
// Two slots keep a small TU-local facet view (documented, honest): m_gameWnd (+0x4) ->
// (CSbiWndHost*) (a 1-field window-host shell) and m_world (+0x30) -> (CSbiGameMgr*) (the
// resource mgr / CDDrawSurfaceMgr; its fold is DEFERRED - a deep CDDrawSurfaceMgr sub-object cascade, see
// the CSbiMusicHost/CSbiMainL1 note above and GameRegistry.h's m_world typing).
// The three TU-specific methods are the real CGruntzMgr methods: Fn29aa->UpdateScoreHud
// (0x860b0), HiPump->AccrueScoreTime (0x861e0),
// SetToggle->FinishLevel (0x8e980); ReportError(i32,i32) added as the i32,i32 overload.
// The dead inline ViewSize() (unused - 0xfe520 reads m_modeW/m_modeH directly) is dropped.

// The seq-keyed object map at (g_gameReg->m_world->m_childGroup + 0x48): Lookup(key, &out)
// returns found (CMapWordToOb::Lookup-style; reloc-masked sibling).
struct CSbiSeqMap {}; // MFC CMapPtrToPtr (Lookup @0x1b8760); cast at the call
SIZE_UNKNOWN(CSbiSeqMap);

// The looked-up object whose vtable slot 8 (+0x20) returns a type tag (== 5
// validates it as the restored sequence holder stored back into m_8).
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

// The notify-payload object the +0x8 holder points at: an event flags pair the
// reset latches (an "abort"/"dirty" bit OR'd into +0x40 and +0x8).
struct CSbiResetHost {
    char m_pad0[0x8];
    i32 m_8; // +0x08  status flags (|= 0x10000)
    char m_padc[0x40 - 0xc];
    i32 m_40; // +0x40  control flags (|= 1)
};
SIZE_UNKNOWN(CSbiResetHost);

// The m_8 render object the item drives: a screen-position pair at +0x5c/+0x60
// and a layer descriptor at +0x198 whose +0x10/+0x14 origin and +0x18/+0x1c
// inset frame the hit-test rect. (m_8 is the base CStatusBarItem int overlaid as
// a pointer, same authentic int-as-pointer overlay as Serialize uses.)
struct CSbiLayer {
    char m_pad0[0x10];
    i32 m_10, m_14; // +0x10/+0x14  rect origin (lo X / lo Y)
    i32 m_18, m_1c; // +0x18/+0x1c  inset (added to the shifted position)
};
SIZE_UNKNOWN(CSbiLayer);
struct CSbiRenderObj {
    char m_pad0[0x5c];
    i32 m_5c, m_60; // +0x5c/+0x60  screen position
    char m_pad64[0x198 - 0x64];
    CSbiLayer* m_layer; // +0x198  layer descriptor
};
SIZE_UNKNOWN(CSbiRenderObj);

// A free-list node {m_0, m_4}; m_0 doubles as the link, m_4 is the sort key.
struct CSbiFreeNode {
    i32 m_0, m_4;
};
SIZE_UNKNOWN(CSbiFreeNode);

// (the tab-widget views CSbiRectSub / CSBI_MenuItem moved to
// <Gruntz/StatusBarTabWidgets.h> - they are instantiated only by the host's own
// builder TU, and keeping them out of this header is what lets TUs that carry the
// REAL chain classes include the host class without a redefinition clash.)

// The MULTIPLAYERTAB frame descriptor (widget->m_38): a frame-index gate
// (+0x64/+0x68) plus a value table (+0x14 -> +0x10).
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

// The per-tab widget list at this+0x2c (AddTail on each created widget).
struct CTabList {};
SIZE_UNKNOWN(CTabList);

// The main-bar setup chain hung off the game-manager: m_30->m_4->m_14->m_2c drives a
// 2-arg rect setter; m_30->m_4->m_14 is also handed to the frame-draw helper.
// (CSbiMainL1/CSbiMainL2/CSbiMainSetup are DISSOLVED, 2026-07-19 - offset-proven
// against the real DDraw chain: L1 == CDDrawSubMgrPages (+0x14 = m_backPair),
// L2 == CDDrawSurfacePair (+0x2c = m_surface), and the "setup" IS the CDDSurface
// (the old (CDDSurface*)tgt Restore cast was the confession). The main-bar chain
// in real terms: world->m_drawTarget->m_backPair->m_surface.)
SIZE_UNKNOWN(CSbiMainL1);
// The resolved GAME_STATUSBAR_MAINBAR cfg record: a frame-entry table at +0x14 indexed
// by +0x64; each entry carries an origin pair at +0x18/+0x1c.
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
// The frame-draw helper (__stdcall, callee-cleans 0x10): the chain object plus the two
// composed origins.
class CDDrawSurfacePair; // the real main-bar draw receiver (ex the CSbiMainL2 facet)
void __stdcall MainBarDrawFrame(CDDrawSurfacePair* obj, i32 x, i32 y, i32 flag); // 0x153790 (NOTE:
// 0x153790 is also bound as CImage::RenderFrame - the pair receiver here is that call's
// object-with-frames facet; arbitration pending)

// The hit-tested tab-highlight widget resolved by HiResolve: polymorphic (Update
// at vtable slot 6 = +0x18); the command id at +0xc and the widget kind at +0x10
// (the outer switch key). Reloc-masked non-virtual siblings otherwise.
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

// ---------------------------------------------------------------------------
// CStatusBarMgr::CStatusBarMgr - the real inline default ctor.
//
// The MEMBER CONSTRUCTIONS the compiler now emits are exactly retail's, in declaration
// (== ascending offset) order:
//   m_tabLists[8]  @+0x2c   -> __ehvec_ctor (0x11f5a0): CPtrList has a dtor, so the EH
//                              vector-ctor iterator runs (retail: push ??1CPtrList /
//                              push ??_FCPtrList / 8 / 0x1c / base). EH states 0/1.
//   m_slots[5]     @+0x220  -> the inline CSbiSlot ctor, unrolled as retail's loop.
//   m_groupSlots[3]@+0x2c0  -> vector-ctor iterator (0x7c20) + &CSbiHlRow::CSbiHlRow
//   m_hlGrid[12]   @+0x378  -> the SAME iterator + the SAME ctor pointer (0xc86d0)
//   m_ptrPool      @+0x530  -> ??0CPtrArray (0x1b4f0b)
// then the scalar body below. Retail's /O2 scheduler interleaves the stores with the
// array-ctor calls; that interleave is the compiler's, not the source's.
// ---------------------------------------------------------------------------
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
    m_fallLastHi = 0;
    m_fallDelay = 0;
    m_fallDelayHi = 0;
    m_destructWarnLast = 0; // +0x560
    m_destructWarnLastHi = 0;
    m_destructWarnDelay = 0;
    m_destructWarnDelayHi = 0;
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
    m_8 = 0;                                     // +0x08
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

// --- vtable catalog (view/base classes bound to their unit vtable rva) ---

// BuildStatusBarTabs' record ctor stamps 0x1eab8c == CSBI_RectOnly's bound table;
// the sub-record's cl-emitted ??_7 reloc-masks it.
// DISSOLUTION ATTEMPTED AND REFUTED (2026-07-19): inlining the real CSBI_RectOnly
// ctor (to let the three widget `new` sites inline it and kill this alias) makes cl
// inline it at depth-2 inside BuildTabzDialog's imageset ctor TOO - where retail
// CALLS the out-of-line 0x101fa0 - and NO obj then emits the COMDAT for a pin
// (measured: 0x101fa0 unbound, net -2 exact + tree-wide type-table butterflies).
// Retail's two ctor flavors (3 inline sites + 1 called site + the 0x101fa0 body)
// cannot come from one plain inline definition under our cl; until that mechanism
// is found, the CSbiRectSub view IS the inline flavor and this alias is the
// measured-necessary record of its vtable identity.
RELOC_VTBL(CSbiRectSub, 0x001eab8c);

#endif // GRUNTZ_SBI_RECTONLY_H
