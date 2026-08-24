#ifndef GRUNTZ_GRUNTZ_CMULTI_H
#define GRUNTZ_GRUNTZ_CMULTI_H

#include <rva.h>

#include <Mfc.h>

#include <Enums.h>
#include <Gruntz/GameStateId.h>
#include <Gruntz/MapMgr.h>
#include <Gruntz/Play.h>
#include <Net/NetMsgId.h>

class CGameApp;
class CTileTriggerContainer;
class MidiManager;
class CFontConfig;
class CChatBoxOwner;
class CWorldSoundSet;
class CNetMgr;
class CNetSessionNode;
struct CNetStatPacket;
struct CNetCtrlMsg;
struct CNetVersionMsg;
struct CNetChannelPacket;
struct CNetOneChannelPacket;
struct CNetChannelTablePacket;
struct CNetConfigBlob;
struct MenuSelectEvent;
class GruntzPlayer;
struct CNetSession;

struct CNetSessionDesc;

struct CNetLobbyName {
    u32 m_dwSize;
    u32 m_dwFlags;
    char* m_shortName;
    char* m_longName;
};

struct CNetLobbyConnection {
    u32 m_dwSize;

    u32 m_dwFlags;
    CNetSessionDesc* m_sessionDesc;
    CNetLobbyName* m_playerName;
};

// HWND is void* here on purpose: HeapDiag.cpp is a <Win32.h> TU, where
// windows.h leaves STRICT off and HWND *is* void*, so its definition mangles
// PAX.  Spelling HWND in this MFC (STRICT) header would emit PAUHWND__ and
// resolve to nothing.
void SetActiveAndFocus(void* hWnd);
void FillPlayerList(HWND hList, CNetMgr* session);
BOOL CALLBACK MultiJoinDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

BOOL CALLBACK NetSetupDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

class CNetPlayerListNode;

struct InterfaceObject;

class CMulti;

class CMulti : public CPlay {
public:
    // inline: retail expands this whole ctor into CGruntzMgr::TransitionState
    CMulti() {
        m_session = NULL;
        m_netGate = NULL;
        m_savedEffectsEnabled = 1;
        m_customLevel = 0;
        m_autoCommandDelay = 1;
    }
    virtual ~CMulti() OVERRIDE;

    virtual i32 LoadGameAssetNamespaces(CGruntzMgr* mgr, i32 areaArg, i32 prevStateId) OVERRIDE;

    virtual void ReleaseResources() OVERRIDE;
    virtual GameStateId Update() OVERRIDE;

    virtual i32 Render() OVERRIDE;
    virtual i32 EnterState(GameStateId) OVERRIDE;
    virtual i32 LeaveState(GameStateId) OVERRIDE;
    virtual i32 OnChar(i32 charCode, i32 keyData) OVERRIDE;
    virtual i32 CompleteLevel() OVERRIDE;
    virtual i32 UnusedPlayQuery() OVERRIDE;
    virtual i32 GetFrame() OVERRIDE;

    virtual i32 LoadByMode(i32 mode, i32 unused) OVERRIDE;

    virtual void OnExit() OVERRIDE;
    virtual void TickStateMgrs() OVERRIDE;

    CGruntzMgr* Mgr() {
        return m_mgr;
    }

    CNetMgr* Peer() {
        return m_netGate;
    }

    void AppendEditLine(HWND edit, char* str);

    CNetSessionNode* LocalPlayer() {
        return m_localPlayer;
    }

    CGruntzMgr* NetGameMgr() {
        return m_mgr;
    }

    i32& ResyncLParam() {
        return m_levelIndex;
    }

    CNetSession* Session() {
        return m_session;
    }

    CString GetString59c();
    RVA(0x000b7ad0, 0x23)
    CString GetString5a0() {
        return m_hostName;
    }

    RVA(0x000b6090, 0x23)
    CString GetConfigNameA() {
        return m_builtInLevelName;
    }
    RVA(0x000b60d0, 0x23)
    CString GetConfigNameB() {
        return m_customLevelName;
    }
    i32 GetCommandDelay();
    i32 GetResendDelay();
    void ReportVersionMsg(char* msg, i32 code);

    void ReportStatusId(u32 strId, i32 level);
    void ReportNetError(i32 level);
    i32 JoinSession();
    i32 RunErrorDialog(char* tmpl, DLGPROC handler, i32 lparam);
    void AckJoinFailure();

    i32 Connect(i32 mode);
    i32 StartTitle();
    void DropTimeout();

    i32 OpenHostChannel(
        void* hostToken,
        const char* name,
        i32 channelId,
        i32 cmdDelay,
        i32 resend,
        i32 unused6,
        i32 unused7,
        i32 unused8
    );

    void SendStatFlag(NetMsgId code, i32 flag);

    void SendNetStat(NetMsgId id, u32 value, i32 flag);

    i32 DropChannelPlayer(i32 idx);
    i32 Poll(i32 token);
    i32 ResolveLocalPlayer();
    void ReportAckLatency();
    i32 VerifyCustomLevel(CNetPlayerListNode* h, CNetSessionNode* token);
    i32 PollSession();
    i32 AutoTuneCmdDelay();

    void OnDropPlayer();

    i32 BroadcastChannelTable(CNetSessionNode* recipient);
    i32 BroadcastOneChannel(GruntzPlayer* ch);

    i32 RegisterChannelFrom(const char* name, ColorTint color, i32 e, i32 f);

    i32 BroadcastChatLine(char* text, i32 toChat, i32 showWnd, HWND hWnd);
    i32 ReadGroupSel();
    i32 PumpA();
    void PumpB();
    void OnOutOfSync();

    i32 Open();
    void Close();
    InterfaceObject* SetupServices();
    i32 DetectConnectionConfig();
    void ApplyCmdDelayDefaults();

    i32 ShowMultiStartDlg();
    CNetPlayerListNode* JoinAndRegisterChannel();
    i32 OnJoinConfirm(HWND hDlg);

    i32 PollSessionGated(i32 sessionGate, i32 pollGate);
    i32 SendStatBuf(CNetStatPacket* pkt, i32 flag);
    i32 SendStatFrom(void* pkt, i32 b, i32 c);
    i32 SendStatPair(CNetSessionNode* recipient, CNetStatPacket* pkt, i32 c);
    i32 SendStatTo(CNetSessionNode* recipient, NetMsgId id, i32 c);
    i32 SendStat3(i32 id, NetMsgId statId, i32 flag);
    i32 SendNetStatTo(CNetSessionNode* recipient, i32 id, u32 value, i32 c);
    i32 SendStatPairRaw(CNetSessionNode* recipient, void* pkt, i32 size, i32 c);
    i32 SendStatValue(i32 id, NetMsgId statId, i32 value, i32 flag);
    i32 DispatchRecvMsg(i32 sender, char* buf, i32 size);
    i32 HandleControlMsg(CNetCtrlMsg* msg, i32 unused);
    i32 OnPlayerLeft(i32 playerId);
    void AckDropPlayer(i32 id);
    void WriteTag(const char*);

    void RecordDropPlayer2(CNetSessionNode* a, i32 id);
    i32 WaitForOtherPlayers();
    i32 LoadMenuSelectSprite(MenuSelectEvent* evp);
    i32 ParseChannelTable(CNetChannelTablePacket* packet);
    i32 RegisterChannel(const char* name, ColorTint color, i32 c, i32 d, i32 idx, i32 e);
    i32 RegisterChannelRec(CNetChannelPacket* rec);
    i32 RemoveChannel(i32 idx);
    i32 OnPauseChannel();
    void OnMultiPause();
    void OnMultiOptions();
    i32 ParseOneChannel(CNetOneChannelPacket* rec);
    i32 SendChannelStat422();
    i32 SendChannelStat423();
    i32 CreateSession();
    u32 FrameSyncWait();
    i32 SetupTcpIpConfig();
    i32 CreateLocalPlayer();
    i32 WaitForConnect();
    i32 SaveConfig(CNetSessionNode* recipient);
    i32 LoadConfig(CNetConfigBlob* cfg);
    i32 ResetPlayerCommands(i32 id);
    u32 GetMaxAckLatency();
    void HandleVersionCheck(CNetVersionMsg* msg);
    void AnnounceVersion(CNetSessionNode* param);

    void ApplyDynSetting(CString s);
    void SetServiceName(CString s);

    CNetSession* m_session;
    CNetMgr* m_netGate;
    i32 m_isHost;
    i32 m_sessionTerminated;
    i32 m_customLevelVerificationPending;
    i32 m_allPlayersReady;
    i32 m_removedByHost;
    i32 m_levelVerifyResult;
    i32 m_verifyDone;
    i32 m_recordAcked[4];
    i32 m_recordToken[4];
    i32 m_pollAbort;
    i32 m_colorSelectionRejected;
    i32 m_gameFull;
    i32 m_versionMismatch;
    i32 m_outOfSync;
    i32 m_syncGate;
    i32 m_pumpGuard;
    i32 m_connected;
    i32 m_waitDialogReplyReceived;
    i32 m_lobbyLaunch;
    i32 m_connectAccepted;
    i32 m_savedEffectsEnabled;
    i32 m_roundComplete;
    CString m_providerConfigPrefix;
    CString m_groupName;
    CString m_hostName;
    i32 m_commandDelay;
    i32 m_drainReload;
    i32 m_gameClosed;
    i32 m_customLevel;
    CString m_builtInLevelName;
    CString m_customLevelName;
    CNetSessionNode* m_localPlayer;
    i32 m_hostIndex;
    i32 m_lastSenderId;
    char _p5c4[0x5cc - 0x5c8];
    i32 m_curSlotId;
    i32 m_reserved5d0;
    i32 m_drainTimer;
    i32 m_frameDelta;
    i32 m_lastTime;
    u32 m_accumTime;
    i32 m_lastFrameSyncTime;
    i32 m_reserved5e8;
    i32 m_reserved5ec;
    i32 m_channelLatency[4];
    i32 m_autoCommandDelay;

    CDWordArray m_readyPlayerIds;

    char m_pad618[0x660 - 0x618];
};

extern CMulti* g_multiState;
extern CString g_sessionName;
extern i32 g_optionsCursor;

extern CNetMgr* g_groupEnumMgr;

extern i32 g_hostServicesMode;

extern HWND g_netPlayerListHwnd;

void MultiJoinHandler();

class CFile;

extern i32 g_serviceId;

extern CMulti* g_connectRptMgr;
void RefreshPlayerRow(HWND hDlg, HWND hList);

extern HWND g_sharedFlag;

extern char s_GameKey[];
extern u32 g_ackThrottleDeadline;

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

#endif // GRUNTZ_GRUNTZ_CMULTI_H
