#ifndef GRUNTZ_GRUNTZ_CMULTI_H
#define GRUNTZ_GRUNTZ_CMULTI_H

#include <rva.h>

#include <Mfc.h>

#include <Enums.h>
#include <Gruntz/BattlezDifficulty.h>
#include <Gruntz/GameStateId.h>
#include <Gruntz/MapMgr.h>
#include <Gruntz/Play.h>
#include <Net/NetMsgId.h>

#include <dplay.h>

class CGameApp;
class CTileTriggerContainer;
class MidiManager;
class CFontConfig;
class CChatBoxOwner;
class CWorldSoundSet;
class CNetMgr;
class CNetPlayerNode;
struct CNetValuePacket;
struct CNetVersionPacket;
struct CNetPlayerRegistrationPacket;
struct CNetPlayerUpdatePacket;
struct CNetPlayerTablePacket;
struct CNetGameConfigPacket;
class GruntzPlayer;
struct CNetSession;

// HWND is void* here on purpose: HeapDiag.cpp is a <Win32.h> TU, where
// windows.h leaves STRICT off and HWND *is* void*, so its definition mangles
// PAX.  Spelling HWND in this MFC (STRICT) header would emit PAUHWND__ and
// resolve to nothing.
void SetActiveAndFocus(void* hWnd);
void FillSessionList(HWND hList, CNetMgr* manager);
BOOL CALLBACK MultiJoinDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

BOOL CALLBACK NetSetupDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

class CNetSessionListNode;

struct CNetProviderNode;

class CMulti;

class CMulti : public CPlay {
public:
    // inline: retail expands this whole ctor into CGruntzMgr::TransitionState
    CMulti() {
        m_session = NULL;
        m_netMgr = NULL;
        m_savedEffectsEnabled = 1;
        m_usesCustomLevel = 0;
        m_autoCommandDelay = 1;
    }
    virtual ~CMulti() OVERRIDE;

    virtual i32 LoadGameAssetNamespaces(CGruntzMgr* mgr, i32 areaArg, i32 prevStateId) OVERRIDE;

    virtual void ReleaseResources() OVERRIDE;
    virtual GameStateId Update() OVERRIDE;

    virtual i32 Render() OVERRIDE;
    virtual i32 EnterState(GameStateId previousState) OVERRIDE;
    virtual i32 LeaveState(GameStateId nextState) OVERRIDE;
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

    CNetMgr* Network() {
        return m_netMgr;
    }

    void AppendEditLine(HWND edit, char* str);

    CNetPlayerNode* LocalPlayer() {
        return m_localPlayer;
    }

    CGruntzMgr* NetGameMgr() {
        return m_mgr;
    }

    i32& SelectedLevelIndex() {
        return m_levelIndex;
    }

    CNetSession* Session() {
        return m_session;
    }

    CString GameName();
    RVA(0x000b7ad0, 0x23)
    CString PlayerName() {
        return m_playerName;
    }

    RVA(0x000b6090, 0x23)
    CString BuiltInLevelName() {
        return m_builtInLevelName;
    }
    RVA(0x000b60d0, 0x23)
    CString CustomLevelName() {
        return m_customLevelName;
    }
    i32 GetCommandDelay();
    i32 GetResendDelay();
    void ReportVersionMsg(char* msg, i32 code);

    void ReportStatusId(u32 strId, i32 level);
    void ReportNetError(i32 level);
    i32 JoinSession();
    i32 RunErrorDialog(char* tmpl, DLGPROC handler, i32 lparam);
    void SendLobbyKeepAlive();

    i32 Connect(i32 mode);
    i32 StartTitle();
    void CheckDropTimeout();

    i32 CreateHostPlayer(
        void* hostToken,
        const char* name,
        ColorTint color,
        i32 cmdDelay,
        i32 resend,
        i32 unused6,
        i32 unused7,
        i32 unused8
    );

    void BroadcastPlayerIdMessage(NetMsgId code, i32 flag);

    void BroadcastValueMessage(NetMsgId id, u32 value, i32 flag);

    i32 DropLobbyPlayer(i32 slotIndex);
    i32 Poll(i32 token);
    i32 ResolveLocalPlayer();
    void ReportMaxAckLatency();
    i32 VerifyCustomLevel(CNetSessionListNode* session, CNetPlayerNode* localPlayer);
    i32 PollSession();
    i32 AutoTuneCmdDelay();

    void ShowDropPlayerDialog();

    i32 BroadcastPlayerTable(CNetPlayerNode* recipient);
    i32 BroadcastPlayerUpdate(GruntzPlayer* player);

    i32 RegisterLocalPlayer(
        const char* name,
        ColorTint color,
        i32 preferredPlayerIndex,
        i32 networkPlayerId
    );

    i32 BroadcastChatLine(char* text, i32 prefixPlayerName, i32 echoLocally, HWND edit);
    i32 ReadGroupSel();
    i32 AdvanceGameFrame();
    void RenderGameFrame();
    void OnOutOfSync();

    i32 Open();
    void Close();
    CNetProviderNode* SelectNetworkProvider();
    i32 DetectConnectionConfig();
    void ApplyCmdDelayDefaults();

    i32 ShowMultiStartDlg();
    CNetSessionListNode* CreateHostSessionAndPlayer();
    i32 OnJoinConfirm(HWND hDlg);

    i32 PollSessionGated(i32 sessionGate, i32 pollGate);
    i32 BroadcastValuePacket(CNetValuePacket* packet, i32 flags);
    i32 BroadcastPacket(void* packet, i32 packetSize, i32 flags);
    i32 SendValuePacketTo(CNetPlayerNode* recipient, CNetValuePacket* packet, i32 flags);
    i32 SendPlayerIdMessageTo(CNetPlayerNode* recipient, NetMsgId messageId, i32 flags);
    i32 SendPlayerIdMessageToId(i32 recipientId, NetMsgId messageId, i32 flags);
    i32 SendValueMessageTo(CNetPlayerNode* recipient, i32 messageId, u32 value, i32 flags);
    i32 SendPacketTo(CNetPlayerNode* recipient, void* packet, i32 packetSize, i32 flags);
    i32 SendValueMessageToId(i32 recipientId, NetMsgId messageId, i32 value, i32 flags);
    i32 DispatchRecvMsg(i32 senderId, char* packet, i32 packetSize);
    i32 HandleSystemMessage(LPDPMSG_GENERIC message, i32 unusedMessageSize);
    i32 OnPlayerLeft(i32 playerId);
    void ApplyPlayerDrop(i32 playerId);
    void WriteTag(const char*);

    void RecordPlayerReady(CNetPlayerNode* unusedPlayer, i32 playerId);
    i32 WaitForOtherPlayers();
    i32 HandlePlayerCreated(LPDPMSG_CREATEPLAYERORGROUP message);
    i32 ApplyPlayerTable(CNetPlayerTablePacket* packet);
    i32 RegisterPlayer(
        const char* name,
        ColorTint color,
        i32 humanControlled,
        BattlezDifficulty difficulty,
        i32 preferredPlayerIndex,
        i32 networkPlayerId
    );
    i32 RegisterPlayerFromPacket(CNetPlayerRegistrationPacket* packet);
    i32 DeactivatePlayer(i32 slotIndex);
    i32 RequestMultiplayerPause();
    void ShowMultiplayerPauseDialog();
    void ShowMultiplayerOptionsDialog();
    i32 ApplyPlayerUpdate(CNetPlayerUpdatePacket* packet);
    i32 AnnounceOptionsOpened();
    i32 AnnounceOptionsClosed();
    i32 CreateSession();
    u32 FrameSyncWait();
    i32 SetupTcpIpConfig();
    i32 CreateLocalPlayer();
    i32 WaitForConnect();
    i32 SendGameConfig(CNetPlayerNode* recipient);
    i32 ApplyGameConfig(CNetGameConfigPacket* config);
    i32 ResetPlayerCommands(i32 playerId);
    u32 GetMaxAckLatency();
    void HandleVersionCheck(CNetVersionPacket* packet);
    void SendVersionCheck(CNetPlayerNode* recipient);

    void SetGameName(CString s);
    void SetPlayerName(CString s);

    CNetSession* m_session;
    CNetMgr* m_netMgr;
    i32 m_isHost;
    i32 m_sessionTerminated;
    i32 m_customLevelVerificationPending;
    i32 m_allPlayersReady;
    i32 m_removedByHost;
    i32 m_levelVerifyResult;
    i32 m_verifyDone;
    i32 m_levelChecksumReceived[4];
    i32 m_levelChecksums[4];
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
    CString m_gameName;
    CString m_playerName;
    i32 m_commandDelay;
    i32 m_resendInterval;
    i32 m_gameClosed;
    i32 m_usesCustomLevel;
    CString m_builtInLevelName;
    CString m_customLevelName;
    CNetPlayerNode* m_localPlayer;
    i32 m_localPlayerId;
    i32 m_lastSenderId;
    char _p5c4[0x5cc - 0x5c8];
    i32 m_processedCommandTick;
    i32 m_reserved5d0;
    i32 m_drainTimer;
    i32 m_frameDelta;
    i32 m_lastTime;
    u32 m_accumTime;
    i32 m_lastFrameSyncTime;
    i32 m_reserved5e8;
    i32 m_reserved5ec;
    i32 m_playerLatencyMs[4];
    i32 m_autoCommandDelay;

    CDWordArray m_readyPlayerIds;

    char m_pad618[0x660 - 0x618];
};

extern CMulti* g_multiState;
extern CString g_sessionName;
extern i32 g_battlezTurnPlayerIndex;

extern CNetMgr* g_netMgr;

extern i32 g_hostServicesMode;

extern HWND g_sessionListHwnd;

void MultiJoinHandler();

class CFile;

extern i32 g_serviceId;

extern CMulti* g_connectRptMgr;
void RefreshSessionSelection(HWND hDlg, HWND hList);

extern HWND g_netMessageEditHwnd;

extern char s_GameKey[];
extern u32 g_ackThrottleDeadline;

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

#endif // GRUNTZ_GRUNTZ_CMULTI_H
