#ifndef GRUNTZ_CSTATUSBARMGR_H
#define GRUNTZ_CSTATUSBARMGR_H

#include <rva.h>

#include <Bute/ButeMgr.h>
#include <Enums.h>
#include <Gruntz/ClockInterval.h>
#include <Gruntz/CoordPool.h>
#include <Gruntz/DestructWarningState.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameTabContent.h>
#include <Gruntz/GruntzCommandId.h>
#include <Gruntz/LogicTypeId.h>
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
#include <Gruntz/TriggerGridDimensions.h>
#include <Gruntz/WarpStoneFragment.h>
#include <Ints.h>
#include <MakeRect.h>

class CSBI_ImageSet;
class CSBI_WellGoo;
class CWarpStoneFly;
class CSBI_MenuItem;
class CSBI_GruntMachine;
class SoundBuffer;
GZ_ENUM_BEGIN(GruntOvenSlotState)
    GRUNT_OVEN_EMPTY = 0,
    GRUNT_OVEN_COOKING = 1,
    GRUNT_OVEN_READY = 2
GZ_ENUM_END(GruntOvenSlotState)

struct GruntOvenSlot {
    GruntOvenSlotState m_state;
    i32 m_frameIndex;
    ClockInterval m_cookingClock;
};

struct CSbiHlRow {
    RVA(0x000c86d0, 0x11)
    CSbiHlRow() {}

    i32 m_state;

    union {
        i32 m_value;
        i32 m_counter;
    };
    ClockInterval m_clock;
};

struct CSbiMachineRow {
    i32 m_state;

    union {
        i32 m_value;
        i32 m_counter;
    };
    ClockInterval m_clock;
};

class CSBI_SideTab;
class CSBI_StatzTabArrow;
class CSBI_WarlordHead;
class CWarpStoneFly;

const i32 s_gruntOvenReadyFrame = 0x1a;

const i32 s_activateErrId = 0x80e4;
const i32 s_activateErrTag = 0x44b;

const i32 s_setTabErrTag = 0x44a;

GZ_ENUM_CONST_BEGIN(GruntWellPct)
    GRUNT_WELL_EMPTY = 0,
    GRUNT_WELL_FULL = 100
GZ_ENUM_CONST_END(GruntWellPct)

#define DELETE_STATUS_ITEMS(list)                                                                  \
    {                                                                                              \
        POSITION pos = (list).GetHeadPosition();                                                   \
        while (pos) {                                                                              \
            delete static_cast<CStatusBarItem*>((list).GetNext(pos));                              \
        }                                                                                          \
        (list).RemoveAll();                                                                        \
    }

class CStatusBarMgr {
    inline b32 BeginGruntPlacement(i32 slot);

public:
    CStatusBarMgr();
    i32 BuildSideTabs();

    RVA(0x000c8980, 0x64)
    ~CStatusBarMgr() {
        Teardown();
    }

    i32 BuildActiveTabContent();
    i32 BuildGameTabContent();

    void StartDestructWarning(i32 countdownMs);
    i32 StartWarpStoneFly(i32 srcX, i32 srcY, WarpStoneFragment fragment);
    void ResetCounters();
    void ResetGruntOvens();
    void EmptyGruntOven(i32 idx);
    i32 StartAvailableGruntOven();
    void AdvanceGruntWell(i32 delta);
    void DrainGruntWell(i32 delta);
    void SetGruntWellTarget(i32 value);
    void SetGruntWell(i32 value);
    void UpdateStatusSystems();
    void Reset();
    void ToggleUnitSample(i32 unitIndex);
    void SetLeftRezMachineAnimation(i32 initialFrame, SbiMachineState state, i32 frameDelayMs);
    void SetRightRezMachineAnimation(i32 initialFrame, SbiMachineState state, i32 frameDelayMs);
    void FinishGruntPlacement(b32 placed);
    void ClearResourceSlot(i32 category, StatusBarHighlightRow row);
    i32 AddResourceToSlot(i32 category, i32 pickupValue, i32 row);
    i32 AddResourceToRow(i32 pickupValue, i32 row);
    i32 ConsumeReadyGrunt();
    void LockDestructButton(i32 resetWarningAnimation);

    i32 BuildStatusBarTabs();

    i32 BuildLevelOverlay();
    i32 PrepareNextResource();
    i32 Initialize(CGameWorld* world);
    i32 Render();
    i32 HandleClick(i32 mouseFlags, i32 screenX, i32 screenY);
    i32 UpdateStatusBar(i32 deltaMs);
    void BuildGameTabResumeButton(b32 show);
    void BuildGameTabPauseButton();

    i32 StartGruntOven(i32);
    void UpdateRezConveyorStatusBar();
    void UpdateResourceMachineAnimation();
    void ResetResourceMachine();
    void UpdateResourceDeliveryAnimation();
    i32 StartResourceGrinderDrop(i32 item, i32 x, i32 y);
    i32 RequestResourceDelivery();
    void ResetForLevel(i32);

    void ResetConveyorBelts();

    i32 SerializeDispatch(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, i32 payload);

    i32 GetNextResourcePickup();
    i32 SetUnitSampleMode(i32 unitIndex, StatusSampleMode sampleMode);
    void UpdateGruntOvenStatusBar();
    void TickGruntWell();
    void UpdateChipGrinderStatusBar();
    void RefreshResourceImages();
    void UpdateDestructWarningAnimation();
    i32 CreateCollapsedSprite();
    i32 SetButtonState(SbiCommandId cmd, SbiMenuItemState state);

    void Teardown();
    i32 TryActivate();
    i32 RequestRedraw();
    i32 SelectToolResource(StatusBarHighlightRow row);
    i32 SelectToyResource(StatusBarHighlightRow row);
    i32 SelectBrickResource(StatusBarHighlightRow row);
    i32 SetGameTabContent(GameTabContent content, b32 forceReload);
    i32 ClearButtonHighlights(StatusBarTab idx);
    i32 HitTestSideTabs(i32 x, i32 y);
    i32 Serialize(CFileMemBase* s);
    i32 Deserialize(CFileMemBase* s);

    i32 ConfigureRect(
        i32 sub,
        CGameWorld* host,
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
    i32 HandleDoubleClick(i32 keyFlags, i32 screenX, i32 screenY);

    i32 OnPointerRelease(i32 keyFlags, i32 x, i32 y);
    i32 HandlePointerDrag(i32 keyFlags, i32 screenX, i32 screenY);
    CStatusBarItem* HitTestItems(i32 screenX, i32 screenY);
    void ResetWidgets(b32 deleteCollapsedSprite);
    void ClearActiveTabContent();
    void AddTabItem(i32 tab, CStatusBarItem* item) {
        m_tabLists[tab].AddTail(item);
    }
    i32 ClearUnitSample(i32 unitIndex);
    void FinishResourcePlacement(i32 consumed, i32 pickupValue);
    void ResetResourceSlots();
    i32 DropFallingItemAt(i32 screenX, i32 screenY, i32 itemFrame);
    void CloseLevelOverlay();
    i32 SelectGruntOvenForPlacement(i32 idx);
    i32 PlaceCursorTarget(i32 unitIndex, i32 activateCamera);

    const RECT* GetBarRect() const {
        return &m_barRect;
    }
    StatusBarDock GetDockState() const {
        return m_position;
    }
    StatusBarTab GetActiveTab() const {
        return m_activeTab;
    }
    void SetActiveTab(StatusBarTab tab) {
        m_activeTab = tab;
    }
    i32 SetDockState(StatusBarDock state);
    i32 RestoreStatusBar();
    i32 SetCollapsedSpritePosition(i32 x, i32 y);
    i32 HitTestCollapsedSprite(i32 screenX, i32 screenY);
    i32 QueuePickupReward(i32 pickupValue, i32 score);
    void DiscardSelectedResource(i32 pickupValue);

    i32 DockStatusBarLeft();
    i32 HideStatusBar();

    void CycleMultiplayerPlayer(i32 reverse);

    i32 DockStatusBarRight();

    StatusBarDock m_position;
    StatusBarDock m_restorePosition;

    class CWwdSpriteObject* m_collapsedSprite;

    CGameWorld* m_world;

    RECT m_barRect;
    i32 m_redrawFrames;
    i32 m_collapsedSpriteX;
    i32 m_collapsedSpriteY;

    CPtrList m_tabLists[8];
    StatusBarTab m_activeTab;
    GameTabContent m_gameTabContent;
    StatusSampleMode m_unitSampleModes[TM_UNITS_PER_PLAYER];
    CSBI_SideTab* m_unitSideTabs[TM_UNITS_PER_PLAYER];

    CSBI_StatzTabArrow* m_unitSampleArrows[TM_UNITS_PER_PLAYER];
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

    CSBI_ImageSet* m_gruntOvenImages[5];
    CStatusBarItem* m_gruntWellBackground;
    CSBI_WellGoo* m_gruntWellGoo;

    GruntOvenSlot m_gruntOvenSlots[5];

    i32 m_gruntWellLevel;
    i32 m_gruntWellTargetLevel;

    // @identity-TODO: constructor and SerializeClockPair prove both clock objects;
    // only m_reserved2b0 is additionally reset, and neither drives a status-bar action.
    ClockInterval m_reserved2a0;
    ClockInterval m_reserved2b0;

    CSbiHlRow m_conveyorSlots[3];
    CSBI_ImageSet* m_conveyorSprites[3];

    CSbiMachineRow m_rightMachine;
    CSbiMachineRow m_leftMachine;
    CSBI_GruntMachine* m_machineDisplay;
    // @identity-TODO: both words are save-streamed without a status-bar consumer.
    i32 m_reserved34c;
    i32 m_reserved350;
    b32 m_gameplayControlsDisabled;
    b32 m_tabsBuilt;
    i32 m_selectedGruntOvenSlot;
    StatusBarHighlightRow m_selectedResourceRow;
    CStatusBarItem* m_resourceMainBackground;
    CStatusBarItem* m_resourceMachineFramework;
    CStatusBarItem* m_resourceUpperBackground;
    CStatusBarItem* m_resourceWindowBackground;
    CSbiHlRow m_resourceSlots[12];
    CSBI_ImageSet* m_resourceSlotSprites[12];
    SbiBeltPhase m_resourceDeliveryPhase;
    i32 m_deliveryPickupType;
    ClockInterval m_resourceDeliveryClock;
    CSBI_ImageSet* m_deliveryItemDisplay;
    // @identity-TODO: unaccessed word required by m_grinderState's retail offset.
    char m_pad4e4[0x4e8 - 0x4e4];
    SbiFallingItemState m_grinderState;
    i32 m_grinderPickupType;
    ClockInterval m_grinderClock;
    CSBI_ImageSet* m_grinderItemDisplay;
    RECT m_grinderItemRect;
    RECT m_deliveryItemRect;
    i32 m_deliveryTargetX;
    b32 m_resourceDeliveryActive;
    i32 m_pendingResourceDeliveries;

    CPtrArray m_rewardQueue;
    // @identity-TODO: initialized to 1 and save-streamed; reward processing never reads it.
    i32 m_reserved544;

    Coord* GetReward(i32 index) const {
        return static_cast<Coord*>(m_rewardQueue.GetAt(index));
    }
    void ClearRewardQueue() {
        for (i32 i = 0; i < m_rewardQueue.GetSize(); i++) {
            Coord* reward = GetReward(i);
            if (reward) {
                g_coordPool.Push(reward);
            }
        }
        m_rewardQueue.RemoveAll();
    }

    b32 m_layoutLocked;
    CWarpStoneFly* m_warpStoneFly;
    b32 m_levelOverlayActive;
    b32 m_quitConfirmationActive;
    DestructWarningState m_destructWarningState;
    DestructButtonFrame m_destructButtonFrame;
    ClockInterval m_destructWarningClock;
    CSBI_ImageSet* m_destructButtonImage;
    b32 m_destructButtonLocked;
    b32 m_observerTabAvailable;
    i32 m_randomRewardThresholds[38];
    i32 m_displayHeight;
    SoundBuffer* m_destructWarningSound;

    CSBI_WarlordHead* m_multiplayerHeadButtons[4];
    i32 m_multiplayerPlayerIndex;
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
    m_collapsedSprite = NULL;
    m_world = NULL;
    m_redrawFrames = 0;
    m_activeTab = TAB_NONE;
    m_gameplayControlsDisabled = false;
    m_tabsBuilt = false;
    m_levelOverlayActive = false;
    m_quitConfirmationActive = false;
    m_displayHeight = 0x1e0;
    m_multiplayerPlayerIndex = 0;
    memset(m_unitSampleModes, 0, sizeof(m_unitSampleModes));
    memset(m_unitSideTabs, 0, sizeof(m_unitSideTabs));
    memset(m_unitSampleArrows, 0, sizeof(m_unitSampleArrows));
    memset(m_gruntOvenImages, 0, sizeof(m_gruntOvenImages));
    memset(m_conveyorSprites, 0, sizeof(m_conveyorSprites));
    memset(m_resourceSlotSprites, 0, sizeof(m_resourceSlotSprites));
    memset(m_multiplayerHeadButtons, 0, sizeof(m_multiplayerHeadButtons));
    m_resourceMainBackground = NULL;
    m_resourceUpperBackground = NULL;
    m_resourceWindowBackground = NULL;
    m_resourceMachineFramework = NULL;
    m_deliveryItemDisplay = NULL;
    m_grinderItemDisplay = NULL;
    m_machineDisplay = NULL;
    m_destructButtonImage = NULL;
    m_gruntWellBackground = NULL;
    m_gruntWellGoo = NULL;
    m_gruntWellTargetLevel = GRUNT_WELL_EMPTY;
    m_gruntWellLevel = GRUNT_WELL_EMPTY;
    m_reserved544 = 1;
    m_layoutLocked = false;
    m_warpStoneFly = NULL;
    m_destructButtonLocked = false;
}

#endif // GRUNTZ_SBI_RECTONLY_H
