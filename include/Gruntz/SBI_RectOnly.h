// SBI_RectOnly.h - Gruntz CSBI_RectOnly (C:\Proj\Gruntz), the FULL method view.
//
// One canonical home for CSBI_RectOnly (the big status-bar HOST item, vtable
// 0x5eab8c) plus every engine-referent view its ~70 reconstructed methods drive.
// These were per-TU inline views in SBI_RectOnly.cpp; consolidating them into this
// shared header is pure code motion (matching-neutral: same layouts, same sizes,
// same virtual counts -> identical codegen), and gives the referent shapes a single
// definition the *Eh.cpp dtor TUs / a final sweep can reuse.
//
// TWO-VIEW SPLIT: this is the FULL-layout method view of CSBI_RectOnly. It is
// deliberately NOT co-included with <Gruntz/SBI_Image.h> (the frameless empty
// `class CSBI_RectOnly : CStatusBarItem {}` intermediate) nor <Gruntz/SbiDtorChain.h>
// (the /GX dtor-chain grand-base): one MSVC5 spelling emits only one of those shapes.
// See the two-view-split note atop <Gruntz/StatusBarItem.h>.
#ifndef GRUNTZ_SBI_RECTONLY_H
#define GRUNTZ_SBI_RECTONLY_H

class CWarpStoneFly; // folded CSbiMode54c
class CSBI_MenuItem; // the per-tab menu-item widget (defined below; folded CSbiSprite view)

#include <Ints.h>
#include <Gruntz/SoundCueMgr.h>
#include <rva.h>
#include <Mfc.h>
#include <Bute/ButeMgr.h>         // canonical CButeMgr (one shape)
#include <Gruntz/GameRegistry.h>  // canonical CGameRegistry (the one *0x24556c singleton)
#include <Gruntz/SbiConfig.h>     // canonical config-host family (one shape)
#include <Gruntz/SerialArchive.h> // the shared CSerialArchive stream (Read @+0x2c / Write @+0x30)
#include <Gruntz/StatusBarItem.h>

// A 24-byte (0x18) slot record: the +0x208 array element.
// m_state: 0 = armed, 2 = ready (see kSlotArmed/kSlotReady).
struct CSbiSlot {
    i32 m_state; // +0x00 (rel +0x208)
    i32 m_value; // +0x04 (rel +0x20c)
    i32 m_8;     // +0x08 (gauge-span hi / used by the SetGaugeSpan inline)
    i32 m_c;     // +0x0c
    char m_pad10[0x18 - 0x10];
};
SIZE_UNKNOWN(CSbiSlot);

// A 24-byte highlight-row record (the +0x378 array element).
struct CSbiHlRow {
    i32 m_state;  // +0x00 state
    i32 m_handle; // +0x04 handle value passed to the notify pointer
    char m_pad8[0x18 - 0x8];
};
SIZE_UNKNOWN(CSbiHlRow);

// The CSBI serialization stream (archive) is the shared WAP32 CSerialArchive (Read @
// vtable +0x2c / Write @ +0x30 - the store/transfer slot), now the one modeled class in
// <Gruntz/SerialArchive.h> - the former local `CSbiStream` view is folded away.

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
// virtual (CSbiTab). The former fake `class CSbiSprite` view is folded away; the
// four non-virtual methods are declared on CSBI_MenuItem below (reloc-masked
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
    i32 m_enabled;                             // +0x04 enabled
    i32 m_kind;                                // +0x08 widget kind tag
    i32 m_cmd;                                 // +0x0c command id
    i32 m_tab;                                 // +0x10 owning tab index
    i32 m_xLo;                                 // +0x14 x lo
    i32 m_yLo;                                 // +0x18 y lo
    i32 m_xHi;                                 // +0x1c x hi
    i32 m_yHi;                                 // +0x20 y hi
};
SIZE_UNKNOWN(CSbiRect);

// The ConfigureRect host (its arg2), its lookup map + record come from the shared
// canonical family (<Gruntz/SbiConfig.h>): CSbiConfigHost / CSbiConfigMap /
// CSbiConfigRecord (host->m_10 carries the CMapWordToOb map at its +0x10).

// A per-stat widget object (m_statObj[]): a sibling thunk drives its (tag,on)
// notifier; the call is reloc-masked, so only the arg shape is load-bearing.
struct CSbiStatObj {
    // Notify2 @0x27f7 IS CSBI_RectOnly::ResetGroupA (args reloc-masked); cast at the call.
};
SIZE_UNKNOWN(CSbiStatObj);

// The element type of the +0x204 pointer array: an engine object whose vtable
// slot 0x30 (index 12) is a __thiscall void(int) notifier.
class CSbiSlotPtr {
public:
    virtual void Destroy();      // slot 0  scalar-deleting dtor
    virtual void Serialize();    // slot 1
    virtual void Setup();        // slot 2
    virtual void ClearFrame();   // slot 3
    virtual void Poll();         // slot 4
    virtual void Tick();         // slot 5
    virtual void HitHandlerA();  // slot 6
    virtual void HitHandlerB();  // slot 7
    virtual void HitHandlerC();  // slot 8
    virtual void HitHandlerD();  // slot 9
    virtual void Refresh();      // slot 10
    virtual void Configure();    // slot 11
    virtual void Notify(i32 on); // +0x30 (slot 12)
    char m_pad4[0x14 - 0x4];
    i32 m_rect14[4]; // +0x14  falling-item rect (UpdateFallingItemStatusBar)
};
SIZE_UNKNOWN(CSbiSlotPtr);

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

// A GAME_DESTRUCT-style sprite-config record: +0x10 is a factory whose no-arg
// __thiscall builds the display object (returned, then Configure'd / Release'd).
struct CSbiSpriteFactory {};
SIZE_UNKNOWN(CSbiSpriteFactory);
struct CSbiDisplayObj {};
SIZE_UNKNOWN(CSbiDisplayObj);
struct CSbiSpriteCfg {
    char m_pad0[0x10];
    CSbiSpriteFactory* m_10; // +0x10
};
SIZE_UNKNOWN(CSbiSpriteCfg);

// The lazily-created 0x40-byte object at CSBI_RectOnly+0x54c: EnsureSub `new`s and
// Init()s it on first use; the retab teardown drives Refresh()/Notify0() and frees it.
// One object, one type (former SBI_RectOnlyEh.cpp `CSbiLazySub` folded in - same
// offset, same lazy-create/free-on-retab lifecycle).
SIZE(CWarpStoneFly, 0x40);

// An MFC CPtrList (head node at +4); RemoveAll @0x1b48a6 is CPtrList::RemoveAll, reached via a
// CPtrList cast at the call. Field-view kept (m_head read directly; MFC hides m_pNodeHead).
struct CSbiPtrList {
    char m_pad0[0x4];
    CSbiNotifyNode* m_head; // +0x04 head node pointer
};
SIZE_UNKNOWN(CSbiPtrList);

// The pooled-ptr collection embedded at +0x530 is an MFC CPtrArray whose head (vptr)
// sits at +0x530 and whose m_pData/m_nSize are the m_ptrTable/m_ptrCount fields of
// CSBI_RectOnly; teardown frees it via CPtrArray::SetSize(0,-1) (0x1b4f75, cast at call).
struct CSbiPtrCollection {
    char m_pad0[0x4]; // +0x530  CPtrArray head (vptr slot)
};
SIZE_UNKNOWN(CSbiPtrCollection);

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
    i32 m_44; // +0x44  latched gauge reading
};
SIZE_UNKNOWN(CSbiGaugeNotify);

// The global attribute-config manager (?g_buteMgr, VA 0x6453d8). GetIntDef/GetInt
// (the StatusBar delay/speed lookups) are on the canonical CButeMgr
// (include/Bute/ButeMgr.h); the reloc-masked __thiscall object's DIR32 name is
// load-bearing (mangles ...@@3VCButeMgr@@A). Extern only (bound by another TU).
extern CButeMgr g_buteMgr;

// The rez-machine snooze display object at +0x348 (Update sets it from the HUD-rect
// A/B y-coords). Reloc-masked __thiscall.
struct CSbiMachineDisplay {};
SIZE_UNKNOWN(CSbiMachineDisplay);

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

// @identity-TODO CStatusBar (the CPlay-owned status-bar object at CPlay+0x2dc)
// -------------------------------------------------------------------------
// NAME-CONFLATION (proven): this class is the big ~0x570 status-bar HOST, NOT the
// real CSBI_RectOnly. The real CSBI_RectOnly is the THIN base of CSBI_Image (11-slot
// vtable 0x5eab8c, size <=0x34, ctor 0x101fa0 which only zeroes m_4/m_24/m_28 + m_8=1;
// modeled correctly in <Gruntz/SBI_Image.h>). CSBI_Image (size 0x34) : CSBI_RectOnly
// (RTTI), so the real CSBI_RectOnly CANNOT be 0x570.
//
// The HOST here is a distinct, likely NON-polymorphic object owned by CPlay through a
// pointer at CPlay+0x2dc: PROVEN by DtorMembers (0xc8980) <- CPlay::CPlayDtorBody
// (0xc8700) at 0xc878d `mov esi,[edi+0x2dc]; call 0x1438(->DtorMembers)` - CPlay tears
// the host down via a DIRECT (non-virtual) DtorMembers call, so the host has no ??_7 /
// no RTTI name (its role name CStatusBar is a reconstruction placeholder). The thin
// CSBI_RectOnly dtor 0x100700 calls ONLY DtorRect (never DtorMembers), confirming the
// two are different classes. The ctor 0x101fa0 + the base slots below belong to the
// THIN class; the ~70 tab/gauge/slot/hit-rect methods belong to the HOST.
//
// @identity-TODO: THIS IS A SPLIT, NOT A RENAME - AND THE HOST HALF CANNOT BE NAMED.
// I went to rename the host and the binary said the job is a different one. Evidence:
//
//  1. THE HOST IS NOT POLYMORPHIC. Its object is built by CPlay::Vfunc1 (0xc7ec0):
//     `push 0x630; call ??2@YAPAXI@Z` -> stored at CPlay+0x2dc. The ctor is INLINED there,
//     and it contains NO vptr store (`mov [esi],<vtable>`) and the whole function carries
//     NO ??_7 relocation at all. The host has no vtable.
//  2. THEREFORE ITS NAME IS NOT IN THE BINARY. RTTI type descriptors hang off vtables; a
//     class with no vtable has no RTTI record. There is nothing to recover the host's real
//     name FROM. (CStatusBarMgr, the obvious suspect, is not an RTTI class at all -
//     `sema class CStatusBarMgr` finds nothing. Suspect eliminated, not confirmed.)
//     So I did NOT rename it. Inventing a class name here would plant a lie in a header
//     that six other TUs include.
//  3. IT IS 0x630 BYTES, not the 0x54c previously believed (the alloc site above).
//  4. AND IT IS NOT A CStatusBarItem. Its +0x2c is an ARRAY of 8 x 0x1c records
//     (`lea edx,[esi+0x2c]; push 0x1c; push 8; call 0x11f5a0`), not the base's scalar
//     m_2c - which is exactly why the host code below has to cast
//     `((CRezList*)((char*)this + 0x2c))` to reach it. The cast was the symptom; the
//     fabricated base is the cause.
//  5. THE TWO HALVES ARE PROVABLY DIFFERENT CLASSES, MERGED HERE. The ctor this class
//     binds (0x101fa0) stamps ??_7CSBI_RectOnly@@6B@ (0x5eab8c) and zeroes m_4/m_24/m_28 -
//     that is the REAL, polymorphic, 0x3c sub-widget's ctor, and SBI_Image.h already
//     declares that same class properly. So `CSBI_RectOnly` here = the real 0x3c
//     polymorphic sub-widget (ctor 0x101fa0, Setup 0xe86e0) MERGED WITH a 0x630
//     non-polymorphic host (~70 tab/gauge/slot methods) that has no name in the binary.
//
// The fix is therefore to SPLIT the host out (it keeps a flagged placeholder identity,
// because the binary genuinely cannot name it) and let the sub-widget half dissolve onto
// SBI_Image.h's real CSBI_RectOnly. The host's ~70 methods are referenced by
// CGruntzMgr/CPlay/GooWellMgr/KitchenSlime/DestructButton/ModeObjInit, so the split
// touches those too. Not attempted here: a half-finished split of a class six TUs include
// is worse than an honest TODO, and I would rather hand over the proof than a mess.
// -------------------------------------------------------------------------
// base vftable (CStatusBarItem) anchored out-of-line in this TU.
class CSBI_RectOnly : public CStatusBarItem {
public:
    CSBI_RectOnly();
    // Declared OUT-OF-LINE (no body here): the ONE real ~CSBI_RectOnly is the /GX chain
    // dtor in SBI_RectOnlyDtorEh.cpp (0x100700). Left IMPLICIT, cl5 synthesised a per-TU
    // inline copy in THIS TU and emitted it as a 16-byte COMDAT under the same mangled
    // name - an ODR-divergent duplicate of the real 96-byte body (the inlined base
    // ~CStatusBarItem, with the derived vptr store dead-stored away by /O2). The linker
    // keeps ONE copy per name and may pick the stub over the real teardown; it also made
    // reloc_fidelity score the stub's relocs against retail's real body (the phantom
    // ??1CStatusBarItem MISBOUND row). Declared-only => no definition here.
    virtual ~CSBI_RectOnly(); // 0x00100700 (SBI_RectOnlyDtorEh.cpp)
    virtual i32 SbiVfunc0() OVERRIDE;

    // vtable slot 2 (0xe86e0): the 10-arg setup; inherited by CSBI_Image/_ImageSet.
    i32 Setup(i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6, i32 a7, i32 a8, i32 a9, i32 a10);

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
    i32 Probe2e69(); // 0x2e69 (post-build validity probe)
    i32 Probe41a1(); // 0x41a1 (post-build validity probe)
    i32 winapi_107d00_SetRect();
    i32 LoadBattlezItemConfig(i32);
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
    void LoadStatzTabToggleSprite(i32, i32);
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
    );
    i32 ClickHilite(i32 x, i32 y, i32 z);
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
    // +0x2c is the inherited base CStatusBarItem::m_2c (Setup arg1 target); the base
    // owns it (its slot-2 Setup @0x100660 stores arg1 there), so leaf fields start @0x30.
    i32 m_30; // +0x30
    i32 m_34; // +0x34
    char m_pad38[0xb8 - 0x38];
    CSbiPtrList m_listB8; // +0xb8  per-tab notify list (RemoveAll'd on retab)
    char m_padc0[0xd4 - 0xc0];
    CSbiPtrList m_listD4; // +0xd4  trailing hit-test list (head at +0xd8)
    char m_paddc[0x10c - 0xdc];
    i32 m_activeTab;              // +0x10c  active tab index
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
    CSbiSlotPtr* m_slotNotify[1]; // +0x204  slot pointer array (4-byte stride)
    char m_pad208[0x218 - 0x208];
    CSbiGaugeNotify* m_gaugeNotify; // +0x218  gauge notifier (vfunc 0x28)
    CSbiGaugeNotify* m_gaugeSink;   // +0x21c  gauge value sink (m_44 = gauge; vfunc 0x28)
    CSbiSlot m_slots[1];            // +0x220  24-byte slot records
    char m_pad238[0x298 - (0x220 + 0x18)];
    i32 m_gauge;       // +0x298  gauge current
    i32 m_gaugeTarget; // +0x29c  gauge target
    char m_pad2a0[0x2b0 - 0x2a0];
    i32 m_2b0;                     // +0x2b0  (multiplayer/battlez reset block)
    i32 m_2b4;                     // +0x2b4
    i32 m_2b8;                     // +0x2b8
    i32 m_2bc;                     // +0x2bc
    CSbiSlot m_groupSlots[3];      // +0x2c0  group-A 24-byte slot records
    CSbiSlotPtr* m_groupNotify[3]; // +0x308  group-A notify pointers
    char m_pad314[0x318 - 0x314];
    i32 m_hudRectB_x;          // +0x318  HUD-rect group B (x0)
    i32 m_hudRectB_y;          // +0x31c  (y0)
    i32 m_hudRectB_clock;      // +0x320  (latched dword from g_dat645588)
    i32 m_hudRectB_clockHi;    // +0x324
    i32 m_hudRectB_z;          // +0x328
    i32 m_hudRectB_zHi;        // +0x32c
    i32 m_hudRectA_x;          // +0x330  HUD-rect group A (x0)
    i32 m_hudRectA_y;          // +0x334  (y0)
    i32 m_hudRectA_clock;      // +0x338
    i32 m_hudRectA_clockHi;    // +0x33c
    i32 m_hudRectA_z;          // +0x340
    i32 m_hudRectA_zHi;        // +0x344
    CSbiMachineDisplay* m_348; // +0x348  rez-machine snooze display object
    i32 m_34c;                 // +0x34c
    i32 m_350;                 // +0x350
    i32 m_hitTestDisabled;     // +0x354  hit-test disable flag
    i32 m_tabsBuilt;           // +0x358  tab-widgets-built flag
    i32 m_activeSlot;          // +0x35c  active-slot index (-1 = none)
    i32 m_pendingHlRow;        // +0x360  pending highlight row index (-1 none)
    CSbiSlotPtr* m_notify0;    // +0x364  notify targets (slot 0x28)
    CSbiSlotPtr* m_notify1;    // +0x368
    CSbiSlotPtr* m_notify2;    // +0x36c
    CSbiSlotPtr* m_notify3;    // +0x370
    char m_pad374[0x378 - 0x374];
    CSbiHlRow m_hlGrid[12];      // +0x378  3 groups x 4 highlight rows (24B each)
    CSbiSlotPtr* m_hlNotify[12]; // +0x498  3 groups x 4 notify pointers
    i32 m_machinePhase;          // +0x4c8  (set to 1 by InitTabRects)
    i32 m_extraNotifyArg0;       // +0x4cc  arg for (*m_extraNotify0)->Notify
    i64 m_beltLast;              // +0x4d0  belt-drop timer last draw-clock (64-bit)
    i64 m_beltInterval;          // +0x4d8  belt-drop timer interval (64-bit)
    CSbiSlotPtr* m_extraNotify0; // +0x4e0
    char m_pad4e4[0x4e8 - 0x4e4];
    i32 m_fallActive;            // +0x4e8  falling-item active flag
    i32 m_extraNotifyArg1;       // +0x4ec  arg for (*m_extraNotify1)->Notify
    i32 m_fallLast;              // +0x4f0  falling-item timer last draw-clock = g_dat645588
    i32 m_fallLastHi;            // +0x4f4
    i32 m_fallDelay;             // +0x4f8  falling-item config delay
    i32 m_fallDelayHi;           // +0x4fc
    CSbiSlotPtr* m_extraNotify1; // +0x500
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
    CSbiPtrCollection m_ptrPool; // +0x530  pooled-ptr collection (RemoveAll on teardown)
    void** m_ptrTable;           // +0x534  pointer to the pooled-ptr table (elements streamed 8B)
    i32 m_ptrCount;              // +0x538  count for m_ptrTable
    char m_pad53c[0x548 - 0x53c];
    i32 m_hlBusy;                 // +0x548
    CWarpStoneFly* m_retabNotify; // +0x54c  a notifier object (freed on retab; Refresh()/Notify0())
    i32 m_toggleActive;           // +0x550  toggle-mode active flag
    i32 m_toggleHandle;           // +0x554  toggle-mode tab handle
    i32 m_destructWarnActive;     // +0x558
    i32 m_modeState;              // +0x55c
    i32 m_destructWarnLast;       // +0x560  destruct-warning last draw-clock
    i32 m_destructWarnLastHi;     // +0x564
    i32 m_destructWarnDelay;      // +0x568  destruct-warning delay (config)
    i32 m_destructWarnDelayHi;    // +0x56c
    CSbiSlotPtr* m_modeNotify;    // +0x570  notify target
    i32 m_modeArmed;              // +0x574
    i32 m_578;                    // +0x578  (cleared on multiplayer/battlez reset)
    i32 m_battlezPct[38];         // +0x57c  running-sum item-percent table (battlez cfg)
    i32 m_barFrameGate;           // +0x614  main-status-bar frame gate
    CSbiDisplayObj* m_destructButton; // +0x618  destruct-button display object
    i32 m_61c[4];                     // +0x61c  trailing dword block (cleared on reset)
    i32 m_tabCycle;                   // +0x62c  4-state highlight cursor (AdvanceTab cycles 0..3)
};

// The cue-lookup string map embedded at host->m_28 + 0x10 (CMapStringToOb).
// (The ex-`CMapStringToOb` view is DISSOLVED: an empty phantom aliasing the MFC library
// CMapStringToOb::Lookup @0x1b8438 - the member is the real map.)

// A resolved cue record: a player at +0x10 plus a draw-clock gate (+0x14 last,
// +0x18 interval). Same shape as GameMode's CBootyFound.
struct CSoundCueMgr; // defined below
struct CSbiCueRecord {
    char m_pad0[0x10];
    CSoundCueMgr* m_10; // +0x10  player (ConfigureItem this)
    i32 m_14;           // +0x14  last draw-clock
    i32 m_18;           // +0x18  interval
};
SIZE_UNKNOWN(CSbiCueRecord);

// The cue player (ConfigureItem == FUN_005360d0, __thiscall, ret 0x10).

// CONSOLIDATION NOTE (g_gameReg->m_world world/resource-mgr views): CSbiGameMgr is the
// canonical CResMgr (<Gruntz/ResMgr.h>) - g_gameReg->m_world, whose m_8 (CKeyTable, the
// Deserialize seq map @+0x48) and m_28 (the sound object) were verified as the same
// slots CResMgr models. CSbiMusicHost is the +0x28 sound object viewed as its CUE
// facet (the same shape as SBI_MenuItem's CMiMusicHost: +0x30 gate, cue map @+0x10),
// a SIBLING of CResMgr::m_28 (CSoundRegistry, the install facet). The fold to CResMgr
// is DEFERRED here (documented, not fabricated): this 4469-line, mostly-@early-stop TU
// reaches m_30 through its own CGameReg singleton, so pulling in <Gruntz/ResMgr.h>
// risks broad codegen shifts across its ~70 functions, and the m_28 cue access needs a
// multi-view cast at ~7 sites (the CSoundRegistry-vs-cue-host facet split is not
// settled by the delinked bytes). Kept as the per-TU view for the final sweep.

// The music host chain: g_gameReg->m_world->m_28->{m_30 gate, Lookup map @+0x10}
// (== CResMgr::m_28 viewed as its cue facet; see the consolidation note above).
struct CSbiMusicHost {
    char m_pad0[0x10];         // +0x00..0x0f
    CMapStringToOb m_map10;    // +0x10  cue lookup map (CMapStringToOb view)
    char m_pad11[0x30 - 0x11]; // +0x11..0x2f
    i32 m_30; // +0x30  reentrancy gate flag (opaque; only null-tested => skip the cue play)
};
SIZE_UNKNOWN(CSbiMusicHost);

// The active game manager (g_gameReg->m_world == CResMgr): carries the main
// status-bar draw chain at +0x04 and the sound object / music host at +0x28. See
// the consolidation note above (fold to CResMgr deferred).
struct CSbiMainL1; // +0x04 draw chain (defined below); the main-bar sprite path
struct CSbiGameMgr {
    char m_pad0[0x4];
    CSbiMainL1* m_4; // +0x04  main-status-bar draw chain (m_4->m_14->m_2c setup)
    char m_pad8[0x28 - 0x8];
    CSbiMusicHost* m_28; // +0x28  music host (CResMgr::m_28 cue facet)
};
SIZE_UNKNOWN(CSbiGameMgr);

// REMOVED (was `struct CSbiSubMgr`): the current play-state at g_gameReg->m_curState
// (+0x2c) is the real CPlay (Play.h). Its highlight methods are CPlay methods, reached
// via ((CPlay*)g_gameReg->m_curState) - a genuine CState->CPlay downcast (m_curState is
// canonically CState*): ScrollTo->ResetGoals (0xd5f00), Refresh->ResetViewport (0xd8c60),
// PostWarn->ArmSnapshot (0xd9240), HiToggle->EnterOverlayDrag (0xd6440), SetState (0xd5b20),
// HiRefresh (0xd6560), and the +0x4f0 highlight-busy gate (CPlay::m_4f0).

// A resolved tile-grid entry (m_grid[]): carries a sub-object at +0x10 whose
// +0x5c/+0x60 are the tile origin pair forwarded to ScrollTo.
struct CSbiTileSub {
    char m_pad0[0x5c];
    i32 m_5c; // +0x5c
    i32 m_60; // +0x60
};
SIZE_UNKNOWN(CSbiTileSub);
struct CSbiTileEntry {
    char m_pad0[0x10];
    CSbiTileSub* m_10; // +0x10
};
SIZE_UNKNOWN(CSbiTileEntry);

// REMOVED (was `struct CSbiActiveObj`): the single-player active object at
// g_gameReg->m_68 (+0x68) is the real CTriggerMgr (TriggerMgr.h), reached via
// ((CTriggerMgr*)g_gameReg->m_68) - a genuine per-mode-reused void* slot (grid/goo-
// well/light-fx in other modes). ProbeXY->ResetCell (0x46bfd0), ScrollProbe->
// RecordListHas (0x4784d0), LoadCameraSprite (0x478960); the placed-cursor latch trio
// is CTriggerMgr m_230/m_recX/m_recY, m_288 == m_288, m_400 == m_groupFlag. The grid
// element type is CTmCell (still an unmatched engine cell), viewed here as CSbiTileEntry.

// REMOVED (was `struct CSbiLogger`): the settings/registry writer at
// g_gameReg->m_settings (+0x38) is the real Utils::RegistryHelper (RegistryHelper.h),
// reached via ((Utils::RegistryHelper*)g_gameReg->m_settings). LogPos->SetValueDword,
// QueryPos->GetValueDword (0x1395d0). (m_settings stays void* in the canonical header -
// its MFC-side dual-view types it CSettingsWriter*, owned by a parallel worker.)

// The window host at g_gameReg->m_gameWnd (+0x4): carries the game HWND at +0x4 (the
// PostMessage target). Reached as ((CSbiWndHost*)g_gameReg->m_gameWnd) - the +0x4
// sub-object of the canonical CGameRegistry, not a facet of the singleton itself.
struct CSbiWndHost {
    char m_pad0[0x4];
    void* m_4; // +0x4  game window handle
};
SIZE_UNKNOWN(CSbiWndHost);

// CONSOLIDATED (was the per-TU `struct CGameReg`, an offset-0 SAME-MEMORY ALIAS of the
// *0x24556c singleton): dissolved into the canonical CGameRegistry (<Gruntz/GameRegistry.h>).
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
// resource mgr / CResMgr; its fold is DEFERRED - a deep CResMgr sub-object cascade, see
// the CSbiMusicHost/CSbiMainL1 note above and GameRegistry.h's m_world typing).
// The three TU-specific methods re-homed to CGameRegistry as the real CGruntzMgr methods
// they are: Fn29aa->UpdateScoreHud (0x860b0), HiPump->AccrueScoreTime (0x861e0),
// SetToggle->FinishLevel (0x8e980); ReportError(i32,i32) added as the i32,i32 overload.
// The dead inline ViewSize() (unused - 0xfe520 reads m_modeW/m_modeH directly) is dropped.

// The seq-keyed object map at (g_gameReg->m_world->m_8 + 0x48): Lookup(key, &out)
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
    CSbiLayer* m_198; // +0x198  layer descriptor
};
SIZE_UNKNOWN(CSbiRenderObj);

// The +0x530 pooled-ptr collection InsertPtr appends/inserts into (CObArray-style).
struct CSbiPtrColl2 {
    void Append(i32 idx, void* node);            // 0x1b5144 (InsertAt tail)
    void InsertAt(i32 idx, void* node, i32 cnt); // 0x1b516b (InsertAt with count)
    void RemoveAt(i32 idx, i32 cnt);             // 0x1b5200 (RemoveAt with count)
};
SIZE_UNKNOWN(CSbiPtrColl2);

// A free-list node {m_0, m_4}; m_0 doubles as the link, m_4 is the sort key.
struct CSbiFreeNode {
    i32 m_0, m_4;
};
SIZE_UNKNOWN(CSbiFreeNode);


// (the CSbiTab base is GONE - it was never a type. It was a fabricated stand-in for the
// canonical CStatusBarItem (already included above), and it declared THIRTEEN virtuals
// against a real base of ELEVEN, so cl emitted a 13-slot vtable for both leaves below
// where retail has 11 and 12. That is not a cosmetic defect - every dispatch past the
// divergence point would have gone to the wrong slot - and its 13 declared-only virtuals
// were 13 guaranteed unresolved externals on top.
//
// The remap, read off the real vtables (sema class) rather than guessed:
//   retail CStatusBarItem  vtbl@0x1eabcc  11 slots (the base; all bodies real)
//   retail CSBI_RectOnly   vtbl@0x1eab8c  11 slots  : CStatusBarItem   <- CSbiRectSub
//   retail CSBI_Image      vtbl@0x1eac0c  12 slots  : CSBI_RectOnly  (adds slot 11)
//   retail CSBI_MenuItem   vtbl@0x1eab4c  12 slots  : CSBI_Image      <- CSBI_MenuItem
// So the leaves derive DIRECTLY from CStatusBarItem here (the CSBI_RectOnly/CSBI_Image
// intermediates add no fields, so this is layout-identical), and the ONE extra slot the
// menu item really has - slot 11, CSBI_Image::SetupImage, which this class overrides -
// is declared on the menu item itself with the canonical signature. The old view's
// "Configure" WAS that slot-11 call, and its "Activate" @ slot 12 was pure fabrication:
// retail's deepest vtable here has 12 slots (0..11), and nothing ever called it.
//
// The CSBI_RectOnly/CSBI_Image intermediates cannot be NAMED here: <Gruntz/SBI_Image.h>
// defines a different class under the CSBI_RectOnly name than this header's 0x570 HOST
// does, so the two cannot be co-included. Freeing that name is the documented cross-lane
// host rename (see the note above); it is not needed to get the slot shape right.

// tag-1 rect-only sub-widget. Its TRUE retail class is CSBI_RectOnly (vtable 0x5eab8c),
// but that name is bound in this TU to the big status-bar HOST, so the sub-widget carries
// this placeholder name; MSVC still auto-stamps a real vtable (reloc-masked).
//
// SIZE IS 0x3c, NOT 0x30 - binary-proven at the allocation site rather than inferred from
// the (empty) field list. Retail's CStatusBarMgr::LoadTabSprites does, at 0x10237d:
//     push 0x3c            ; sizeof
//     call 0x1b9b46        ; operator new
//     mov  ecx,eax
//     call 0x1e88          ; -> 0x101fa0 ??0CSBI_RectOnly (stamps ??_7CSBI_RectOnly)
// so CSBI_RectOnly = CStatusBarItem (0x30) + three words. The ctor leaves them
// uninitialised (Setup fills them), which is why the old 0x30 guess looked self-
// consistent: an incomplete ctor does NOT bound an object.
// SIZE IS 0x30, NOT 0x3c - and the ALLOCATION SITE IN THIS TU says so. The three rect
// sub-widgets BuildStatusBarTabs creates are allocated `push 0x30; call ??2@YAPAXI@Z`
// (@0xffe46 / 0xffed5 / 0xfff4b), while the five menu items in the SAME function are
// `push 0x3c` (@0xfffd9 / 0x100071 / 0x100105 / 0x100199 / 0x100275). So the +0x30/+0x34/
// +0x38 trio belongs to the MENU ITEM ONLY - exactly as the old base's own comment said
// ("fields end at +0x30 (the rect-widget size); the menu-item's m_30/m_34/m_38 live in
// CSBI_MenuItem") before it handed them to the rect widget anyway, on the strength of a
// DIFFERENT allocation site (0x10237d, in CStatusBarMgr::LoadTabSprites - which allocates
// a different, larger object). We were over-allocating these by 12 bytes.
class CSbiRectSub : public CStatusBarItem { // TRUE class CSBI_RectOnly, vtable 0x5eab8c
public:
    CSbiRectSub() {
        m_8 = 1;
    }
};
SIZE(CSbiRectSub, 0x30);

// tag-2 menu item (0x3c). vtable 0x5eab4c -> auto-named ??_7CSBI_MenuItem@@6B@.
// The dtor is declared OUT-OF-LINE (no body): an implicit one makes cl5 emit a
// divergent 5-byte COMDAT `??1CSBI_MenuItem@@UAE@XZ = jmp ??1CSbiTab@@UAE@XZ`,
// duplicating the real dtor (SBI_MenuItem.cpp 0x1007d0) and calling a base dtor no
// obj defines (CSbiTab is a view) -> unresolved external at link. Declared-only =>
// the reference binds to the one real body at its retail rva.
class CSBI_MenuItem : public CStatusBarItem {
public:
    CSBI_MenuItem() {
        m_8 = 2;
        m_34 = 0;
        m_30 = 0;
        m_38 = 0;
    }
    // Retail vtable shape (sema class: vtbl@0x1eab4c, TWELVE slots; overrides 0/1/3/4/5
    // and slot 11) - mirrored EXACTLY from the canonical CSBI_MenuItem in
    // <Gruntz/SBI_MenuItem.h>, so every entry this TU emits binds to the same symbol the
    // canonical does. Declared-only => the references bind to the one real body per rva.
    virtual ~CSBI_MenuItem();         // slot 0  0x1007d0 (SBI_MenuItem.cpp)
    virtual i32 SbiVfunc0() OVERRIDE; // slot 1
    virtual void SbiSlot3() OVERRIDE; // slot 3
    virtual void SbiSlot4() OVERRIDE; // slot 4
    virtual void SbiSlot5() OVERRIDE; // slot 5
    // slot 11 - the 11-arg image setup CSBI_Image introduces and this class overrides.
    // It is what the old view called "Configure": the tab-configure call site pushes
    // exactly these 11 dwords. Declared with the canonical signature so it mangles to the
    // same ?SetupImage@CSBI_MenuItem@@ symbol (out-of-line body: InitItem @0xe80e0).
    virtual i32 SetupImage(
        i32 a1,
        CSbiConfigHost* host,
        i32 a3,
        i32 a4,
        i32 a5,
        i32 a6,
        i32 a7,
        i32 a8,
        i32 key,
        i32 a10,
        i32 a11
    ); // slot 11
    // The tab-widget drivers CSBI_RectOnly reaches through m_tabSprite* (non-virtual
    // reloc-masked call rel32; bodies + rvas bound in SBI_MenuItem.cpp). These fold
    // the former fake CSbiSprite view onto the real class: Release->Blit, Show->
    // SetState, Hide->ProbeState, Configure->ResolveFrame (proven by shared rva).
    i32 ResolveFrame(i32 key, i32 a); // 0xe81e0  (was CSbiSprite::Configure)
    i32 SetState(i32 state, i32 a);   // 0xe8310  (was CSbiSprite::Show)
    i32 ProbeState(i32 state);        // 0xe8480  (was CSbiSprite::Hide)
    i32 Blit();                       // 0xe84f0  (was CSbiSprite::Release)
    i32 m_30;                         // +0x30
    i32 m_34;                         // +0x34
    i32 m_38;                         // +0x38
};
SIZE(CSBI_MenuItem, 0x3c);

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
struct CSbiMainSetup {};
SIZE_UNKNOWN(CSbiMainSetup);
struct CSbiMainL2 {
    char m_pad0[0x2c];
    CSbiMainSetup* m_2c; // +0x2c
};
SIZE_UNKNOWN(CSbiMainL2);
struct CSbiMainL1 {
    char m_pad0[0x14];
    CSbiMainL2* m_14; // +0x14
};
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
void __stdcall MainBarDrawFrame(CSbiMainL2* obj, i32 x, i32 y, i32 flag); // 0x153790

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

// --- vtable catalog (view/base classes bound to their unit vtable rva) ---

#endif // GRUNTZ_SBI_RECTONLY_H
