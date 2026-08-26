#ifndef SRC_GRUNTZ_CPLAY_H
#define SRC_GRUNTZ_CPLAY_H

#include <rva.h>

#include <Mfc.h>

#include <Clock64.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <Gruntz/ColorTint.h>
#include <Gruntz/CoordNode.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameStateId.h>
#include <Gruntz/GruntDeathType.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/PlayerCommandKind.h>
#include <Gruntz/QuestLevel.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/State.h>
#include <Gruntz/StatusBarDock.h>
#include <Gruntz/Timer.h>
#include <Gruntz/View.h>
#include <Gruntz/ViewportResizeMode.h>
#include <Io/SaveGame.h>
#include <Rez/FrameClock.h>

class MidiManager;
class MidiSequence;
class CGameStats;
class CChatBoxOwner;
class CFontConfig;
class CWorldSoundSet;
class CVoiceManager;
class CGruntzCmdMgr;
class CTriggerMgr;
class CStatusBarMgr;
class CMinimap;
class CTileTriggerContainer;
struct CGameObject;
class CWwdSpriteObject;

class CMulti;

class CFileMemBase;

class CImage;

class CDDrawWorker;

class CPlay : public CState {
public:
    struct ClockInterval {
        Clock64 m_start;
        Clock64 m_interval;

        ClockInterval() {
            m_start.m_lo = 0;
            m_interval.m_lo = 0;
            m_start.m_hi = 0;
            m_interval.m_hi = 0;
        }
    };

    QuestLevel CurrentQuestLevel() const {
        return static_cast<QuestLevel>(m_levelIndex);
    }

    void DrawCustomLevelBanner();

    void DrawDebugStatsFull();

    CPlay();
    virtual ~CPlay() OVERRIDE;

    RVA(0x0008c910, 0x6)
    virtual GameStateId Update() OVERRIDE {
        return GAMESTATE_PLAY;
    }
    virtual i32 Render() OVERRIDE;

    virtual i32 LoadGameAssetNamespaces(CGruntzMgr*, i32, i32) OVERRIDE;

    virtual void ReleaseResources() OVERRIDE;

    virtual i32 RestoreDisplay() OVERRIDE;

    virtual i32 InputVirtual() OVERRIDE;
    virtual i32 EnterState(GameStateId previousState) OVERRIDE;
    virtual i32 LeaveState(GameStateId nextState) OVERRIDE;
    virtual i32 OnChar(i32 charCode, i32 keyData) OVERRIDE;
    virtual i32 OnKeyDown(i32 virtualKey, i32 keyData) OVERRIDE;
    virtual i32 OnKeyUp(i32 virtualKey, i32 keyData) OVERRIDE;
    virtual i32 OnLButtonDown(i32 eventArg, i32 x, i32 y) OVERRIDE;
    virtual i32 OnLButtonUp(i32 keyFlags, i32 x, i32 y) OVERRIDE;
    virtual i32 OnLButtonDblClk(i32 keyFlags, i32 x, i32 y) OVERRIDE;
    virtual i32 OnRButtonDown(i32 keyFlags, i32 x, i32 y) OVERRIDE;
    virtual i32 OnRButtonUp(i32 keyFlags, i32 x, i32 y) OVERRIDE;
    virtual i32 OnRButtonDblClk(i32 keyFlags, i32 x, i32 y) OVERRIDE;
    RVA(0x0008c970, 0x1c)
    virtual i32 OnMouseMove(i32 keyFlags, i32 cursorX, i32 cursorY) OVERRIDE {
        m_cursorX = cursorX;
        m_cursorY = cursorY;
        return 1;
    }
    virtual i32 CompleteLevel() OVERRIDE;
    virtual i32 PauseGame() OVERRIDE;
    virtual i32 ResumeGame() OVERRIDE;

    RVA(0x0008c930, 0x3)
    virtual i32 UnusedPlayQuery() {
        return 0;
    }
    RVA(0x0008c950, 0x3)
    virtual i32 GetFrame() {
        return 0;
    }

    virtual i32 CountObjectsByCategory(i32 category);

    virtual i32 LoadImageBanks();

    virtual i32 LoadByMode(i32 level, i32 unused);

    virtual i32 HandleDragMove(i32 keyFlags, i32 x, i32 y);
    virtual void OnExit();
    virtual void FreeListTeardown();
    virtual void ModeCleanup();

    virtual i32 DrawStateMessage();

    RVA(0x000d0030, 0x1)
    virtual void PostLoadImageBanks() {}

    virtual void PostSetup(HDC dc);

    virtual void TickStateMgrs();

    virtual void UpdateWorldFrame();
    virtual i32 UpdateWorldFixedSteps();
    virtual i32 BuildMusicCategoryTable(i32);
    virtual i32 BuildWorldLevelPath(i32);

    Coord* StartMarkerAt(i32 index) {
        return static_cast<Coord*>(m_startMarkers.GetAt(index));
    }
    i32 StartMarkerCount() {
        return m_startMarkers.GetSize();
    }
    Coord* PlacedObjectCellAt(i32 group, i32 index) {
        return static_cast<Coord*>(m_placedObjectCells[group].GetAt(index));
    }
    i32 PlacedObjectCellCount(i32 group) {
        return m_placedObjectCells[group].GetSize();
    }
    Coord* CameraBookmarkAt(i32 index) {
        return static_cast<Coord*>(m_cameraBookmarks.GetAt(index));
    }
    void SetCameraBookmarkAt(i32 index, Coord* bookmark) {
        m_cameraBookmarks.SetAt(index, bookmark);
    }
    i32 CameraBookmarkCount() {
        return m_cameraBookmarks.GetSize();
    }

    i32 RestoreCursorSaveUnder();

    void PlayCueAt(
        i32 cueId,
        i32 fontSel,
        i32 toFrontPage,
        i32 r,
        i32 g,
        i32 b,
        i32 flag,
        RECT* rectSrc
    );

    i32 PostActionCue(i32 cueId);

    void DrawMessageFrame(i32 index, b32 useFront);

    void LoadSBITextEdges(i32 msgId);
    i32 BuildGruntNamespaceList(CMulti* finishGate);

    i32 StepViewportResize();
    i32 GetAmbientId();
    void StepScroll();
    i32 SetDarknessCurse(b32 active);
    i32 SetTinyViewportCurse(b32 active);
    i32 SetMonitorCurse(b32 active);
    i32 SetRandomMoveIconsCurse(b32 active);

    i32 ShrinkViewport(i32 step);
    i32 ExpandViewport(i32 step);
    i32 NotifyVisibleEntities();

    i32 ResetViewport();

    void RegionEnter();
    void RegionLeave();

    i32 ProfileDeltaFrame();
    i32 ProfileInputFrame();

    void DrawDebugStats();

    i32 LoadCursorAnimation(
        const char* spriteKey,
        i32 initialFrame,
        b32 animate,
        i32 frameDelayMs,
        b32 tintForPlayer
    );
    i32 AdvanceCursorAnimation(i32 elapsedMs);
    i32 ResetGoals(i32, i32);

    i32 PositionBridgeToggle(StatusBarDock mode, StatusBarDock unused);

    b32 PlaceStartGruntz();
    i32 ValidateLevelTiles();

    i32 BuildHelpReveal(b32 final);
    i32 RegisterInputBindings();

    i32 LoadLevelAnims(i32 force);

    i32 DrawLevelInfoText();

    i32 LoadLoadingBarSprite();

    i32 ForwardReady();
    void ResetRightClickState();

    i32 QuitToMenu();

    i32 SetCursorFrame(i32 item);

    i32 ExecuteCommand(
        u8 playerIndex,
        char unitIndex,
        GZ_ENUM_STORAGE(PlayerCommandKind, char) commandKind,
        i16 targetXOrPlayerIndex,
        i16 targetYOrUnitIndex,
        char pickupType,
        u8 scheduleSlot
    );
    i32 Flip();

    i32 CloseLevelOverlay(i32 unused);
    i32 ClearPlacedObjects();
    i32 FlushPendingOps();

    i32 SetDefeatCountdown(b32 active, i32 durationMs);
    i32 CanQuickSave();
    i32 PostHudRect();

    i32 DrawWorldPresent();

    i32 OpenLevelOverlay(b32 showQuitConfirmation);

    i32 LoadActionTileSprites(i32 force);
    i32 LoadLevelSounds(i32 force);
    i32 LoadLevelImages(i32 force);
    i32 LoadGameImages(i32 force);
    i32 LoadGameSounds(i32 force);
    i32 LoadGameAnims(i32 force);
    i32 LoadGruntSoundNamespaces(CMulti* notify);
    i32 BuildSpriteImageKeyTable(CMulti* notify);
    i32 BuildAnizKeyTable(CMulti* notify);

    i32 EnterMode(GameStateId mode);
    i32 ResetPlayState();

    i32 FindStartPointAt(i32 x, i32 y, i32* outX, i32* outY);

    i32 AddLevelGruntz();

    i32 SetEffectSpriteDurations();

    i32 BuildWarlordNameTable(CMulti* finishGate);

    i32 LoadWarlordSprites(CMulti* ctx, i32* loaded);

    i32 SerializeDispatch(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, i32 payload);

    i32 SerializeHeader(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, i32 payload);
    i32 SavePlayState(CFileMemBase* ar);
    i32 LoadPlayState(CFileMemBase* ar);

    CString m_reserved1b4;
    char m_pad1b8[0x1bc - 0x1b8];
    b32 m_returnToMenuOnComplete;
    b32 m_completedFinalLevel;
    b32 m_initialFramePending;
    i32 m_reserved1c8;
    i32 m_savedClock;

    SaveSlot m_saveSlot;
    i32 m_packetsRcvd;
    i32 m_packetsSent;
    i32 m_rngSeed;

    CStatusBarMgr* m_statusBar;

    CChatBoxOwner* m_chatBox;

    CTileTriggerContainer* m_tileTriggers;
    b32 m_dragSnapActive;
    b32 m_dragInProgress;
    i32 m_reserved2f0;
    i32 m_cursorFrame;
    i32 m_cursorId;
    Coord m_cursorOffset;
    i32 m_dragClampMaxX;
    i32 m_dragClampMaxY;
    b32 m_worldReady;
    RECT m_hudRect;

    CMinimap* m_minimap;
    char m_pad324[0x328 - 0x324];
    ClockInterval m_bootyTiming;

    ClockInterval m_ambientTiming;
    b32 m_ambientInitDone;
    char m_pad34c[0x350 - 0x34c];
    ClockInterval m_syncTiming;
    Coord m_tileClick;
    b32 m_dragInhibit1;
    b32 m_dragInhibit2;

    CPtrArray m_startMarkers;

    struct Anchor {
        i32 m_x;
        i32 m_y;
    };
    Anchor m_anchors[4];

    CPtrArray m_placedObjectCells[4];
    CTimer* m_levelTimer;
    ClockInterval m_cueTiming;
    b32 m_cueToggle;
    i32 m_lastCueId;
    CString m_cueText;
    b32 m_drewThisFrame;

    POINT m_pathPreviewSource;
    POINT m_pathPreviewDestination;
    i16 m_pathPreviewColor;
    char m_pad42a[0x430 - 0x42a];

    ClockInterval m_region0Timing;
    ClockInterval m_region1Timing;
    ClockInterval m_region2Timing;
    ClockInterval m_region3Timing;
    b32 m_region0Gate;
    b32 m_region1Gate;
    b32 m_region2Gate;
    b32 m_region3Gate;
    ViewportResizeMode m_viewportResizeMode;
    b32 m_hudSuppressed;

    CPtrArray m_cameraBookmarks;
    i32 m_cameraBookmarkIndex;
    ClockInterval m_defeatCountdownTiming;
    b32 m_defeatCountdownActive;
    i32 m_scrollEdgeActive;
    i32 m_scrollEdgeLock;
    i32 m_revealFrame;

    CImage *m_revealCapMid, *m_revealCapEnd, *m_revealCapStart;

    CDDrawWorker* m_cursorSprite;
    CImage* m_cursorImage;
    b32 m_cursorUsesPlayerTint;
    i32 m_cursorFrameDelayMs;
    i32 m_cursorFrameCountdownMs;
    i32 m_cursorFrameIndex;

    CWwdSpriteObject* m_cursorSnapSprite;
    b32 m_cursorAnimationActive;
    b32 m_renderDisabled;
    b32 m_playerCommandPending;
    b32 m_winLoseBanner;
    b32 m_inGame;
    b32 m_levelOverlayOpen;
    b32 m_paused;
    b32 m_cursorTargetValid;
    i32 m_lastScrollTimeX;
    i32 m_lastScrollTimeY;
    i32 m_stepCountdown;
    i32 m_focusPlayerIndex;
    MidiSequence* m_savedMusicSequence;
    i32 m_reserved51c;

    i32 SaveUnderAndDrawCursor(CDDrawSurfacePair* pair);
    i32 LoadCursorSprites(i32 cursorId, b32 targetValid);
    i32 LoadScrollSpeedOptions();
    i32 BuildGruntTypeNameTable(PickupType typeIdx, i32 mode, i32 lightGate, CMulti* finishGate);

    i32 ScanBuildTiles();
    i32 ScanShuffleQuads();
};

ColorTint FindAvailablePlayerColor();
void SetPlayerColorAvailable(ColorTint color, b32 available);
i32 IsPlayerColorAvailable(ColorTint color);
void ResetPlayerColorAvailability();

extern GruntDeathType g_areaPitDeath;

extern b32 g_playActive;
extern i32 g_deactivateProfileMs;
extern i32 g_flipProfileMs;
extern b32 g_playerColorAvailable[TINT_COUNT];

extern i32 g_lastLevelNum;
extern GruntDeathType g_areaHazardDeath;
extern b32 g_levelBias100;
extern char* g_colorNames[];
extern char* g_difficultyNames[];

void Cmd_ApplyScrollParams(i32 durationMs, i32 jitterX, i32 jitterY, i32 panMinX, i32 panMaxX);
CString GetColorName(i32 colorIdx, b32 upper);
CString GetDifficultyName(i32 diffIdx, b32 upper);

i32 LayerBlitFrame(
    CDDrawSurfaceMgr* surfaceMgr,
    CImage* src,
    i32 x,
    i32 y,
    b32 useFront,
    b32 useColorKey
);
void UpdateMgrScroll(CGruntzMgr* pm, CStatusBarMgr* bar, b32 snapFlag);
i32 DrawTextToOverlaySurface(
    CDDrawSurfaceMgr* surfaceMgr,
    CString* text,
    RECT* box,
    i32 fontSel,
    i32 shadow,
    i32 r,
    i32 g,
    i32 b,
    i32 flag
);
i32 DrawTextToBackSurface(
    CDDrawSurfaceMgr* surfaceMgr,
    CString* text,
    RECT* box,
    i32 fontSel,
    i32 shadow,
    i32 r,
    i32 g,
    i32 b,
    i32 flag
);
void Cmd_ResetScroll();
i32 InitializeLevelArea(i32 levelIndex);
void ActiveWait(u32 ms);

inline CPlay::~CPlay() {
    CPlay::ReleaseResources();
}

// @early-stop
inline CPlay::CPlay() {
    m_returnToMenuOnComplete = false;
    m_completedFinalLevel = false;
    m_reserved1c8 = 0;
    m_chatBox = NULL;
    m_levelTimer = NULL;
    m_statusBar = NULL;
    m_tileTriggers = NULL;
    m_cursorSprite = NULL;
    m_cursorSnapSprite = NULL;
    m_reserved2f0 = 0;
    m_packetsRcvd = 0;
    m_packetsSent = 0;
    m_cursorFrame = 0;
    m_cursorId = -1;
    m_minimap = NULL;
    m_cursorUsesPlayerTint = false;
    m_defeatCountdownActive = false;
    m_ambientInitDone = true;
    m_stepCountdown = 0;
    m_savedMusicSequence = NULL;
    m_worldReady = false;
    m_dragSnapActive = false;
    m_playerCommandPending = false;
    m_dragInhibit1 = false;
    m_dragInhibit2 = false;
    m_dragInProgress = false;
    m_cursorTargetValid = false;
}

#endif // SRC_GRUNTZ_CPLAY_H
