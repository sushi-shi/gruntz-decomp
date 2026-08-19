#ifndef GRUNTZ_GRUNTZ_GRUNTZMGR_H
#define GRUNTZ_GRUNTZ_GRUNTZMGR_H

#include <rva.h>

#include <DDrawMgr/ColorDepth.h>
#include <Enums.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GameStateId.h>
#include <Gruntz/GruntzCommandId.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/TileTriggerLogic.h>

GZ_ENUM_FORWARD(GameModeId);

GZ_ENUM_FORWARD(WorldInitReportTag);

GZ_ENUM_FORWARD(MovieId);

class CDialog;

#include <Dsndmgr/GruntzSoundZ.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GruntzMapMgr.h>
#include <Gruntz/GruntzPlayer.h>
#include <Gruntz/SpriteRefTable.h>
#include <Gruntz/State.h>
#include <Gruntz/String.h>
#include <Image/CImage.h>
#include <Io/SaveGame.h>
#include <Wap32/Wap32.h>

class CGruntzCmdMgr;

class CFileMemBase;

class CDDrawSubMgrLeafScan;

class CDDrawWorkerRegistry;

class CDDrawWorker;

class CDDrawSurfaceMgr;

struct IDirectPlayLobby;
struct CNetLobbyConnection;

class CWorldSoundSet;

struct EngObj;
class CFaderMgr;
class CCheatMgr;
class CShadeTableCache;
class CGruntSpawnConfig;
class CGameLevel;
class CLightFxMgr;

class CSymParser;
namespace Utils {
    class RegistryHelper;
}
class CFontConfig;
class CTriggerMgr;
class CPlay;
class CBattlezData;

class CGruntzMgr : public CGameMgr {
public:
    CGruntzMgr();
    virtual ~CGruntzMgr() OVERRIDE;
    virtual i32 Run(CGameWnd*, char*) OVERRIDE;
    virtual i32 IsActive() OVERRIDE;

    virtual void Close() OVERRIDE;
    void AccrueScoreTime();
    void OnCheckpointReached();
    void DelayedQuit();

    i32 LaunchPortal(i32 quitAfter);

    i32 LaunchProcessInDir(char* exe, char* dir);

    void Post(i32 code);
    i32 SaveGameAs();
    // wParam is an ErrorStringId (a string-table resource id - ShowError feeds
    // it to LoadStringA); lParam is a bare per-call-site tag that ShowError
    // prints verbatim as "(%i)", so it has no domain. See ErrorStringId.h for
    // the CMD_*/IDS_* conflation the call sites currently carry.
    void ReportError(WPARAM wParam, LPARAM lParam);

    void XorLiveObjectFlags(i32 mask);

    // TmDeflectStep is the only retail caller of the out-of-line COMDAT.
    RVA(0x00075ad0, 0x4)
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
    CString BuildMoviePath(MovieId movie);

    virtual i32 PerFrameTick() OVERRIDE;

    void RefreshGameClock();
    void AdvanceFrame(i32 doDraw, i32 unused);
    i32 CheckPlayState();
    i32 RestoreVideoMode(i32 save);
    i32 SetVideoMode(i32 w, i32 h, i32 flag);

    i32 CheckDisplayBoundsA();
    i32 CheckDisplayBoundsB();

    i32 ForwardCharToState(i32 a, i32 b);
    i32 ForwardKeyDownToState(i32 a, i32 b);
    i32 ForwardKeyUpToState(i32 a, i32 b);
    i32 ForwardLButtonDownToState(i32 a, i32 b, i32 c);
    i32 ForwardLButtonUpToState(i32 a, i32 b, i32 c);
    i32 ForwardLButtonDblClkToState(i32 a, i32 b, i32 c);
    i32 ForwardRButtonDownToState(i32 a, i32 b, i32 c);
    i32 ForwardRButtonUpToState(i32 a, i32 b, i32 c);
    i32 ForwardRButtonDblClkToState(i32 a, i32 b, i32 c);
    i32 ForwardMouseMoveToState(i32 a, i32 b, i32 c);

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
    i32 AppendChatMessage(char* msg);
    i32 ShowToggleMessage(char* itemName, i32 on);

    i32 IsMoviePathValid();
    void ReportWorldStatus(WorldInitReportTag tag);
    i32 LoadMonologoSprite();
    i32 CheatRevealTreasures();

    i32 SetGruntColor(CDDrawWorker* sink, const char* key, i32 idx);
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
    void StopBankIfActive();
    void StopBank0IfActive();

    i32 SetAssetRoot(char* path);

    void MuteMusicIfActive(i32 ms);
    void RestoreMusicVolumeIfActive(i32 ms);

    i32 RegisterSetSkillDebugCmd();

    i32 Rand();
    i32 RandRange(i32 lo, i32 hi);
    i32 SetVoiceVolume(i32 v);
    void SetSoundVolume(i32 v);

    void UnloadSoundChain();
    void ClearOptionsSlots();
    RVA(0x000928c0, 0x23)
    CString GetWorldFileName() {
        return m_strWorldFile;
    }
    i32 AdvanceOptionsCycle();
    i32 SyncOptionsState();
    void SetCellHeight(i32 r, i32 c, i32 v);
    i32 PassClickToPlayState(i32 areaArg, i32 forceTransition, i32 unused);
    i32 SwitchToNextState();

    i32 TransitionState(GameStateId stateId, i32 areaArg, i32 keepCurrent, i32 unused);

    void EnterModalUI(const char* msg);

    i32 ExitModalUI(class CDialog* dlg, i32 notify);
    i32 FinishLevel(i32 full, i32 stopBank);
    i32 FillSaveInfo(SaveSlot* dst, const char* snapshot);
    i32 SaveState(CFileMemBase* ar);
    i32 LoadState(CFileMemBase* ar);

    RECT* GetRect(RECT* out);

    i32 BuildLevelRezPath(i32 isEmpty, i32 hi, i32 lo, i32 id, CString name);
    void UpdateScoreHud();

    i32 BroadcastCmd(CFileMemBase* ar, SerialMode cmd, LogicTypeId typeId, i32 pObj);
    void RecomputeViewScale();

    i32 MakeRezPath();

    i32 HandleDebugPosition();

    CString GetRezPath();

    void SetGameClock(i32 now, i32 delta, i32 abs);
    void ResetClockGlobals();
    i32 TickStateMgrs();
    void SetRunState(i32 v);
    i32 CheckSavedMode();
    i32 IsLobbyHostReady();
    void OnMusicMuteBegin();
    void OnMusicMuteEnd();
    void OnMusicFadeStep(i32 value);
    i32 RunFromState();

    CPlay* PickPlayOrPausedState();
    CState* PickPausedThenPlayState();

    i32 RunModalDialog(const char* tmpl, DLGPROC dlgProc, i32 flag);

    virtual i32 HandleCommand(i32 notifyCode, GruntzCommandId nID, i32 lParam) OVERRIDE;

    void SetSoundLevelState(i32 loaded);
    i32 RunLoadGameDialog();
    i32 Quicksave();

    i32 Quickload();
    i32 RunDebugGruntTypeDialog();

    i32 LoadOptionsSlotName(
        i32 slot,
        i32 unusedB,
        i32 unusedC,
        i32 unusedD,
        i32 unusedE,
        const CString& val,
        i32 unusedG
    );
    i32 CountReadyOptionsSlots(i32 anyState);

    GruntzPlayer* FindOptionsSlot(i32 x);
    i32 ResetOptionsSlot(i32 idx);
    void ResetAllOptionsSlots();
    i32 IsStandardMode();
    i32 DebugJumpLevel();
    i32 PostSlotCommandB1(i32 slot);
    i32 PostSlotCommandB6(i32 slot);

    i32 LoadSaveMessageSprite();

    i32 IsBattlezMapFile(CString path);

    i32 ChangeState(i32 arg);

    // A member: HandleCommand's only call site materialises `this` in ecx
    // (`push <url> / mov ecx,esi / call`), which is the __thiscall sequence; the body
    // never reads `this`, and __thiscall with one stack argument returns `ret 4` just
    // as the __stdcall spelling did.
    i32 LaunchWebBrowser(char* url);

    // Retail reproduces the whole tagSIZE at every `.cx`/`.cy` use - an 8-byte frame
    // temp whose unread half is a dead store - so the size arrives by value.
    tagSIZE GetModeSize() {
        return m_modeSize;
    }

    // Retail reaches m_cheatMgr through this accessor, not the member: cl emits it
    // out of line as the 4-byte COMDAT `mov eax,[ecx+0x44]; ret` (0x20f20, won by
    // chatboxowner) and CChatBoxOwner::ProcessCheatInput calls it through the ILT
    // thunk 0x167c. One call site in the whole game.
    RVA(0x00020f20, 0x4)
    CCheatMgr* CheatMgr() {
        return m_cheatMgr;
    }

    CState* m_curState;
    CDDrawSurfaceMgr* m_world;

    CSymParser* m_symParser;

    Utils::RegistryHelper* m_settings;

    // @identity-TODO
    // Current source only zeroes and deletes this slot; a retail write or allocation
    // site is needed to prove its concrete type.
    CObject* m_reserved3c; // owned slot; deleted in teardown, never allocated
    CFaderMgr* m_faderMgr;
    CCheatMgr* m_cheatMgr;

    CGruntzSoundZ* m_sound;
    i32 m_reserved4c;
    CShadeTableCache* m_shadeCache;

    CWorldSoundSet* m_inputState;

    CSaveGame* m_saveSink;

    CFontConfig* m_chatLog;
    CGruntSpawnConfig* m_cueSink;

    i32 m_reserved64;
    CTriggerMgr* m_cmdGrid;
    CGruntzCmdMgr* m_cmdSubMgr;
    CGruntzMapMgr* m_tileGrid;

    CSpriteRefTable* m_spriteFactory;

    CLightFxMgr* m_logicPump;

    CBattlezData* m_scoreHud;
    i32 m_numRuns;
    i32 m_numMovies;
    ColorDepth m_colorDepth;
    tagSIZE m_modeSize;
    tagSIZE m_savedModeSize;
    i32 m_lobbyResult;
    i32 m_lobbyProbed;
    i32 m_delayedQuitPending;
    i32 m_reserveda8;
    i32 m_modalBusy;
    i32 m_renderGate;

    i32 m_reservedb4;
    i32 m_isCheckpointPrompts;
    SaveSlot* m_saveInfoRec;
    struct IDirectPlayLobby* m_lobby;

    CNetLobbyConnection* m_connSettings;
    CString m_strWorldFile;
    i32 m_reservedcc;
    char m_driveLetter;
    char m_padD1[3];
    i32 m_driveLetterProbed;
    CPtrArray m_stateStack;

    CString m_strRezPath;

    CString m_strMoviePath;
    i32 m_inGameDir;
    i32 m_haveRez;
    i32 m_haveMoviez;
    i32 m_isVoiceEnabled;
    i32 m_isAmbientEnabled;
    i32 m_isInterlaced;
    i32 m_isHighDetail;
    i32 m_isEffectsEnabled;
    i32 m_loadingSaveGame;
    i32 m_isEasyMode;

    i32 m_soundVolume;

    i32 m_voiceVolume;

    i32 m_scrollSpeed;

    i32 m_isBattlezLevel;
    i32 m_isMultiLevel;
    i32 m_isCustomLevel;
    GameModeId m_gameMode;
    i32 m_optionsCount;
    RECT m_viewBounds;
    char m_pad14c[0x150 - 0x14c];
    GruntzPlayer m_options[4];
};

extern i32 g_scoreTimeBase;

i32 PumpIdleFrame();

extern i32 g_monologoShown;

extern char g_msgScratch[256];

extern u32 g_gruntDestruction;
extern u32 g_gruntCreation;
extern u32 g_gooPuddlez;
extern u32 g_explosionz;
extern u32 g_resolutionChanged;

extern i32 g_debugDisplayFlags;

extern i32 g_warpX;
extern i32 g_warpY;

CString RunCustomWorldDialog(HWND parent, CString* out);
i32 FindProcessByName(const char* name, i32 flag, HANDLE* out);
i32 __stdcall LaunchPortalExe(char* outPath);

char GetGruntzDriveLetter();
i32 FileExists(const char* szPath);
void ChannelSlots_InitAll();

BOOL CALLBACK SetSkillLevelDialogProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK WarpDialogProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK PsycheDialogProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK JumpLevelDialogProc(HWND, UINT, WPARAM, LPARAM);

#endif // GRUNTZ_GRUNTZ_GRUNTZMGR_H
