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
#include <Gruntz/PlayViewMode.h>
#include <Gruntz/QuestLevel.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/State.h>
#include <Gruntz/StatusBarDock.h>
#include <Gruntz/Timer.h>
#include <Gruntz/View.h>
#include <Io/SaveGame.h>
#include <Rez/FrameClock.h>

class CGruntzSoundZ;
class CGruntzSoundInnerZ;
class CBattlezData;
class CChatBoxOwner;
class CFontConfig;
class CWorldSoundSet;
class CGruntSpawnConfig;
class CGruntzCmdMgr;
class CTriggerMgr;
class CStatusBarMgr;
class CLightFxRender;
class CTileTriggerContainer;
struct CGameObject;
class CWwdGameObjectA;

class CMulti;

class CFileMemBase;

class CImage;

class CDDrawWorker;

class CPlay : public CState {
public:
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
    virtual i32 EnterState(GameStateId) OVERRIDE;
    virtual i32 LeaveState(GameStateId) OVERRIDE;
    virtual i32 OnChar(i32, i32) OVERRIDE;
    virtual i32 OnKeyDown(i32, i32) OVERRIDE;
    virtual i32 OnKeyUp(i32, i32) OVERRIDE;
    virtual i32 OnLButtonDown(i32, i32, i32) OVERRIDE;
    virtual i32 OnLButtonUp(i32, i32, i32) OVERRIDE;
    virtual i32 OnLButtonDblClk(i32, i32, i32) OVERRIDE;
    virtual i32 OnRButtonDown(i32, i32, i32) OVERRIDE;
    virtual i32 OnRButtonUp(i32, i32, i32) OVERRIDE;
    virtual i32 OnRButtonDblClk(i32, i32, i32) OVERRIDE;
    virtual i32 OnMouseMove(i32, i32, i32) OVERRIDE;
    virtual i32 CompleteLevel() OVERRIDE;
    virtual i32 PauseGame() OVERRIDE;
    virtual i32 ResumeGame() OVERRIDE;

    virtual i32 UnusedPlayQuery();
    virtual i32 GetFrame();

    virtual i32 CountObjectsByCategory(i32 category);

    virtual i32 LoadImageBanks();

    virtual i32 LoadByMode(i32 level, i32 unused);

    virtual i32 HandleDragMove(i32 a, i32 x, i32 y);
    virtual void OnExit();
    virtual void FreeListTeardown();
    virtual void ModeCleanup();

    virtual i32 DrawStateMessage();

    RVA(0x000d0030, 0x1)
    virtual void PostLoadImageBanks() {}

    virtual void PostSetup(void* dc);

    virtual void TickStateMgrs();

    virtual void DrawWorldFrame();
    virtual i32 DrawWorldFrames();
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
    void** CameraBookmarkData() {
        return m_cameraBookmarks.GetData();
    }
    i32 CameraBookmarkCount() {
        return m_cameraBookmarks.GetSize();
    }

    i32 StepInputA();

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

    void DrawMessageFrame(i32 index, i32 useFront);

    void LoadSBITextEdges(i32 msgId);
    i32 BuildGruntNamespaceList(CMulti* arg);

    i32 StepViewportResize();
    i32 GetAmbientId();
    void StepScroll();
    i32 SetDarknessCurse(i32 active);
    i32 SetTinyViewportCurse(i32 active);
    i32 SetMonitorCurse(i32 active);
    i32 SetRandomMoveIconsCurse(i32 active);

    i32 ClampViewport(i32 inset);
    i32 ClampViewport2(i32 stride);
    i32 NotifyVisibleEntities();

    i32 ResetViewport();

    void RegionEnter();
    void RegionLeave();

    i32 ProfileDeltaFrame();
    i32 ProfileInputFrame();

    void DrawDebugStats();

    i32 BeginGridWalk(const char*, i32, i32, i32, i32);
    i32 StepGridWalk(i32 dt);
    i32 ResetGoals(i32, i32);

    i32 PositionBridgeToggle(StatusBarDock mode, StatusBarDock unused);

    b32 PlaceStartGruntz();
    i32 ValidateLevelTiles();

    i32 BuildHelpReveal(i32 final);
    i32 RegisterInputBindings();

    i32 LoadLevelAnims(i32 force);

    i32 DrawLevelInfoText();

    i32 LoadLoadingBarSprite();

    i32 ForwardReady();

    i32 QuitToMenu();

    i32 SetCursorFrame(i32 item);

    i32 ExecCommand(
        u8 targetIndex,
        char gruntIndex,
        GZ_ENUM_STORAGE(PlayerCommandKind, char) cmdKind,
        i16 posX,
        i16 posY,
        char extraByte,
        u8 targetType
    );
    i32 Flip();

    i32 ReleaseLevelOverlay(i32 unused);
    i32 ClearPlacedObjects();
    i32 FlushPendingOps();

    i32 ArmSnapshot(i32 active, i32 dur);
    i32 CanQuickSave();
    i32 PostHudRect();

    i32 DrawWorldPresent();

    i32 EnterOverlayDrag(i32 arg);

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

    i32 BuildWarlordNameTable(CMulti* arg);

    i32 LoadWarlordSprites(CMulti* ctx, i32* loaded);

    i32 SyncState(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, i32 pObj);

    i32 HeaderSerialize(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, i32 pObj);
    i32 SavePlayState(CFileMemBase* ar);
    i32 LoadPlayState(CFileMemBase* ar);

    CString m_reserved1b4;
    char m_pad1b8[0x1bc - 0x1b8];
    i32 m_returnToMenuOnComplete;
    i32 m_completedFinalLevel;
    i32 m_initialFramePending;
    i32 m_reserved1c8;
    i32 m_savedClock;

    SaveSlot m_saveSlot;
    i32 m_packetsRcvd;
    i32 m_packetsSent;
    i32 m_rngSeed;

    CStatusBarMgr* m_guts;

    CChatBoxOwner* m_hitTest;

    CTileTriggerContainer* m_beginMarker;
    i32 m_dragSnapActive;
    i32 m_dragInProgress;
    i32 m_reserved2f0;
    i32 m_cursorFrame;
    i32 m_levelId;
    Coord m_cursorOffset;
    i32 m_dragClampMaxX;
    i32 m_dragClampMaxY;
    i32 m_worldReady;
    RECT m_hudRect;

    CLightFxRender* m_lightFx;
    char m_pad324[0x328 - 0x324];
    union {
        Clock64 m_bootyTimer64;
        struct {
            i32 m_bootyTimerLo, m_bootyTimerHi;
        };
    };
    union {
        Clock64 m_bootyInterval64;
        struct {
            i32 m_bootyInterval, m_bootyIntervalHi;
        };
    };

    union {
        Clock64 m_ambientTimer64;
        struct {
            i32 m_ambientTimerLo, m_ambientTimerHi;
        };
    };
    union {
        Clock64 m_ambientInterval64;
        struct {
            i32 m_ambientInterval, m_ambientIntervalHi;
        };
    };
    i32 m_ambientInitDone;
    char m_pad34c[0x350 - 0x34c];
    i32 m_syncTimerLo, m_syncTimerHi, m_syncInterval, m_syncIntervalHi;
    Coord m_tileClick;
    i32 m_dragInhibit1;
    i32 m_dragInhibit2;

    CPtrArray m_startMarkers;

    struct Anchor {
        i32 m_x;
        i32 m_y;
    };
    Anchor m_anchors[4];

    CPtrArray m_placedObjectCells[4];
    CTimer* m_frameMarker;
    union {
        Clock64 m_cueTimer64;
        struct {
            i32 m_cueTimerLo, m_cueTimerHi;
        };
    };
    union {
        Clock64 m_cueInterval64;
        struct {
            i32 m_cueInterval, m_cueIntervalHi;
        };
    };
    i32 m_cueToggle;
    i32 m_lastCueId;
    CString m_cueText;
    i32 m_drewThisFrame;

    POINT m_pathPreviewSource;
    POINT m_pathPreviewDestination;
    i16 m_pathPreviewColor;
    char m_pad42a[0x430 - 0x42a];

    union {
        Clock64 m_region0Timer64;
        struct {
            i32 m_region0TimerLo, m_region0TimerHi;
        };
    };
    union {
        Clock64 m_region0Interval64;
        struct {
            i32 m_region0Interval, m_region0IntervalHi;
        };
    };
    union {
        Clock64 m_region1Timer64;
        struct {
            i32 m_region1TimerLo, m_region1TimerHi;
        };
    };
    union {
        Clock64 m_region1Interval64;
        struct {
            i32 m_region1Interval, m_region1IntervalHi;
        };
    };
    union {
        Clock64 m_region2Timer64;
        struct {
            i32 m_region2TimerLo, m_region2TimerHi;
        };
    };
    union {
        Clock64 m_region2Interval64;
        struct {
            i32 m_region2Interval, m_region2IntervalHi;
        };
    };
    union {
        Clock64 m_region3Timer64;
        struct {
            i32 m_region3TimerLo, m_region3TimerHi;
        };
    };
    union {
        Clock64 m_region3Interval64;
        struct {
            i32 m_region3Interval, m_region3IntervalHi;
        };
    };
    i32 m_region0Gate;
    i32 m_region1Gate;
    i32 m_region2Gate;
    i32 m_region3Gate;
    PlayViewMode m_viewMode;
    i32 m_hudSuppressed;

    CPtrArray m_cameraBookmarks;
    i32 m_cameraBookmarkIndex;
    union {
        Clock64 m_snapBase64;
        struct {
            i32 m_snapBaseLo, m_snapBaseHi;
        };
    };
    union {
        Clock64 m_snapDur64;
        struct {
            i32 m_snapDur, m_snapDurHi;
        };
    };
    i32 m_snapshotActive;
    i32 m_scrollEdgeActive;
    i32 m_scrollEdgeLock;
    i32 m_revealFrame;

    CImage *m_revealCapMid, *m_revealCapEnd, *m_revealCapStart;

    CDDrawWorker* m_grid;
    CImage* m_gridCurFrame;
    i32 m_gridHasSprite;
    i32 m_gridDelayBase;
    i32 m_gridDelayCount;
    i32 m_gridRow;

    CWwdGameObjectA* m_scrollSink;
    i32 m_gridWalkActive;
    i32 m_renderDisabled;
    i32 m_playerCommandPending;
    i32 m_winLoseBanner;
    i32 m_inGame;
    i32 m_overlayDrag;
    i32 m_paused;
    i32 m_dragEndNotify;
    i32 m_lastScrollTimeX;
    i32 m_lastScrollTimeY;
    i32 m_stepCountdown;
    i32 m_focusPlayerIndex;
    CGruntzSoundInnerZ* m_savedZonedSound;
    // retail `new CPlay` is push 0x520 (CGruntzMgr::TransitionState @0x8b960);
    // the ctor never touches this tail word.
    i32 m_reserved51c;

    i32 DrawCursorSaveUnder(CDDrawSurfacePair* pair);
    i32 LoadCursorSprites(i32 frame, i32 flag);
    i32 LoadScrollSpeedOptions();
    i32 BuildGruntTypeNameTable(PickupType typeIdx, i32 mode, i32 lightGate, CMulti* finishGate);

    i32 ScanBuildTiles();
    i32 ScanShuffleQuads();
};
SIZE(0x520);
SIZE_UNKNOWN();

i32 ChannelSlots_FindFree();
void ChannelSlots_Set(i32 slot, i32 value);
i32 ChannelSlots_Get(i32 slot);
void ChannelSlots_InitAll();

// Per-world death cause for pit/liquid tiles, set from the AREA%i bank.
extern GruntDeathType g_areaPitDeath;

extern "C" i32 g_playActive;
extern "C" i32 g_profAccA;
extern "C" i32 g_profAccB;
extern "C" i32 g_soundChannelInUse[TINT_COUNT];

extern i32 g_lastLevelNum;
// Per-world death cause a StaticHazard inflicts; copied into the hazard
// object's WWD `Smarts` slot at construction.
extern "C" GruntDeathType g_areaHazardDeath;
extern "C" i32 g_levelBias100;
extern char* g_colorNames[];
extern char* g_difficultyNames[];

void Cmd_ApplyScrollParams(i32 durationMs, i32 jitterX, i32 jitterY, i32 panMinX, i32 panMaxX);
CString GetColorName(i32 colorIdx, i32 upper);
CString GetDifficultyName(i32 diffIdx, i32 upper);

i32 LayerBlitFrame(CDDrawSurfaceMgr* mgr, CImage* img, i32 x, i32 w, i32 one, i32 zero);
void UpdateMgrScroll(CGruntzMgr* pm, CStatusBarMgr* bar, i32 snapFlag);
i32 ShowHudMessage(
    CDDrawSurfaceMgr* sink,
    CString* text,
    RECT* box,
    i32 fontSel,
    i32 b,
    i32 c,
    i32 d,
    i32 e,
    i32 f
);
i32 ShowHudMessageAlt(
    CDDrawSurfaceMgr* sink,
    CString* text,
    RECT* box,
    i32 fontSel,
    i32 b,
    i32 c,
    i32 d,
    i32 e,
    i32 f
);
void Cmd_ResetScroll();
i32 InitializeLevelArea(i32 a);
void ActiveWait(u32 ms);

inline CPlay::~CPlay() {
    CPlay::ReleaseResources();
}

// @early-stop
// retail copy 0x0008c9d0 (emitted by gruntzmgr; pin there).
//
// Residue: each timer/interval pair is zeroed in retail as +0, +8, +4, +0xc
// (0x328, 0x330, 0x32c, 0x334 and eight more groups just like it), inside the
// mem-init run - i.e. BETWEEN the subobject ctor calls, with the EH state byte
// stepping 0..4 around them. A mem-init list cannot produce that: MSVC5 runs
// mem-inits in DECLARATION order and ignores the list order (measured - the
// list below is already written lo/interval/hi/intervalHi and cl still emits
// 0x328, 0x32c, 0x330, 0x334). So each pair is really ONE 16-byte member class
// whose inline ctor body writes m_timeLo, m_intervalLo, m_timeHi,
// m_intervalHi - the same source-order effect that makes CState::CState emit
// its RECTs as left, right, top, bottom. Realizing that class means retyping
// ~147 references across Play.cpp and friends.
inline CPlay::CPlay()
    : m_bootyTimerLo(0),
      m_bootyInterval(0),
      m_bootyTimerHi(0),
      m_bootyIntervalHi(0),
      m_ambientTimerLo(0),
      m_ambientInterval(0),
      m_ambientTimerHi(0),
      m_ambientIntervalHi(0),
      m_syncTimerLo(0),
      m_syncInterval(0),
      m_syncTimerHi(0),
      m_syncIntervalHi(0),
      m_cueTimerLo(0),
      m_cueInterval(0),
      m_cueTimerHi(0),
      m_cueIntervalHi(0),
      m_region0TimerLo(0),
      m_region0Interval(0),
      m_region0TimerHi(0),
      m_region0IntervalHi(0),
      m_region1TimerLo(0),
      m_region1Interval(0),
      m_region1TimerHi(0),
      m_region1IntervalHi(0),
      m_region2TimerLo(0),
      m_region2Interval(0),
      m_region2TimerHi(0),
      m_region2IntervalHi(0),
      m_region3TimerLo(0),
      m_region3Interval(0),
      m_region3TimerHi(0),
      m_region3IntervalHi(0),
      m_snapBaseLo(0),
      m_snapDur(0),
      m_snapBaseHi(0),
      m_snapDurHi(0) {
    m_returnToMenuOnComplete = 0;
    m_completedFinalLevel = 0;
    m_reserved1c8 = 0;
    m_hitTest = NULL;
    m_frameMarker = NULL;
    m_guts = NULL;
    m_beginMarker = NULL;
    m_grid = NULL;
    m_scrollSink = NULL;
    m_reserved2f0 = 0;
    m_packetsRcvd = 0;
    m_packetsSent = 0;
    m_cursorFrame = 0;
    m_levelId = -1;
    m_lightFx = NULL;
    m_gridHasSprite = 0;
    m_snapshotActive = 0;
    m_ambientInitDone = 1;
    m_stepCountdown = 0;
    m_savedZonedSound = NULL;
    m_worldReady = 0;
    m_dragSnapActive = 0;
    m_playerCommandPending = 0;
    m_dragInhibit1 = 0;
    m_dragInhibit2 = 0;
    m_dragInProgress = 0;
    m_dragEndNotify = 0;
}

#endif // SRC_GRUNTZ_CPLAY_H
