#ifndef GRUNTZ_GRUNTZ_CMULTI_H
#define GRUNTZ_GRUNTZ_CMULTI_H

#include <rva.h>
#include <Gruntz/Play.h>
#include <Mfc.h>
#include <Gruntz/MapMgr.h>

class CGameApp;
class CTileTriggerContainer;
class CGruntzSoundZ;
class CFontConfig;
class CChatBoxOwner;
class CWorldSoundSet;
class CNetMgr;
class CNetSessionNode;
struct CNetStatPacket;
struct CNetCtrlMsg;
struct CNetVersionMsg;
class GruntzPlayer;
struct CNetSession;

class CNetSessionDesc;

struct CNetLobbyName {
    u32 m_dwSize;
    u32 m_dwFlags;
    char* m_shortName;
    char* m_longName;
};
SIZE(0x10);

struct CNetLobbyConnection {
    u32 m_dwSize;

    u32 m_dwFlags;
    CNetSessionDesc* m_sessionDesc;
    CNetLobbyName* m_playerName;
};
SIZE_UNKNOWN();

void SetActiveAndFocus(void* hwnd);
void FillPlayerList(HWND hList, CNetMgr* session);
INT_PTR CALLBACK MultiJoinDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

INT_PTR CALLBACK NetSetupDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

class CNetPlayerListNode;

struct InterfaceObject;

class CMulti;

class CMulti : public CPlay {
public:
    CMulti();
    virtual ~CMulti() OVERRIDE;

    virtual i32 LoadGameAssetNamespaces(CGruntzMgr* mgr, i32 areaArg, i32 prevStateId) OVERRIDE;

    virtual void ReleaseResources() OVERRIDE;
    virtual GameStateId Update() OVERRIDE;

    virtual i32 Render() OVERRIDE;
    virtual i32 Vslot09(i32) OVERRIDE;
    virtual i32 FrameSlot28(i32) OVERRIDE;
    virtual i32 Vslot0b(i32, i32) OVERRIDE;
    virtual i32 Vslot15() OVERRIDE;
    virtual i32 Vslot1a() OVERRIDE;
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

    CNetSessionNode* LocalPlayer() {
        return m_5bc;
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
        return m_5b4;
    }
    RVA(0x000b60d0, 0x23)
    CString GetConfigNameB() {
        return m_5b8;
    }
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
        void* a0,
        const char* name,
        i32 channelId,
        i32 cmdDelay,
        i32 resend,
        i32 unused6,
        i32 unused7,
        i32 unused8
    );

    void SendStatFlag(i32 code, i32 flag);

    void SendNetStat(i32 id, u32 value, i32 flag);

    i32 DropChannelPlayer(i32 idx);
    i32 Poll(i32 token);
    i32 ResolveLocalPlayer();
    void ReportAckLatency();
    i32 VerifyCustomLevel(void* h, CNetSessionNode* token);
    i32 PollSession();
    i32 AutoTuneCmdDelay();

    void OnDropPlayer();

    i32 BroadcastChannelTable(CNetSessionNode* recipient);
    i32 BroadcastOneChannel(GruntzPlayer* ch);

    i32 RegisterChannelFrom(const char* name, i32 b, i32 e, i32 f);

    i32 BroadcastChatLine(char* text, i32 toChat, i32 showWnd, void* hWnd);
    i32 ReadGroupSel();
    i32 PumpA();
    void PumpB();
    void OnOutOfSync();

    i32 Open();
    InterfaceObject* SetupServices();
    i32 DetectConnectionConfig();
    void ApplyCmdDelayDefaults();

    i32 ShowMultiStartDlg();
    CNetPlayerListNode* JoinAndRegisterChannel();
    i32 OnJoinConfirm(void* hDlg);

    i32 PollSessionGated(i32 a1, i32 a2);
    i32 SendStatBuf(CNetStatPacket* pkt, i32 flag);
    i32 SendStatFrom(void* pkt, i32 b, i32 c);
    i32 SendStatPair(CNetSessionNode* recipient, CNetStatPacket* pkt, i32 c);
    i32 SendStatTo(CNetSessionNode* recipient, i32 id, i32 c);
    i32 SendStat3(i32 id, u32 value, i32 flag);
    i32 SendNetStatTo(CNetSessionNode* recipient, i32 id, u32 value, i32 c);
    i32 SendStatPairRaw(CNetSessionNode* recipient, void* pkt, i32 size, i32 c);
    i32 SendStatValue(i32 id, i32 statId, i32 value, i32 flag);
    i32 DispatchRecvMsg(i32 sender, char* buf, i32 size);
    i32 HandleControlMsg(CNetCtrlMsg* msg, i32 unused);
    i32 OnPlayerLeft(i32 playerId);
    void AckDropPlayer(i32 id);
    RVA(0x000bd4a0, 0x3)
    void WriteTag(const char*) {}

    void RecordDropPlayer2(CNetSessionNode* a, i32 id);
    i32 WaitForOtherPlayers();
    i32 LoadMenuSelectSprite(void* evp);
    i32 ParseChannelTable(void* packet);
    i32 RegisterChannel(const char* name, i32 id, i32 c, i32 d, i32 idx, i32 e);
    i32 RegisterChannelRec(void* rec);
    i32 RemoveChannel(i32 idx);
    i32 OnPauseChannel();
    void OnMultiPause();
    void OnMultiOptions();
    i32 ParseOneChannel(void* rec);
    i32 SendChannelStat422();
    i32 SendChannelStat423();
    i32 CreateSession();
    u32 FrameSyncWait();
    i32 SetupTcpIpConfig();
    i32 CreateLocalPlayer();
    i32 WaitForConnect();
    i32 SaveConfig(CNetSessionNode* recipient);
    i32 LoadConfig(void* cfg);
    i32 ResetPlayerCommands(i32 id);
    u32 GetMaxAckLatency();
    void HandleVersionCheck(CNetVersionMsg* msg);
    void AnnounceVersion(CNetSessionNode* param);

    void ApplyDynSetting(CString s);
    void SetServiceName(CString s);
    void PopulateGroupList(void* hList, i32 flag);

    char _padMp[0x520 - 0x51c];
    CNetSession* m_session;
    CNetMgr* m_netGate;
    i32 m_isHost;
    i32 m_sessionTerminated;
    i32 m_530;
    i32 m_534;
    i32 m_538;
    i32 m_levelVerifyResult;
    i32 m_verifyDone;
    i32 m_recordAcked[4];
    i32 m_recordToken[4];
    i32 m_pollAbort;
    i32 m_568;
    i32 m_56c;
    i32 m_570;
    i32 m_574;
    i32 m_syncGate;
    i32 m_pumpGuard;
    i32 m_connected;
    i32 m_584;
    i32 m_588;
    i32 m_58c;
    i32 m_590;
    i32 m_594;
    CString m_598;
    CString m_groupName;
    CString m_hostName;
    i32 m_5a4;
    i32 m_drainReload;
    i32 m_5ac;
    i32 m_5b0;
    CString m_5b4;
    CString m_5b8;
    CNetSessionNode* m_5bc;
    i32 m_hostIndex;
    i32 m_lastSenderId;
    char _p5c4[0x5cc - 0x5c8];
    i32 m_curSlotId;
    i32 m_5d0;
    i32 m_drainTimer;
    i32 m_frameDelta;
    i32 m_lastTime;
    i32 m_accumTime;
    i32 m_5e4;
    i32 m_5e8;
    i32 m_5ec;
    i32 m_channelLatency[4];
    i32 m_600;

    CDWordArray m_604;

    char m_pad618[0x660 - 0x618];
};
SIZE_UNKNOWN();
SIZE_UNKNOWN();

extern CMulti* g_multiState;
extern CString g_sessionName;
extern "C" i32 g_optionsCursor;

extern CNetMgr* g_groupEnumMgr;

extern i32 g_hostServicesMode;

extern HWND g_netPlayerListHwnd;

void MultiJoinHandler();

class CFile;

extern "C" i32 g_serviceId;
extern "C" i32 Cfg_SetSection(char* buf, const char* fmt, const char* arg);

extern "C" i32 Cfg_AppendKeyVal(char* buf, const char* key, i32 val);
extern "C" CMulti* g_connectRptMgr;
extern "C" i32 Cfg_GetKey(char* out, const char* src, const char* key);
extern "C" HWND g_setupDlgHwnd;
extern "C" i32 BaseDlgProc(HWND, u32 msg, u32 wParam, i32 lParam);
extern "C" void RefreshPlayerRow(HWND hDlg, HWND hList);
extern "C" i32 NetFormatKeyed(char* out, void* src, const char* key);
extern CFile g_obj646778;
extern "C" void PumpBRefresh2356(void* reg, void* fx, i32 flag);
extern "C" void __stdcall PlayIfElapsed(i32 tag, i32 a, i32 b, i32 c);

extern "C" HWND g_sharedFlag;

extern char s_GameKey[];
extern u32 g_ackThrottleDeadline;

void ShowHudMessage(
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
