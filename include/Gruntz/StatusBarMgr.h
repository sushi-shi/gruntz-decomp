#include <rva.h>

#include <Mfc.h>

#include <Bute/ButeMgr.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/SbGeom.h>
#include <Gruntz/SbiConfig.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/StatusBarItem.h>
#include <Ints.h>

#ifndef GRUNTZ_CSTATUSBARMGR_H
#define GRUNTZ_CSTATUSBARMGR_H

class CSBI_ImageSet;
class CSBI_WellGoo;
class CWarpStoneFly;
class CSBI_MenuItem;
class CSBI_GruntMachine;
class DirectSoundMgr;

struct CSbiSlot {

    CSbiSlot() {
        m_startTimeLo = 0;
        m_interval = 0;
        m_startTimeHi = 0;
    }
    i32 m_state;
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

struct CSbiHlRow {
    CSbiHlRow();
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

extern CButeMgr g_buteMgr;

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
    CStatusBarMgr();
    i32 BuildSideTabs();

    ~CStatusBarMgr() {
        Teardown();
    }

    i32 LoadTabSprites();
    void BuildGameMenu();

    void UpdateDestructButton(i32 arg);
    i32 EnsureSub(i32 a, i32 b, i32 c);
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

    i32 Sync(CFileMemBase* s, i32 op, i32 typeId, i32 pObj);

    i32 GetActiveValue();
    i32 LoadStatzTabToggleSprite(i32 value, i32 idx);
    void UpdateGruntOvenStatusBar();
    void TickGauge();
    void UpdateChipGrinderStatusBar();
    void NotifyAllSlots();
    void UpdateDestructButtonStatusBar();
    i32 Activate();
    i32 SetTabState(i32 tab, i32 state);

    void Teardown();
    i32 TryActivate();
    i32 Deactivate();
    i32 HlClickGroup0(i32 row);
    i32 HlClickGroup1(i32 row);
    i32 HlClickGroup2(i32 row);
    i32 SetTab(i32 tab, i32 flag);
    i32 ClearTabSprites(i32 idx);
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

    i32 SetState(i32 state);
    i32 RefreshState();
    i32 SetSpritePos(i32 x, i32 y);
    i32 HitTestLayer(i32 x, i32 y);
    i32 InsertPtr(i32 a, i32 b);
    void ReportTab(i32 tab);

    i32 RefreshA();
    i32 HideRect();

    void AdvanceTab(i32 reverse);

    i32 DockStatusBarRight();

    i32 m_position;
    i32 m_restorePosition;

    class CWwdGameObjectA* m_barSprite;

    CDDrawSurfaceMgr* m_world;

    RECT m_rect10;
    i32 m_redrawFrames;
    i32 m_barX;
    i32 m_barY;

    CPtrList m_tabLists[8];
    i32 m_activeTab;
    i32 m_itemKind;
    i32 m_statFlags[15];
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

    i64 m_reserved2a0;
    i64 m_reserved2a8;
    i32 m_reserved2b0;
    i32 m_reserved2b4;
    i32 m_reserved2b8;
    i32 m_reserved2bc;

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
    i32 m_pendingHlRow;
    CStatusBarItem* m_notify0;
    CStatusBarItem* m_notify1;
    CStatusBarItem* m_notify2;
    CStatusBarItem* m_notify3;
    char m_pad374[0x378 - 0x374];
    CSbiHlRow m_hlGrid[12];
    CSBI_ImageSet* m_hlNotify[12];
    i32 m_machinePhase;
    i32 m_extraNotifyArg0;
    i64 m_beltLast;
    i64 m_beltInterval;
    CSBI_ImageSet* m_extraNotify0;
    char m_pad4e4[0x4e8 - 0x4e4];
    i32 m_fallActive;
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
    i32 m_destructWarnActive;
    i32 m_modeState;
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

inline CStatusBarMgr::CStatusBarMgr() {

    m_reserved2a0 = 0;
    m_reserved2a8 = 0;
    m_reserved2b0 = 0;
    m_reserved2b4 = 0;
    m_reserved2b8 = 0;
    m_reserved2bc = 0;
    m_machineB.m_lastLo = 0;
    m_machineB.m_lastHi = 0;
    m_machineB.m_intervalLo = 0;
    m_machineB.m_intervalHi = 0;
    m_machineA.m_lastLo = 0;
    m_machineA.m_lastHi = 0;
    m_machineA.m_intervalLo = 0;
    m_machineA.m_intervalHi = 0;
    m_beltLast = 0;
    m_beltInterval = 0;
    m_fallLast = 0;
    m_fallDelay = 0;
    m_destructWarnLast = 0;
    m_destructWarnDelay = 0;

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
    m_destructButton = 0;
    m_tabSprite11 = 0;
    m_tabSprite12 = 0;
    m_tabSprite13 = 0;
    m_tabSprite14 = 0;
    m_barSprite = 0;
    m_world = 0;
    m_redrawFrames = 0;
    m_activeTab = 0;
    m_hitTestDisabled = 0;
    m_tabsBuilt = 0;
    m_toggleActive = 0;
    m_toggleHandle = 0;
    m_barFrameGate = 0x1e0;
    m_tabCycle = 0;
    memset(m_statFlags, 0, sizeof(m_statFlags));
    memset(m_hitRects, 0, sizeof(m_hitRects));
    memset(m_statObj, 0, sizeof(m_statObj));
    memset(m_slotNotify, 0, sizeof(m_slotNotify));
    memset(m_hlNotify, 0, sizeof(m_hlNotify));
    m_groupNotify[0] = 0;
    m_groupNotify[1] = 0;
    m_groupNotify[2] = 0;
    m_warlordHead[0] = 0;
    m_warlordHead[1] = 0;
    m_warlordHead[2] = 0;
    m_warlordHead[3] = 0;
    m_notify0 = 0;
    m_notify2 = 0;
    m_notify3 = 0;
    m_notify1 = 0;
    m_extraNotify0 = 0;
    m_extraNotify1 = 0;
    m_machineDisplay = 0;
    m_modeNotify = 0;
    m_gaugeNotify = 0;
    m_gaugeSink = 0;
    m_gaugeTarget = 0;
    m_gauge = 0;
    m_reserved544 = 1;
    m_hlBusy = 0;
    m_retabNotify = 0;
    m_modeArmed = 0;
}

#endif // GRUNTZ_SBI_RECTONLY_H
