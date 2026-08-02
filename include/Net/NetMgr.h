#ifndef NET_NETMGR_H
#define NET_NETMGR_H

#include <Ints.h>
#include <string.h>
#include <rva.h>
#include <Wap32/Object.h>

#include <Mfc.h>
#include <wtypes.h>
#include <basetyps.h>
#include <Utils/RegistryHelper.h>
#include <Gruntz/ObList.h>
#include <Rez/RezMgr.h>
#include <Gruntz/String.h>
#include <Gruntz/GruntzPlayer.h>

void ActiveWait(u32 milliseconds);

CString __stdcall operator+(const CString& lhs, const char* rhs);

class GruntzPlayer;
class CGruntzMgr;

class CGruntzCmdMgr;
class CNetMgr;
class CMulti;
struct GruntRec;
class CDDrawSurfaceMgr;

extern i32 g_dropPlayerId;

extern "C" i32 g_localVersion;
extern i32 g_remoteVersion;

union NetGuid {
    GUID m_guid;
    i32 m_words[4];
};

extern NetGuid g_dplayAppGuid;
extern "C" i32 g_cfgWord;

struct CNetVersionMsg {
    char m_pad0[0x18];
    i32 m_remoteVersion;
    i32 m_localVersion;
};
SIZE_UNKNOWN();

struct CNetVersionPacket {
    u8 m_flags;
    char m_pad1[3];

    i32 m_statId;
    i32 m_cfgWord;
    i32 m_butePos;
    char m_pad10[8];
    i32 m_remoteVersion;
    i32 m_localVersion;
};
SIZE(0x20);

class CNetSessionNode;

struct CNetStatPacket {
    u8 m_flags;
    char m_pad1[3];
    i32 m_statId;
    i32 m_value;
    char m_padc[4];
};
SIZE_UNKNOWN();

#pragma pack(push, 1)
struct CNetChannelPacket {
    u8 m_flags;
    char m_pad01[3];
    i32 m_statId;
    u8 m_present;
    u8 m_kind;
    u8 m_slot;
    u8 m_flagsB;
    u8 m_configId;
    u8 m_colorIndex;
    u8 m_humanControlled;
    char m_pad0f[1];
    i32 m_hostIndex;
    char m_name[0x28 - 0x14];
};
SIZE(0x28);

struct CNetOneChannelPacket {
    u8 m_flags;
    char m_pad01[3];
    i32 m_statId;
    i32 m_playerIndex;
    u8 m_present;
    u8 m_colorIndex;
    u8 m_humanControlled;
    u8 m_configId;
    char m_pad10[1];
    u8 m_comboSel;
    u8 m_readyFlag;
    char m_pad13[1];
    i32 m_slotKey;
    char m_name[0x2c - 0x18];
};
SIZE(0x2c);

struct CNetChannelRow {
    u8 m_liveGate;
    u8 m_colorIndex;
    u8 m_humanControlled;
    u8 m_configId;
    u8 m_pad04;
    u8 m_comboSel;
    u8 m_readyFlag;
    u8 m_pad07;
    i32 m_slotKey;
    char m_name[0x20 - 0x0c];
};
SIZE(0x20);

struct CNetChannelTablePacket {
    u8 m_flags;
    char m_pad01[3];
    i32 m_statId;
    CNetChannelRow m_rows[4];
};
SIZE(0x88);
#pragma pack(pop)

struct CNetCmdSlot {
    i32 m_state;
    i32 m_isRemote;

    i32 m_latchedSeq;
    GruntzPlayer* m_desc;

    i32 m_latency;

    i32 m_baseSeq;
    i32 m_maxSeq;

    CMulti* m_owner;

    CPtrList m_cmds;
    i32 m_ackFlags[4];
    i32 m_rangeA[3];
    i32 m_rangeB[3];

    CNetCmdSlot();
    ~CNetCmdSlot();
    void ResetAll();
    void AdvanceSeq(i32 id);
    void RaiseMax(i32 v);
    void ResetTriple(i32* p);

    i32 NetCmdIdFind(i32* arr, i32 v);
    void NetCmdIdAdd(i32* arr, i32 v);
    void NetCmdIdClear(i32* arr, i32 v);
    void AddCmd(GruntRec* cmd);
    void RemoveCmd(i32 seq);
    void GetRange(i32* pMin, i32* pMax);
    GruntRec* FindCmd(i32 seq);
    void ClearCmds();
    void Touch();
    void FullReset();
    void ClearAckFlags();
    CString BuildHostName();
    i32 Init(CMulti* owner, GruntzPlayer* desc, i32 state);
    i32 ProcessCmd(i32 playerId, void* rec, i32 size);

    i32 Ready();
};
SIZE(0x64);

#pragma pack(push, 1)
struct CNetCmdHdr {
    i32 m_sequence;
    i32 m_windowBase;
    i32 m_checksum;

    u8 m_entryCount;
};
SIZE(0xd);
#pragma pack(pop)

struct GruntRec {
    i32 m_seq;
    i32 m_checksum;
    unsigned char m_count;
    char pad09[3];
    i32 m_payloadLen;
    char m_payload[0x410 - 0x10];
};
SIZE(0x410);

class CGruntzCommand;
struct CNetCtrlMsg;
struct CNetMsg;

union CNetWireMsg {
    char* m_bytes;
    CNetMsg* m_msg;
    CNetCtrlMsg* m_ctrl;
    CNetChannelPacket* m_chan;
    CNetOneChannelPacket* m_oneChannel;
    CNetVersionMsg* m_version;
    CNetCmdHdr* m_cmdHdr;
};

void* AllocateGruntRecord(i32 clear);
void RecycleCmd(void* cmd);

struct CNetSession {

    CGruntzMgr* m_mgr;

    CMulti* m_session;
    CNetMgr* m_netMgr;
    CNetSessionNode* m_localDesc;

    i32 m_tick;
    i32 m_snapshotDone;
    i32 m_seq;
    i32 m_period;
    CNetCmdSlot m_slots[4];
    CGruntzCommand* m_idMap[0x80];
    GruntRec m_records[0x80];

    CNetCmdSlot* FindCmdSlot(i32 playerId);
    void ResetCmdBuffers();
    i32 AllSlotsReachedSeq(i32 seq);
    void AdvanceAllSlots(i32 id);
    void RaiseAllSlotsMax(i32 v);
    i32 CheckLatency(i32 cap);
    CNetCmdSlot* CreateSlot(i32 index, i32 owner);
    i32 Verify();
    void ResetAll();
    void Reset();
    i32 Verify(i32 n);

    CNetCmdSlot* FindSlot(u32 key);

    i32 Init(CGruntzMgr* mgr, class CMulti* owner, CNetMgr* netMgr);

    ~CNetSession();
    void ResetSync();
    i32 Poll(i32 delta);
    i32 Dispatch(i32 a, CNetCtrlMsg* b, i32 c);
    i32 DispatchMsg(CNetCtrlMsg* m, i32 ctrlArg);
    i32 Tick();
    i32 SendAll();

    i32 SendGruntRecord(i32 seq, GruntRec* rec, u8 flag, i32 slot, i32 dpTo);
    i32 SendBatch();
    i32 SendOne(CNetCmdSlot* s, i32 v);
    void Reconcile();
    i32 Advance();
    CGruntzCommand* GetSlotPtr(i32 v);

    void ArmSlot(void* node, u8 parity);

    i32 Checksum();

    void BuildGruntzCrcInfo();

    void* operator new(size_t n) {
        return ::operator new(static_cast<u32>(n));
    }
    void operator delete(void* p) {
        ::operator delete(p);
    }

    CNetSession() {
        ResetAll();
    }
};
SIZE(0x20bb0);

struct NetDPName {
    u32 dwSize;
    u32 dwFlags;
    char* lpszShortNameA;
    char* lpszLongNameA;
};
SIZE(0x10);

typedef BOOL(__stdcall* NetEnumSessionsCallback)(
    void* lpThisSD,
    void* lpdwTimeout,
    DWORD dwFlags,
    CNetMgr* lpContext
);
typedef BOOL(__stdcall* NetEnumPlayersCallback)(
    u32 dpId,
    DWORD dwPlayerType,
    NetDPName* lpName,
    DWORD dwFlags,
    CNetMgr* lpContext
);

struct IDirectPlay4Z {

    STDMETHOD(QueryInterface)(const GUID* riid, void* out) PURE;
    STDMETHOD(AddRef)() PURE;
    STDMETHOD(Release)() PURE;
    STDMETHOD(Open)(void* a, void* b, i32 c) PURE;
    STDMETHOD(v04)() PURE;
    STDMETHOD(v05)() PURE;

    STDMETHOD(CreatePlayer)
    (i32* lpidPlayer,
     NetDPName* lpPlayerName,
     i32 hEvent,
     void* lpData,
     u32 dwDataSize,
     u32 dwFlags) PURE;
    STDMETHOD(v07)() PURE;
    STDMETHOD(v08)() PURE;
    STDMETHOD(v09)() PURE;
    STDMETHOD(v0a)() PURE;
    STDMETHOD(v0b)() PURE;
    STDMETHOD(EnumGroupsCb)(void* desc, NetEnumPlayersCallback callback, void* ctx, i32 flags) PURE;
    STDMETHOD(EnumPlayers)(
        void* desc,
        void* a,
        NetEnumSessionsCallback callback,
        void* ctx,
        void* flags
    ) PURE;
    STDMETHOD(Enum2)(void* desc, void* ctx) PURE;
    STDMETHOD(v0f)() PURE;
    STDMETHOD(v10)() PURE;
    STDMETHOD(GetMessageCount)(i32 idPlayer, i32* lpCount) PURE;
    STDMETHOD(v12)() PURE;
    STDMETHOD(GetGroupData)(i32 id, void* lpData, i32 flags) PURE;
    STDMETHOD(GetData2)(i32 id, void* lpData, u32* lpSize, u32 fl) PURE;
    STDMETHOD(v15)() PURE;
    STDMETHOD(GetPlayerData2)(void* in, void* out) PURE;
    STDMETHOD(v17)() PURE;
    STDMETHOD(EnumGroups)(void* desc, i32 flags) PURE;
    STDMETHOD(Receive)(i32* lpidFrom, i32* lpidTo, i32 flags, void* lpData, i32* lpSize) PURE;
    STDMETHOD(SetData5)(i32 a, i32 b, i32 c, void* data, i32 size) PURE;
    STDMETHOD(v1b)() PURE;
    STDMETHOD(v1c)() PURE;
    STDMETHOD(GetData5)(i32 id, void* lpData, i32 size, i32 fl) PURE;
    STDMETHOD(v1e)() PURE;
    STDMETHOD(v1f)() PURE;
    STDMETHOD(v20)() PURE;
    STDMETHOD(v21)() PURE;
    STDMETHOD(v22)() PURE;
    STDMETHOD(v23)() PURE;
    STDMETHOD(v24)() PURE;
    STDMETHOD(v25)() PURE;
    STDMETHOD(v26)() PURE;
    STDMETHOD(v27)() PURE;
    STDMETHOD(v28)() PURE;
    STDMETHOD(v29)() PURE;
    STDMETHOD(v2a)() PURE;
    STDMETHOD(v2b)() PURE;
    STDMETHOD(v2c)() PURE;
    STDMETHOD(v2d)() PURE;
    STDMETHOD(v2e)() PURE;
    STDMETHOD(v2f)() PURE;
    STDMETHOD(v30)() PURE;
    STDMETHOD(SendEx)(
        i32 idFrom,
        i32 idTo,
        i32 flags,
        LPVOID lpData,
        i32 size,
        i32 pri,
        i32 timeout,
        LPVOID ctx,
        LPDWORD lpMsgId
    ) PURE;
};
SIZE_UNKNOWN();

class CNetPlayerListNode;

struct CNetSessionDesc {
    i32 m_dwSize;
    i32 m_dwFlags;
    GUID m_guidInstance;
    GUID m_guidApplication;
    i32 m_dwMaxPlayers;
    i32 m_dwCurrentPlayers;
    char* m_lpszName;
    char* m_lpszPassword;
    i32 m_dwReserved1;
    i32 m_dwReserved2;
    i32 m_dwUser1;
    i32 m_dwUser2;
    i32 m_dwUser3;
    i32 m_dwUser4;
};
SIZE(0x50);

struct CNetCaps {
    i32 m_dwSize;
    i32 m_dwFlags;
    i32 m_dwMaxBufferSize;
    i32 m_dwMaxQueueSize;
    i32 m_dwMaxPlayers;
    i32 m_dwHundredBaud;
    i32 m_dwLatency;
    i32 m_dwMaxLocalPlayers;
    i32 m_dwHeaderLength;
    i32 m_dwTimeout;
};
SIZE(0x28);

class CNetPlayerListNode : public CObject {
public:
    CNetSessionDesc m_desc;

    __POSITION* m_listPosition;

    CNetPlayerListNode() {
        memset(&m_desc, 0, sizeof(m_desc));
        m_listPosition = 0;
    }
    virtual ~CNetPlayerListNode() OVERRIDE;
    i32 Init(CNetSessionDesc* desc);

    void FreeStrings();

    char* GroupName();
};
SIZE(0x58);

class CNetSessionNode : public CObject {
public:
    i32 m_id;
    CString m_name;
    CString m_longName;
    i32 m_enumFlags;
    char* m_ownedBufferB;
    char* m_ownedBufferA;
    i32 m_reserved1c;
    __POSITION* m_listPosition;

    CNetSessionNode() {
        m_id = 0;
        m_listPosition = 0;
        m_ownedBufferA = 0;
        m_ownedBufferB = 0;
    }
    virtual ~CNetSessionNode() OVERRIDE;

    i32 InitSession(i32 id, const char* nameA, const char* nameB, i32 flags);

    CString GetName();
};
SIZE(0x24);

extern "C" BOOL __stdcall
NetEnumPlayerCb(void* lpThisSD, void* lpdwTimeout, DWORD dwFlags, CNetMgr* ctx);

extern "C" BOOL __stdcall
NetEnumCb(u32 dpId, DWORD dwType, NetDPName* lpName, DWORD dwFlags, CNetMgr* ctx);

struct IDirectPlay;

struct CNetCtrlMsg {

    union {
        i32 m_code;
        struct {
            u8 m_routeFlags;
            u8 m_routeSlot;
        } m_route;
    };
    i32 m_subCode;
    i32 m_playerId;
};
SIZE_UNKNOWN();

struct MenuSelectEvent {
    char m_pad0[0x4];
    i32 m_armed;
    i32 m_id;
    char m_pad0c[0x20 - 0xc];
    char* m_nameA;
    char* m_nameB;
};
SIZE_UNKNOWN();

class CFontConfig;

extern "C" char g_recvBuffer[];

extern u8 g_chanStat422_flag;
extern i32 g_chanStat422_id;
extern i32 g_chanStat422_val;
extern u8 g_chanStat423_flag;
extern i32 g_chanStat423_id;
extern i32 g_chanStat423_val;

struct CChatPacket {
    u8 m_flag;
    char m_pad1[3];
    i32 m_id;
    i32 m_val;
    char m_buf[0x100]; // capacity unproven
};

extern CChatPacket g_chatPacket;

extern "C" i32 g_playerLeftFlag;
extern "C" i32 g_activePlayerCount;

struct InterfaceObject;

class CNetMgr : public CObject {
public:
    virtual ~CNetMgr() OVERRIDE;

    void OnMultiOptions();
    void OnMultiPause();
    void OnOutOfSync();
    void ApplyCmdDelayDefaults();
    u32 GetMaxAckLatency();
    void ReportAckLatency();
    CNetSessionNode* FindPlayerById(i32 id);

    i32 LoadMenuSelectSprite(void* ev);

    struct InterfaceObject* Find(i32 kind);

    i32 RemovePlayerObj(CNetSessionNode* obj);
    i32 RemovePlayerById(i32 id);
    i32 RemovePlayerNode(CNetPlayerListNode* node);
    i32 EnumSessions2(void* ctx);
    void* GetPlayerData(i32 id);
    i32 SetGroupData2(CNetSessionNode* a, CNetSessionNode* b, i32 c, void* data, i32 size);

    i32 SendEx(
        i32 idFrom,
        i32 idTo,
        i32 flags,
        LPVOID lpData,
        i32 size,
        i32 pri,
        i32 timeout,
        LPVOID ctx,
        LPDWORD lpMsgId
    );
    i32 SetData(i32 a, i32 b, i32 c, void* data, i32 size);
    i32 Receive(CNetSessionNode* from, CNetSessionNode* to, i32 flags, void* lpData, i32* lpSize);
    i32 SetGroupDataFrom(CNetSessionNode* a, i32 c, void* data, i32 size);
    i32 GetGroupInfo(CNetSessionNode* a, void* desc, i32 flags);
    i32 EnumSessions(void* desc, void* ctx);

    void Destroy();
    void ClearGroupList();
    void ClearPlayerList();
    void ClearSessionList();

    i32 ReadGroupSel(void* hList);
    i32 ReadPlayerSel(void* hList);

    i32 EnumPlayersInto(void* a, void* b);

    i32 Init(void* a, NetGuid appGuid);

    CNetPlayerListNode* AddPlayerNode(void* playerDesc);
    void PopulatePlayerList(void* hList);
    CNetSessionNode*
    EnumPlayersCb(CNetPlayerListNode* a, const char* name, const char* longName, i32 d);
    i32 EnumGroupsAll();
    i32 EnumGroupsRange(void* rec, i32 flags);
    CNetSessionNode* AddSessionNode(i32 id, const char* nameA, const char* nameB, i32 flags);

    CNetSessionNode* CreatePlayer(char* name, const char* longName, i32 c);
    void PopulateSessionList(void* hList);

    i32 DropChannelPlayer(i32 idx);
    i32 LoadConfig(void* cfg);
    i32 AutoTuneCmdDelay();

    i32 WriteCmdDelay(i32 flag);

    i32 InitFromProvider(InterfaceObject* a, GUID appGuid);
    i32 EnumServiceProviders(i32 validated);
    InterfaceObject* AddGroupNode(void* guid, void* name);
    CNetPlayerListNode*
    EnumGroupsInto(i32 maxPlayers, char* sessionName, i32 user1, const char* password);

    static void ReportError(char* file, i32 line, i32 hr, void* hWnd);

    static void SetReportMode(i32 log, i32 msgBox, i32 beep, i32 third);

    void HandleVersionCheck(CNetVersionMsg* msg);
    void AnnounceVersion(i32 param);

    i32 CreateSession();

    i32 VerifyCustomLevel(void* h, CNetSessionNode* playerTok);

    i32 Poll(i32 token);

    void AckJoinFailure();
    void DropTimeout();

    i32 CreateLocalPlayer();
    CString GetString5a0();

    i32 SaveConfig(CNetSessionNode* recipient);

    i32 SendStatBuf(CNetStatPacket* pkt, i32 flag);
    void SendStatFlag(i32 id, i32 flag);
    void SendNetStat(i32 id, u32 value, i32 flag);
    i32 SendStatFrom(CNetStatPacket* pkt, i32 b, i32 c);
    i32 SendStatPair(CNetSessionNode* recipient, CNetStatPacket* pkt, i32 c);

    i32 SendStatTo(CNetSessionNode* recipient, i32 id, i32 c);

    i32 SendNetStatTo(CNetSessionNode* recipient, i32 id, u32 value, i32 c);
    i32 SendStatPairRaw(CNetSessionNode* recipient, void* pkt, i32 size, i32 c);
    i32 SendStatValue(i32 id, i32 statId, i32 value, i32 flag);

    i32 PollSessionGated(i32 a1, i32 a2);

    i32 HandleControlMsg(CNetCtrlMsg* msg, i32 unused);
    i32 OnPlayerLeft(i32 playerId);

    void HandleSpriteMsg(CNetCtrlMsg* msg);

    void ReportVersionMsg(const char* msg, i32 zero);
    void SendStatPacket(i32 param, const void* packet, i32 size, i32 flag);

    i32 ResolveLocalPlayer();
    i32 BroadcastChannelTable(CNetSessionNode* recipient);
    i32 ParseChannelTable(void* packet);
    i32 RegisterChannelFrom(const char* name, i32 b, i32 e, i32 f);
    i32 RegisterChannel(const char* name, i32 id, i32 c, i32 d, i32 idx, i32 e);
    i32 RegisterChannelRec(void* rec);
    i32 RemoveChannel(i32 idx);
    i32 OnPauseChannel();
    i32 BroadcastOneChannel(GruntzPlayer* ch);
    i32 ParseOneChannel(void* rec);
    i32 SendChannelStat422();
    i32 SendChannelStat423();
    i32 BroadcastChatLine(char* text, i32 toChat, i32 showWnd, void* hWnd);

    u32 FrameSyncWait();
    void OnDropPlayer();
    i32 WaitForConnect();
    i32 ResetPlayerCommands(i32 id);

    void ReportStatusId(UINT strId, i32 level);
    void AckDropPlayer(i32 id);

    i32 SendStat3(i32 id, u32 value, i32 flag);
    i32 PollSession();
    i32 DispatchRecvMsg(i32 sender, char* buf, i32 size);

    NetGuid m_appGuid;
    IDirectPlay* m_releaseIface;
    IDirectPlay4Z* m_directPlay;

    CObList m_groups;
    CObList m_players;
    CObList m_sessions;

    InterfaceObject* m_groupSel;

    CNetPlayerListNode* m_playerSel;
    CNetSessionNode* m_sessionSel;
    POSITION m_groupSelId;
    POSITION m_playerSelId;
    POSITION m_sessionSelId;
    i32 m_reserved88;

    CNetMgr() {
        m_releaseIface = 0;
        m_directPlay = 0;
    }

    i32 DetectConnectionConfig();
    i32 SetupTcpIpConfig();
    i32 OnJoinConfirm(void* hDlg);

    void ApplyDynSetting(CString s);

    void PopulateGroupList(HWND hList, i32 flag);
    void SetServiceName(CString s);
};
SIZE(0x8c);

extern "C" i32 g_spEnumValidated;
class CNetMgr;
struct NetDPName;
extern "C" BOOL __stdcall
NetEnumPlayerCb(void* lpThisSD, void* lpdwTimeout, DWORD dwFlags, CNetMgr* ctx);
extern "C" BOOL __stdcall
NetEnumCb(u32 dpId, DWORD dwType, NetDPName* lpName, DWORD dwFlags, CNetMgr* ctx);

extern "C" i32 g_activePlayerCount;

static i32 __stdcall
EnumProviderCb(GUID* lpGuid, char* lpName, DWORD dwMajor, DWORD dwMinor, void* lpContext);

#endif // NET_NETMGR_H
