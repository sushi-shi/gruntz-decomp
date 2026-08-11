#ifndef GRUNTZ_CSTATUSBARMGR_H
#define GRUNTZ_CSTATUSBARMGR_H

#include <rva.h>

#include <Mfc.h>

#include <Bute/ButeMgr.h>
#include <Enums.h>
#include <Gruntz/DestructWarningState.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameTabContent.h>
#include <Gruntz/GruntzCommandId.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SbGeom.h>
#include <Gruntz/SbiBeltPhase.h>
#include <Gruntz/SbiConfig.h>
#include <Gruntz/SbiFallingItemState.h>
#include <Gruntz/SbiHlRowState.h>
#include <Gruntz/SbiMachineState.h>
#include <Gruntz/SbiMenuItemState.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/StatusBarDock.h>
#include <Gruntz/StatusBarHighlightRow.h>
#include <Gruntz/StatusBarItem.h>
#include <Gruntz/StatusBarTab.h>
#include <Gruntz/StatusSampleMode.h>
#include <Gruntz/WarpStoneFragment.h>
#include <Ints.h>

class CSBI_ImageSet;
class CSBI_WellGoo;
class CWarpStoneFly;
class CSBI_MenuItem;
class CSBI_GruntMachine;
class DirectSoundMgr;
GZ_ENUM_BEGIN(SbiSlotState)
    SLOT_ARMED = 0,
    // Entered when the slot is activated; the progress animation runs from
    // here until its frame reaches kSlotCommitLevel, which sets SLOT_READY.
    SLOT_FILLING = 1,
    SLOT_READY = 2
GZ_ENUM_END(SbiSlotState)

struct CSbiSlot {

    CSbiSlot() {
        m_startTime = 0;
        m_interval = 0;
    }
    SbiSlotState m_state;
    i32 m_value;
    union {
        i64 m_startTime;
        struct {
            i32 m_startTimeLo;
            i32 m_startTimeHi;
        };
    };
    i64 m_interval;
};
SIZE(0x18);

// A 64-bit timestamp paired with its 64-bit interval - the unit `SyncClockPair`
// serializes and the shape CSbiHlRow carries in its own tail. Its constructor writes
// lastLo, intervalLo, lastHi, intervalHi, which is retail's +0/+8/+4/+0xc store order.
struct SbiClockPair {
    SbiClockPair() {
        m_lastLo = 0;
        m_intervalLo = 0;
        m_lastHi = 0;
        m_intervalHi = 0;
    }

    union {
        i64 m_last;
        struct {
            i32 m_lastLo, m_lastHi;
        };
    };
    union {
        i64 m_interval;
        struct {
            i32 m_intervalLo, m_intervalHi;
        };
    };
};
SIZE(0x10);

struct CSbiHlRow {
    // Inline: retail's out-of-line copy has NO rel32 caller at all - it exists only
    // because cl 5.0 hands its ADDRESS to `??_H` (`vector constructor iterator`) for
    // CStatusBarMgr's m_groupSlots[3] and m_hlGrid[12].  That address-take is what
    // emits the COMDAT, while every scalar/small-array site expands the body in
    // place.  See docs/patterns/inline-ctor-comdat-via-vector-ctor-iterator.md.
    RVA(0x000c86d0, 0x11)
    CSbiHlRow() {
        m_lastLo = 0;
        m_intervalLo = 0;
        m_lastHi = 0;
        m_intervalHi = 0;
    }

    i32 m_state;

    union {
        i32 m_value;
        i32 m_counter;
    };
    union {
        i64 m_last;
        struct {
            i32 m_lastLo, m_lastHi;
        };
    };
    union {
        i64 m_interval;
        struct {
            i32 m_intervalLo, m_intervalHi;
        };
    };
};
SIZE(0x18);

class CSBI_SideTab;
class CSBI_StatzTabArrow;
class CSBI_WarlordHead;
class CWarpStoneFly;

const i32 kSlotCommitLevel = 0x1a;

const i32 kActivateErrId = 0x80e4;
const i32 kActivateErrTag = 0x44b;

const i32 kSetTabErrTag = 0x44a;

GZ_ENUM_CONST_BEGIN(SbiGaugePct)
    SBI_GAUGE_EMPTY = 0,
    SBI_GAUGE_FULL = 100
GZ_ENUM_CONST_END(SbiGaugePct)

class CStatusBarMgr {
public:
    CStatusBarMgr();
    i32 BuildSideTabs();

    ~CStatusBarMgr() {
        Teardown();
    }

    i32 LoadTabSprites();
    i32 BuildGameMenu();

    void UpdateDestructButton(i32 arg);
    i32 EnsureSub(i32 a, i32 b, WarpStoneFragment fragment);
    void ResetCounters();
    void ResetSlots();
    void ArmSlot(i32 idx);
    i32 AnySlotActive();
    void AdvanceGauge(i32 delta);
    void DrainGauge(i32 delta);
    void SetGaugeTarget(i32 value);
    void SetGauge(i32 value);
    void RefreshAll();
    void Reset();
    void ToggleStat(i32 idx);
    void SetHudRectA(i32 y0, SbiMachineState x0, i32 z);
    void SetHudRectB(i32 y0, SbiMachineState x0, i32 z);
    void CommitSlot(i32 active);
    void ClearHlCell(i32 group, StatusBarHighlightRow row);
    i32 SetHlCell(i32 row, i32 handle, i32 group);
    i32 SetHlCellByTier(i32 handle, i32 group);
    i32 FindReadySlot();
    void SetMode(i32 mode);

    i32 BuildStatusBarTabs();

    i32 BuildTabzDialog();
    i32 StartChipMachineCycle();
    i32 LoadBattlezItemConfig(CDDrawSurfaceMgr* world);
    i32 LoadMainStatusBarSprite();
    i32 UpdateStatusBarTabHighlight(i32, i32, i32);
    i32 LoadDestructButtonSprite(i32);
    void BuildGameTabResumeButton(i32);
    void BuildGameTabPauseButton();

    i32 LoadGooCookingSprite(i32);
    void UpdateRezConveyorStatusBar();
    void LoadRezMachineConfig();
    void UpdateRezMachineSnoozeStatusBar();
    void LoadChipMachineConfig();
    i32 UpdateFallingItemStatusBar(i32, i32, i32);
    i32 UpdateRezMachineWakeStatusBar();
    void LoadMultiplayerBattlezConfig(i32);

    void ResetGroupA();

    i32 Sync(CFileMemBase* s, SerialMode op, LogicTypeId typeId, i32 pObj);

    i32 GetActiveValue();
    i32 LoadStatzTabToggleSprite(i32 idx, StatusSampleMode value);
    void UpdateGruntOvenStatusBar();
    void TickGauge();
    void UpdateChipGrinderStatusBar();
    void NotifyAllSlots();
    void UpdateDestructButtonStatusBar();
    i32 Activate();
    i32 SetTabState(SbiCommandId cmd, SbiMenuItemState state);

    void Teardown();
    i32 TryActivate();
    i32 Deactivate();
    i32 HlClickGroup0(StatusBarHighlightRow row);
    i32 HlClickGroup1(StatusBarHighlightRow row);
    i32 HlClickGroup2(StatusBarHighlightRow row);
    i32 SetTab(GameTabContent tab, i32 flag);
    i32 ClearTabSprites(StatusBarTab idx);
    i32 HitTest(i32 x, i32 y);
    i32 Serialize(CFileMemBase* s);
    i32 Deserialize(CFileMemBase* s);

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

    i32 OnPointerRelease(i32 button, i32 x, i32 y);
    i32 ClickToggle(i32 btn, i32 x, i32 y);
    CStatusBarItem* HitTestRects(i32 x, i32 y);
    void ResetWidgets(i32 keepLists);
    void ClearTabGroup();
    i32 ClearStat(i32 idx);
    void EnterHlRow(i32 row, i32 group);
    void InitTabRects();
    i32 SetFallRect(i32 a, i32 b, i32 c);
    void ExitMode();
    i32 ActivateSlot(i32 idx);
    i32 PlaceCursorTarget(i32 row, i32 commit);

    i32 SetState(StatusBarDock state);
    i32 RefreshState();
    i32 SetSpritePos(i32 x, i32 y);
    i32 HitTestLayer(i32 x, i32 y);
    i32 InsertPtr(i32 a, i32 b);
    void ReportTab(i32 tab);

    i32 RefreshA();
    i32 HideRect();

    void AdvanceTab(i32 reverse);

    i32 DockStatusBarRight();

    StatusBarDock m_position;
    StatusBarDock m_restorePosition;

    class CWwdGameObjectA* m_barSprite;

    CDDrawSurfaceMgr* m_world;

    RECT m_rect10;
    i32 m_redrawFrames;
    i32 m_barX;
    i32 m_barY;

    CPtrList m_tabLists[8];
    StatusBarTab m_activeTab;
    GameTabContent m_itemKind;
    StatusSampleMode m_statFlags[15];
    CSBI_SideTab* m_hitRects[15];

    CSBI_StatzTabArrow* m_statObj[15];
    CSBI_MenuItem* m_tabSprite0;
    CSBI_MenuItem* m_tabSprite1;
    CSBI_MenuItem* m_tabSprite2;
    CSBI_MenuItem* m_tabSprite3;
    CSBI_MenuItem* m_tabSprite4;
    CSBI_MenuItem* m_tabSprite5;
    CSBI_MenuItem* m_tabSprite6;
    CSBI_MenuItem* m_tabSprite7;
    CSBI_MenuItem* m_tabSprite8;
    CSBI_MenuItem* m_tabSprite9;
    CSBI_MenuItem* m_tabSprite10;
    CSBI_MenuItem* m_tabSprite11;
    CSBI_MenuItem* m_tabSprite12;
    CSBI_MenuItem* m_tabSprite13;
    CSBI_MenuItem* m_tabSprite14;

    CSBI_ImageSet* m_slotNotify[5];
    CStatusBarItem* m_gaugeNotify;
    CSBI_WellGoo* m_gaugeSink;

    CSbiSlot m_slots[5];

    i32 m_gauge;
    i32 m_gaugeTarget;

    SbiClockPair m_reserved2a0;
    SbiClockPair m_reserved2b0;

    CSbiHlRow m_groupSlots[3];
    CSBI_ImageSet* m_groupNotify[3];
    char m_pad314[0x318 - 0x314];

    CSbiHlRow m_machineB;
    CSbiHlRow m_machineA;
    CSBI_GruntMachine* m_machineDisplay;
    i32 m_reserved34c;
    i32 m_reserved350;
    i32 m_hitTestDisabled;
    i32 m_tabsBuilt;
    i32 m_activeSlot;
    StatusBarHighlightRow m_pendingHlRow;
    CStatusBarItem* m_notify0;
    CStatusBarItem* m_notify1;
    CStatusBarItem* m_notify2;
    CStatusBarItem* m_notify3;
    char m_pad374[0x378 - 0x374];
    CSbiHlRow m_hlGrid[12];
    CSBI_ImageSet* m_hlNotify[12];
    SbiBeltPhase m_machinePhase;
    i32 m_extraNotifyArg0;
    i64 m_beltLast;
    i64 m_beltInterval;
    CSBI_ImageSet* m_extraNotify0;
    char m_pad4e4[0x4e8 - 0x4e4];
    SbiFallingItemState m_fallActive;
    i32 m_extraNotifyArg1;
    i64 m_fallLast;
    i64 m_fallDelay;
    CSBI_ImageSet* m_extraNotify1;
    RECT m_fallRect;
    RECT m_itemRect;
    i32 m_itemBaseX;
    i32 m_rezActive;
    i32 m_rezTick;

    CPtrArray m_ptrPool;
    i32 m_reserved544;

    i32 m_hlBusy;
    CWarpStoneFly* m_retabNotify;
    i32 m_toggleActive;
    i32 m_toggleHandle;
    DestructWarningState m_destructWarnActive;
    DestructButtonFrame m_modeState;
    i64 m_destructWarnLast;
    i64 m_destructWarnDelay;
    CSBI_ImageSet* m_modeNotify;
    i32 m_modeArmed;
    i32 m_observerTabAvailable;
    i32 m_battlezPct[38];
    i32 m_barFrameGate;
    DirectSoundMgr* m_destructButton;

    CSBI_WarlordHead* m_warlordHead[4];
    i32 m_tabCycle;
};
SIZE(0x630);

SIZE_UNKNOWN();

// CONTESTED, unresolved against the CURRENT layout: an independent lane read retail
// as emitting each zero-store pair at its DECLARATION position in the member-ctor
// sequence - ahead of `??_H(m_groupSlots,0x18,3)` (0xc8046), ahead of
// `??0CPtrArray(m_ptrPool)` (0xc80cb), and straight after it (0xc8106) - which would
// make these mem-initializers rather than body statements. That reading was written
// against the OLD four-i64 spelling of the 0x2a0/0x2b0 band and no longer compiles
// now that the band is two 16-byte clock objects, so it has never been A/B'd against
// this layout. Re-test it here before trusting either form; do not cite the address
// evidence as settled.
inline CStatusBarMgr::CStatusBarMgr() {
    // m_reserved2a0/2b0 and m_machineA/B zero themselves in the mem-init run, which is
    // where retail's stores are - between the m_slots[5] and m_groupSlots[3] ctor loops
    // and between m_groupNotify and m_machineDisplay. Writing them again here only
    // produced the same stores in the wrong PLACE and the wrong ORDER.
    //
    // m_beltLast/m_fallLast/m_destructWarnLast are the SAME 16-byte clock pair, and
    // retail zeroes them in the same +0/+8/+4/+0xc order - but typing all three as
    // SbiClockPair too spends CStatusBarMgr's /Ob1 inline budget: cl then CALLS
    // ??0CSbiHlRow for m_hlGrid[12] and ??0SbiClockPair for the pair at 0x560 where
    // retail expands both (measured: multi 98.08 -> 97.69, play 112 -> 106 exact).
    m_beltLast = 0;
    m_beltInterval = 0;
    m_fallLast = 0;
    m_fallDelay = 0;
    m_destructWarnLast = 0;
    m_destructWarnDelay = 0;

    m_tabSprite0 = NULL;
    m_tabSprite1 = NULL;
    m_tabSprite2 = NULL;
    m_tabSprite3 = NULL;
    m_tabSprite4 = NULL;
    m_tabSprite5 = NULL;
    m_tabSprite6 = NULL;
    m_tabSprite7 = NULL;
    m_tabSprite8 = NULL;
    m_tabSprite9 = NULL;
    m_tabSprite10 = NULL;
    m_destructButton = NULL;
    m_tabSprite11 = NULL;
    m_tabSprite12 = NULL;
    m_tabSprite13 = NULL;
    m_tabSprite14 = NULL;
    m_barSprite = NULL;
    m_world = NULL;
    m_redrawFrames = 0;
    m_activeTab = TAB_NONE;
    m_hitTestDisabled = 0;
    m_tabsBuilt = 0;
    m_toggleActive = 0;
    m_toggleHandle = 0;
    m_barFrameGate = 0x1e0;
    m_tabCycle = 0;
    // Seven memsets in ASCENDING member order, which is what retail emits:
    // 0x114 / 0x150 / 0x18c as `rep stosd`, then 0x204 (5), 0x308 (3), 0x498 (12)
    // and 0x61c (4).  A short memset unrolls to stores through ONE `lea` base
    // (0xc81b8 `lea eax,[esi+0x204]`, 0xc8203 `lea edx,[esi+0x308]`, 0xc8220
    // `lea ecx,[esi+0x61c]`); per-element `= NULL` assignments emit a disp32
    // store each instead, and land in source order rather than ascending.
    memset(m_statFlags, 0, sizeof(m_statFlags));
    memset(m_hitRects, 0, sizeof(m_hitRects));
    memset(m_statObj, 0, sizeof(m_statObj));
    memset(m_slotNotify, 0, sizeof(m_slotNotify));
    memset(m_groupNotify, 0, sizeof(m_groupNotify));
    memset(m_hlNotify, 0, sizeof(m_hlNotify));
    memset(m_warlordHead, 0, sizeof(m_warlordHead));
    m_notify0 = NULL;
    m_notify2 = NULL;
    m_notify3 = NULL;
    m_notify1 = NULL;
    m_extraNotify0 = NULL;
    m_extraNotify1 = NULL;
    m_machineDisplay = NULL;
    m_modeNotify = NULL;
    m_gaugeNotify = NULL;
    m_gaugeSink = NULL;
    m_gaugeTarget = SBI_GAUGE_EMPTY;
    m_gauge = SBI_GAUGE_EMPTY;
    m_reserved544 = 1;
    m_hlBusy = 0;
    m_retabNotify = NULL;
    m_modeArmed = 0;
}

#endif // GRUNTZ_SBI_RECTONLY_H
