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
class SoundBuffer;
GZ_ENUM_BEGIN(SbiSlotState)
    SLOT_ARMED = 0,
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

struct SbiClockPair {
    SbiClockPair() : m_last(0), m_interval(0) {}

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

struct CSbiHlRow {
    RVA(0x000c8700, 0x11)
    CSbiHlRow() {
        m_state = IDX(HLROW_OFF);
        m_value = 0;
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

class CSBI_SideTab;
class CSBI_StatzTabArrow;
class CSBI_WarlordHead;
class CWarpStoneFly;

const i32 kSlotCommitLevel = 0x1a;

const i32 kActivateErrId = 0x80e4;
const i32 kActivateErrTag = 0x44b;

const i32 kSetTabErrTag = 0x44a;

GZ_ENUM_CONST_BEGIN(GruntWellPct)
    GRUNT_WELL_EMPTY = 0,
    GRUNT_WELL_FULL = 100
GZ_ENUM_CONST_END(GruntWellPct)

GZ_ENUM_CONST_BEGIN(StatusBarGruntSlots)
    STATUSBAR_GRUNT_SLOT_COUNT = 15
GZ_ENUM_CONST_END(StatusBarGruntSlots)

class CStatusBarMgr {
public:
    CStatusBarMgr();
    i32 BuildSideTabs();

    RVA(0x000c89b0, 0x64)
    ~CStatusBarMgr() {
        Teardown();
    }

    i32 LoadTabSprites();
    i32 BuildGameMenu();

    void StartDestructWarning(i32 countdownMs);
    i32 StartWarpStoneFly(i32 srcX, i32 srcY, WarpStoneFragment fragment);
    void ResetCounters();
    void ResetSlots();
    void ArmSlot(i32 idx);
    i32 AnySlotActive();
    void AdvanceGruntWell(i32 delta);
    void DrainGruntWell(i32 delta);
    void SetGruntWellTarget(i32 value);
    void SetGruntWell(i32 value);
    void UpdateStatusSystems();
    void Reset();
    void ToggleStat(i32 idx);
    void SetLeftRezMachineAnimation(i32 initialFrame, SbiMachineState state, i32 frameDelayMs);
    void SetRightRezMachineAnimation(i32 initialFrame, SbiMachineState state, i32 frameDelayMs);
    void CommitSlot(b32 active);
    void ClearHlCell(i32 group, StatusBarHighlightRow row);
    i32 SetHlCell(i32 row, i32 handle, i32 group);
    i32 SetHlCellByTier(i32 handle, i32 group);
    i32 FindReadySlot();
    void LockDestructButton(i32 resetWarningAnimation);

    i32 BuildStatusBarTabs();

    i32 BuildTabzDialog();
    i32 StartChipMachineCycle();
    i32 LoadBattlezItemConfig(CDDrawSurfaceMgr* world);
    i32 LoadMainStatusBarSprite();
    i32 UpdateStatusBarTabHighlight(i32 mouseFlags, i32 x, i32 y);
    i32 UpdateStatusBar(i32 deltaMs);
    void BuildGameTabResumeButton(b32 show);
    void BuildGameTabPauseButton();

    i32 LoadGooCookingSprite(i32);
    void UpdateRezConveyorStatusBar();
    void LoadRezMachineConfig();
    void UpdateRezMachineSnoozeStatusBar();
    void LoadChipMachineConfig();
    i32 UpdateFallingItemStatusBar(i32 item, i32 x, i32 y);
    i32 UpdateRezMachineWakeStatusBar();
    void LoadMultiplayerBattlezConfig(i32);

    void ResetConveyorBelts();

    i32 SerializeDispatch(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, i32 payload);

    i32 GetActiveValue();
    i32 LoadStatzTabToggleSprite(i32 idx, StatusSampleMode value);
    void UpdateGruntOvenStatusBar();
    void TickGruntWell();
    void UpdateChipGrinderStatusBar();
    void NotifyAllSlots();
    void UpdateDestructWarningAnimation();
    i32 Activate();
    i32 SetTabState(SbiCommandId cmd, SbiMenuItemState state);

    void Teardown();
    i32 TryActivate();
    i32 Deactivate();
    i32 SelectToolResource(StatusBarHighlightRow row);
    i32 SelectToyResource(StatusBarHighlightRow row);
    i32 SelectBrickResource(StatusBarHighlightRow row);
    i32 SetTab(GameTabContent tab, b32 forceReload);
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
    i32 HandleDoubleClick(i32 keyFlags, i32 x, i32 y);

    i32 OnPointerRelease(i32 keyFlags, i32 x, i32 y);
    i32 HandlePointerDrag(i32 keyFlags, i32 x, i32 y);
    CStatusBarItem* HitTestRects(i32 x, i32 y);
    void ResetWidgets(b32 keepLists);
    void ClearTabGroup();
    void AddTabItem(i32 tab, CStatusBarItem* item) {
        m_tabLists[tab].AddTail(item);
    }
    i32 ClearStat(i32 idx);
    void EnterHlRow(i32 row, i32 group);
    void InitTabRects();
    i32 DropFallingItemAt(i32 screenX, i32 screenY, i32 itemFrame);
    void ExitMode();
    i32 ActivateSlot(i32 idx);
    i32 PlaceCursorTarget(i32 unitIndex, i32 activateCamera);

    i32 SetState(StatusBarDock state);
    i32 RestoreStatusBar();
    i32 SetSpritePos(i32 x, i32 y);
    i32 HitTestLayer(i32 x, i32 y);
    i32 QueuePickupReward(i32 pickupValue, i32 score);
    void ReportTab(i32 tab);

    i32 DockStatusBarLeft();
    i32 HideRect();

    void AdvanceTab(i32 reverse);

    i32 DockStatusBarRight();

    StatusBarDock m_position;
    StatusBarDock m_restorePosition;

    class CWwdSpriteObject* m_barSprite;

    CDDrawSurfaceMgr* m_world;

    RECT m_barRect;
    i32 m_redrawFrames;
    i32 m_barX;
    i32 m_barY;

    CPtrList m_tabLists[8];
    StatusBarTab m_activeTab;
    GameTabContent m_itemKind;
    StatusSampleMode m_statFlags[STATUSBAR_GRUNT_SLOT_COUNT];
    CSBI_SideTab* m_hitRects[STATUSBAR_GRUNT_SLOT_COUNT];

    CSBI_StatzTabArrow* m_statObj[STATUSBAR_GRUNT_SLOT_COUNT];
    CSBI_MenuItem* m_statzTabButton;
    CSBI_MenuItem* m_resourceTabButton;
    CSBI_MenuItem* m_gruntzTabButton;
    CSBI_MenuItem* m_multiTabButton;
    CSBI_MenuItem* m_gameTabButton;
    CSBI_MenuItem* m_gameResumePauseButton;
    CSBI_MenuItem* m_gameLoadButton;
    CSBI_MenuItem* m_gameSaveButton;
    CSBI_MenuItem* m_gameSettingsButton;
    CSBI_MenuItem* m_gameHelpButton;
    CSBI_MenuItem* m_gameQuitButton;
    CSBI_MenuItem* m_endPrimaryButton;
    CSBI_MenuItem* m_endSecondaryButton;
    CSBI_MenuItem* m_confirmYesButton;
    CSBI_MenuItem* m_confirmNoButton;

    CSBI_ImageSet* m_slotNotify[5];
    CStatusBarItem* m_gruntWellBackground;
    CSBI_WellGoo* m_gruntWellGoo;

    CSbiSlot m_slots[5];

    i32 m_gruntWellLevel;
    i32 m_gruntWellTargetLevel;

    SbiClockPair m_reserved2a0;
    SbiClockPair m_reserved2b0;

    CSbiHlRow m_conveyorSlots[3];
    CSBI_ImageSet* m_conveyorSprites[3];
    char m_pad314[0x318 - 0x314];

    CSbiHlRow m_rightMachine;
    CSbiHlRow m_leftMachine;
    CSBI_GruntMachine* m_machineDisplay;
    i32 m_reserved34c;
    i32 m_reserved350;
    b32 m_chatBoxDisabled;
    b32 m_tabsBuilt;
    i32 m_activeSlot;
    StatusBarHighlightRow m_pendingHlRow;
    CStatusBarItem* m_resourceMainBackground;
    CStatusBarItem* m_resourceMachineFramework;
    CStatusBarItem* m_resourceUpperBackground;
    CStatusBarItem* m_resourceWindowBackground;
    char m_pad374[0x378 - 0x374];
    CSbiHlRow m_resourceSlots[12];
    CSBI_ImageSet* m_resourceSlotSprites[12];
    SbiBeltPhase m_machinePhase;
    i32 m_machineItem;
    SbiClockPair m_beltClock;
    CSBI_ImageSet* m_machineItemSprite;
    char m_pad4e4[0x4e8 - 0x4e4];
    SbiFallingItemState m_fallActive;
    i32 m_fallingItem;
    SbiClockPair m_fallClock;
    CSBI_ImageSet* m_fallingItemSprite;
    RECT m_fallingItemRect;
    RECT m_machineItemRect;
    i32 m_machineItemTargetX;
    b32 m_rezActive;
    i32 m_rezTick;

    CPtrArray m_rewardQueue;
    i32 m_reserved544;

    b32 m_hlBusy;
    CWarpStoneFly* m_retabNotify;
    b32 m_levelOverlayActive;
    b32 m_quitConfirmationActive;
    DestructWarningState m_destructWarningState;
    DestructButtonFrame m_destructButtonFrame;
    SbiClockPair m_destructWarningClock;
    CSBI_ImageSet* m_destructButtonImage;
    b32 m_destructButtonLocked;
    b32 m_observerTabAvailable;
    i32 m_battlezPct[38];
    i32 m_barFrameGate;
    SoundBuffer* m_destructWarningSound;

    CSBI_WarlordHead* m_warlordHead[4];
    i32 m_tabCycle;
};

inline CStatusBarMgr::CStatusBarMgr() {
    m_statzTabButton = NULL;
    m_resourceTabButton = NULL;
    m_gruntzTabButton = NULL;
    m_multiTabButton = NULL;
    m_gameTabButton = NULL;
    m_gameResumePauseButton = NULL;
    m_gameLoadButton = NULL;
    m_gameSaveButton = NULL;
    m_gameSettingsButton = NULL;
    m_gameHelpButton = NULL;
    m_gameQuitButton = NULL;
    m_destructWarningSound = NULL;
    m_endPrimaryButton = NULL;
    m_endSecondaryButton = NULL;
    m_confirmYesButton = NULL;
    m_confirmNoButton = NULL;
    m_barSprite = NULL;
    m_world = NULL;
    m_redrawFrames = 0;
    m_activeTab = TAB_NONE;
    m_chatBoxDisabled = false;
    m_tabsBuilt = false;
    m_levelOverlayActive = false;
    m_quitConfirmationActive = false;
    m_barFrameGate = 0x1e0;
    m_tabCycle = 0;
    memset(m_statFlags, 0, sizeof(m_statFlags));
    memset(m_hitRects, 0, sizeof(m_hitRects));
    memset(m_statObj, 0, sizeof(m_statObj));
    memset(m_slotNotify, 0, sizeof(m_slotNotify));
    memset(m_conveyorSprites, 0, sizeof(m_conveyorSprites));
    memset(m_resourceSlotSprites, 0, sizeof(m_resourceSlotSprites));
    memset(m_warlordHead, 0, sizeof(m_warlordHead));
    m_resourceMainBackground = NULL;
    m_resourceUpperBackground = NULL;
    m_resourceWindowBackground = NULL;
    m_resourceMachineFramework = NULL;
    m_machineItemSprite = NULL;
    m_fallingItemSprite = NULL;
    m_machineDisplay = NULL;
    m_destructButtonImage = NULL;
    m_gruntWellBackground = NULL;
    m_gruntWellGoo = NULL;
    m_gruntWellTargetLevel = GRUNT_WELL_EMPTY;
    m_gruntWellLevel = GRUNT_WELL_EMPTY;
    m_reserved544 = 1;
    m_hlBusy = false;
    m_retabNotify = NULL;
    m_destructButtonLocked = false;
}

#endif // GRUNTZ_SBI_RECTONLY_H
