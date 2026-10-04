#ifndef GRUNTZ_GRUNTZ_GRUNTZMGR_H
#define GRUNTZ_GRUNTZ_GRUNTZMGR_H

#include <vector>

#include <string>

#include <Ints.h>

#include <DDrawMgr/ColorDepth.h>
#include <Enums.h>
#include <Gruntz/DebugDisplayFlags.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GameStateId.h>
#include <Gruntz/GruntzCommandId.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/TileTriggerLogic.h>

#include <dplay.h>

GZ_ENUM_FORWARD(GameModeId);

GZ_ENUM_FORWARD(WorldInitReportTag);

GZ_ENUM_FORWARD(MovieId);

class CDialog;

#include <Dsndmgr/MidiManager.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GruntzMapMgr.h>
#include <Gruntz/GruntzPlayer.h>
#include <Gruntz/SpriteRefTable.h>
#include <Gruntz/State.h>
#include <Gruntz/StateChange.h>
#include <Runtime/StateTransition.h>
#include <Gruntz/String.h>
#include <Image/CImage.h>
#include <Io/SaveGame.h>
#include <Wap32/Wap32.h>

class CGruntzCmdMgr;

class CFileMemBase;

class SoundCueRegistry;

class CDDrawWorkerRegistry;

class CDDrawWorker;

class CDDrawSurfaceMgr;

struct IDirectPlayLobby;

class CWorldSoundSet;

struct EngObj;
class CFaderMgr;
class CCheatMgr;
class CShadeTableCache;
class CVoiceManager;
class CGameLevel;
class CLightFxMgr;

class CRezMgr;
class Settings;
class CFontConfig;
class CTriggerMgr;
class CPlay;
class CGameStats;

class CGruntzMgr : public CGameMgr, private StateTransitionHost {
public:
    CGruntzMgr();
    virtual ~CGruntzMgr()  ;
    virtual i32 Run(CGameWnd*, char*)  ;
    virtual i32 IsActive()  ;

    virtual void Close()  ;
    void FinalizeLevelAndShowResults();
    void OnCheckpointReached();
    void DelayedQuit();
    bool IsQuitPending() const;
    bool IsSceneFading() const;
    bool IsStateTransitioning() const { return m_stateTransition.active() || m_completingStateChange; }
    void AdvanceStateChange(u32 deltaMs);
    void CancelStateChange();

    i32 LaunchPortal(i32 quitAfter);

    i32 LaunchProcessInDir(const std::string& app, const std::string& directory);

    void Post(i32 code);
    i32 OpenBattlezSetup();
    void ReportError(WPARAM wParam, LPARAM lParam);

    void XorLiveObjectFlags(i32 mask);

    CGruntzMapMgr* GetTileGrid() {
        return m_tileGrid;
    }

    void RegisterLevelAssetKeys();
    char GetGruntzDriveLetter();
    i32 IsInPlayState();

    i32 CaptureWorldFile();

    i32 ShowMessageBox(const char* text, u32 type);

    i32 ClearWorldFile();
    i32 InitializeLobbyConnectionSettings();
    std::string BuildMoviePath(MovieId movie);

    virtual i32 UpdateFrame()  ;

    void RefreshGameClock();
    void HandleAppActivation(b32 active, i32 unused);
    i32 CheckPlayState();
    i32 RestoreVideoMode(b32 save);
    i32 SetVideoMode(i32 w, i32 h, b32 saveMode);

    i32 TryNextResolution();
    i32 TryPreviousResolution();

    i32 ForwardCharToState(i32 charCode, i32 keyData);
    i32 ForwardKeyDownToState(i32 virtualKey, i32 keyData);
    i32 ForwardKeyUpToState(i32 virtualKey, i32 keyData);
    i32 ForwardLButtonDownToState(i32 keyFlags, i32 x, i32 y);
    i32 ForwardLButtonUpToState(i32 keyFlags, i32 x, i32 y);
    i32 ForwardLButtonDblClkToState(i32 keyFlags, i32 x, i32 y);
    i32 ForwardRButtonDownToState(i32 keyFlags, i32 x, i32 y);
    i32 ForwardRButtonUpToState(i32 keyFlags, i32 x, i32 y);
    i32 ForwardRButtonDblClkToState(i32 keyFlags, i32 x, i32 y);
    i32 ForwardMouseMoveToState(i32 keyFlags, i32 x, i32 y);

    CState* TopState();
    void PushState(CState* s);
    i32 PopTopIfMatches(CState* s);
    void ClearStateStack();
    i32 CheckMovieFileExists();
    CState* FindStateById(GameStateId id);

    i32 GoToNextLevel();
    i32 GoToPrevLevel();
    i32 ToggleObjectLayer();
    i32 ToggleHeightLayer();
    i32 ToggleBaseLayer();
    i32 PollUnlessIdle();
    i32 RejectWorldFileCommand();
    i32 AppendChatMessage(const std::string& msg);
    i32 ShowToggleMessage(const std::string& itemName, i32 on);

    i32 IsMoviePathValid();
    void ReportWorldStatus(WorldInitReportTag tag);
    i32 LoadMonologoSprite();
    i32 CheatRevealTreasures();

    i32 SetGruntColor(CDDrawWorker* sink, const std::string& key, i32 idx);
    void CheatSkeletonToggle();
    void CheatEclipseToggle();
    i32 WarpCheat();

    typedef i32(__cdecl* ScanCb)(CGameObject* obj, i32 user);
    i32 ScanObjectsInRadius(i32 x, i32 y, i32 radius, i32 mask, ScanCb cb, i32 user);

    i32 ScanObjectsInRect(i32 offX, i32 offY, RECT* rect, i32 mask, ScanCb cb, i32 user);
    i32 SetColorDepth(ColorDepth depth);
    i32 LoadWorldMode(ColorDepth mode);
    void OnWorldModeLoaded(ColorDepth mode);
    i32 ResetWorldState();
    void PauseMusicIfEnabled();
    void ResumeMusicIfEnabled();

    i32 SetAssetRoot(const std::string& path);

    void MuteMusicIfActive(i32 durationMs);
    void RestoreMusicVolumeIfActive(i32 durationMs);

    i32 RegisterSetSkillDebugCmd();

    i32 Rand();
    i32 RandRange(i32 lo, i32 hi);
    i32 SetVoiceVolume(i32 v);
    void SetSoundVolume(i32 v);

    void StopAudioPlayback();
    void DeactivateAllPlayers();

    std::string GetWorldFileName() {
        return m_strWorldFile;
    }
    i32 AdvanceComputerPlayerTurns();
    i32 InitializeBattlezPlayers();
    void SetCellHeight(i32 x, i32 y, i32 value);
    // Success means accepted; completion and errors are delivered by later callbacks.
    i32 PassClickToPlayState(i32 areaArg, b32 forceTransition, i32 unused,
        const StateChangeOptions& options = StateChangeOptions());
    i32 SwitchToNextState(const StateChangeOptions& options = StateChangeOptions(0x429));

    i32 TransitionState(GameStateId stateId, i32 areaArg, b32 keepCurrent, i32 unused,
        const StateChangeOptions& options = StateChangeOptions());

    void EnterModalUI(const char* msg);

    i32 ExitModalUI(class CDialog* dlg, b32 notify);
    i32 FinishLevel(b32 pauseGame, b32 pauseMusic);
    i32 FillSaveInfo(SaveSlot* dst, const char* snapshot);
    i32 SaveState(CFileMemBase* ar);
    i32 LoadState(CFileMemBase* ar);

    RECT* GetRect(RECT* out);

    i32 ResolveLevelChecksum(
        b32 useDirectLevelReference,
        b32 isBattlez,
        b32 isCustom,
        i32 levelId,
        std::string levelName
    );
    void CommitSinglePlayerProgress();

    i32 SerializeGameState(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, i32 payload);
    void RecomputeViewScale();

    i32 MakeRezPath();

    i32 HandleDebugPosition();

    std::string GetRezPath();

    void SetGameClock(i32 now, i32 delta, i32 abs);
    void ResetClockGlobals();
    i32 TickStateMgrs();
    void SetSoundEnabled(b32 enabled);
    i32 CheckSavedMode();
    i32 IsLobbyHostReady();
    void OnMusicMuteBegin();
    void OnMusicMuteEnd();
    void OnMusicFadeStep(i32 value);
    i32 PlayLogoMovie();

    CPlay* PickPlayOrPausedState();
    CState* PickPausedThenPlayState();

    i32 RunModalDialog(const char* tmpl, DLGPROC dlgProc, b32 notify);

    virtual i32 HandleCommand(i32 notifyCode, GruntzCommandId nID, i32 lParam)  ;

    void SetMusicEnabled(b32 enabled);
    i32 RunLoadGameDialog();
    i32 Quicksave();

    i32 Quickload();
    i32 RunDebugGruntTypeDialog();

    i32 SetInactivePlayerName(
        i32 slot,
        i32 unusedB,
        i32 unusedC,
        i32 unusedD,
        i32 unusedE,
        const std::string& val,
        i32 unusedG
    );
    i32 CountActivePlayers(b32 includeComputerPlayers);

    GruntzPlayer* FindPlayerByNetworkId(i32 networkPlayerId);
    i32 ResetPlayerSlot(i32 slot);
    void ResetAllPlayerSlots();
    i32 IsStandardMode();
    i32 DebugJumpLevel();
    i32 PostSlotCommandB1(i32 slot);
    i32 PostSlotCommandB6(i32 slot);

    i32 LoadSaveMessageSprite();

    i32 IsBattlezMapFile(const std::string& path);

    i32 PlayMovieEntry(i32 entryId);

    i32 LaunchWebBrowser(char* url);

    tagSIZE GetModeSize() {
        return m_modeSize;
    }

    CCheatMgr* CheatMgr() {
        return m_cheatMgr;
    }

    GameModeId GetGameMode() const {
        return m_gameMode;
    }

    CDDrawSurfaceMgr* World() {
        return m_world;
    }

    CVoiceManager* VoiceMgr() {
        return m_voiceManager;
    }

    CRezMgr* ResourceArchive() {
        return m_resourceArchive;
    }

    CFontConfig* ChatLog() {
        return m_chatLog;
    }

    CState* m_curState;
    CDDrawSurfaceMgr* m_world;

    CRezMgr* m_resourceArchive;

    Settings* m_settings;

    CObject* m_reserved3c;
    CFaderMgr* m_faderMgr;
    CCheatMgr* m_cheatMgr;

    MidiManager* m_midi;

    i32 m_reserved4c;
    CShadeTableCache* m_shadeCache;

    CWorldSoundSet* m_worldSounds;

    CSaveGame* m_saveGame;

    CFontConfig* m_chatLog;
    CVoiceManager* m_voiceManager;

    i32 m_reserved64;
    CTriggerMgr* m_triggerMgr;
    CGruntzCmdMgr* m_commandMgr;
    CGruntzMapMgr* m_tileGrid;

    CSpriteRefTable* m_spriteFactory;

    CLightFxMgr* m_lightFxMgr;

    CGameStats* m_gameStats;
    i32 m_numRuns;
    i32 m_numMovies;
    ColorDepth m_colorDepth;
    tagSIZE m_modeSize;
    tagSIZE m_savedModeSize;
    i32 m_lobbyResult;
    b32 m_lobbyProbed;

    i32 m_reserveda8;
    b32 m_modalBusy;
    b32 m_renderGate;

    i32 m_reservedb4;
    b32 m_isCheckpointPrompts;
    SaveSlot* m_saveInfoRec;
    struct IDirectPlayLobby* m_lobby;

    LPDPLCONNECTION m_connSettings;
    std::string m_strWorldFile;

    i32 m_reservedcc;
    char m_driveLetter;
    b32 m_driveLetterProbed;
    std::vector<CState*> m_stateStack;

    std::string m_strRezPath;

    std::string m_strMoviePath;
    b32 m_inGameDir;
    b32 m_haveRez;
    b32 m_haveMoviez;
    b32 m_isVoiceEnabled;
    b32 m_isAmbientEnabled;
    b32 m_isInterlaced;
    b32 m_isHighDetail;
    b32 m_isEffectsEnabled;
    b32 m_loadingSaveGame;
    b32 m_isEasyMode;

    i32 m_soundVolume;

    i32 m_voiceVolume;

    i32 m_scrollSpeed;

    b32 m_isBuiltInBattlezLevel;
    b32 m_isBuiltInMultiplayerLevel;
    b32 m_isCustomLevel;
    GameModeId m_gameMode;
    i32 m_computerPlayerCount;
    RECT m_viewBounds;
    GruntzPlayer m_players[4];
private:
    StateTransition m_stateTransition;
    StateChange m_stateChange;
    bool m_completingStateChange;
    bool QueueStateChange(const StateChange& change);
    virtual bool BeginDeparture();
    virtual TransitionProgress AdvanceDeparture(u32 deltaMs);
    virtual bool InstallDestination();
    virtual TransitionProgress AdvanceInstallation(u32 deltaMs);
    virtual bool BeginArrival();
    virtual TransitionProgress AdvanceArrival(u32 deltaMs);
};

extern i32 g_roundStartTimeMs;

i32 PumpIdleFrame();

extern b32 g_monologoShown;


extern u32 g_gruntDestruction;
extern u32 g_gruntCreation;
extern u32 g_gooPuddlez;
extern u32 g_explosionz;
extern u32 g_resolutionChanged;

extern DebugDisplayFlags g_debugDisplayFlags;

extern i32 g_warpX;
extern i32 g_warpY;

std::string RunCustomWorldDialog(HWND parent);
std::string FindPortalExecutable();

char GetGruntzDriveLetter();
void ResetPlayerColorAvailability();

BOOL CALLBACK SetSkillLevelDialogProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK WarpDialogProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK PsycheDialogProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK JumpLevelDialogProc(HWND, UINT, WPARAM, LPARAM);

#endif
