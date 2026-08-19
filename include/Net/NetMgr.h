#ifndef NET_NETMGR_H
#define NET_NETMGR_H

#include <rva.h>

#include <Mfc.h>

#include <Enums.h>
#include <Gruntz/ColorTint.h>
#include <Gruntz/GruntzPlayer.h>
#include <Gruntz/ObList.h>
#include <Gruntz/String.h>
#include <Ints.h>
#include <Net/NetMsgId.h>
#include <Net/NetSlotState.h>
#include <Rez/RezMgr.h>
#include <Utils/RegistryHelper.h>
#include <Wap32/Object.h>

#include <basetyps.h>
#include <string.h>
#include <wtypes.h>

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

extern i32 g_localVersion;
extern i32 g_remoteVersion;

union NetGuid {
    GUID m_guid;
    i32 m_words[4];
};

extern NetGuid g_dplayAppGuid;
extern i32 g_cfgWord;

struct CNetVersionMsg {
    char m_pad0[0x18];
    i32 m_remoteVersion;
    i32 m_localVersion;
};

struct CNetVersionPacket {
    u8 m_flags;
    char m_pad1[3];

    NetMsgId m_statId;
    i32 m_cfgWord;
    i32 m_butePos;
    char m_pad10[8];
    i32 m_remoteVersion;
    i32 m_localVersion;
};

class CNetSessionNode;

struct CNetStatPacket {
    u8 m_flags;
    char m_pad1[3];
    NetMsgId m_statId;
    i32 m_value;
    char m_padc[4];
};

struct CNetChannelStatPacket {
    u8 m_flags;
    char m_pad1[3];
    NetMsgId m_statId;
    i32 m_value;
};

#pragma pack(push, 1)
struct CNetChannelPacket {
    u8 m_flags;
    char m_pad01[3];
    NetMsgId m_statId;
    u8 m_present;
    u8 m_kind;
    u8 m_slot;
    u8 m_flagsB;
    u8 m_configId;
    GZ_ENUM_STORAGE(ColorTint, u8) m_colorIndex;
    u8 m_humanControlled;
    char m_pad0f[1];
    i32 m_hostIndex;
    char m_name[0x28 - 0x14];
};

struct CNetOneChannelPacket {
    u8 m_flags;
    char m_pad01[3];
    NetMsgId m_statId;
    i32 m_playerIndex;
    u8 m_present;
    GZ_ENUM_STORAGE(ColorTint, u8) m_colorIndex;
    u8 m_humanControlled;
    u8 m_configId;
    char m_pad10[1];
    u8 m_comboSel;
    u8 m_readyFlag;
    char m_pad13[1];
    i32 m_slotKey;
    char m_name[0x2c - 0x18];
};

struct CNetChannelRow {
    u8 m_liveGate;
    GZ_ENUM_STORAGE(ColorTint, u8) m_colorIndex;
    u8 m_humanControlled;
    u8 m_configId;
    u8 m_pad04;
    u8 m_comboSel;
    u8 m_readyFlag;
    u8 m_pad07;
    i32 m_slotKey;
    char m_name[0x20 - 0x0c];
};

struct CNetChannelTablePacket {
    u8 m_flags;
    char m_pad01[3];
    NetMsgId m_statId;
    CNetChannelRow m_rows[4];
};
#pragma pack(pop)

struct CNetCmdSlot {
    NetSlotState m_state;
    i32 m_isRemote;

    i32 m_latchedSeq;
    GruntzPlayer* m_desc;

    i32 m_latency;

    i32 m_baseSeq;
    i32 m_maxSeq;

    CMulti* m_owner;

    CPtrList m_cmds;
    i32 m_ackFlags[NET_SLOT_COUNT];
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
    i32 Init(CMulti* owner, GruntzPlayer* desc, NetSlotState state);
    i32 ProcessCmd(i32 playerId, char* rec, i32 size);

    i32 Ready();
};

#pragma pack(push, 1)
struct CNetCmdHdr {
    i32 m_sequence;
    i32 m_windowBase;
    i32 m_checksum;

    u8 m_entryCount;
};
#pragma pack(pop)

struct GruntRec {
    i32 m_seq;
    i32 m_checksum;
    unsigned char m_count;
    char pad09[3];
    i32 m_payloadLen;
    char m_payload[0x410 - 0x10];
};

class CGruntzCommand;
struct CNetCtrlMsg;
struct CNetMsg;
struct CNetConfigBlob;
struct MenuSelectEvent;

union CNetWireMsg {
    char* m_bytes;
    CNetMsg* m_msg;
    CNetCtrlMsg* m_ctrl;
    CNetChannelPacket* m_chan;
    CNetOneChannelPacket* m_oneChannel;
    CNetChannelTablePacket* m_chanTable;
    CNetVersionMsg* m_version;
    CNetCmdHdr* m_cmdHdr;
    CNetConfigBlob* m_config;
    MenuSelectEvent* m_menuSelect;
};

GruntRec* AllocateGruntRecord(i32 clear);
void RecycleCmd(GruntRec* cmd);

struct CNetSession {

    CGruntzMgr* m_mgr;

    CMulti* m_session;
    CNetMgr* m_netMgr;
    CNetSessionNode* m_localDesc;

    i32 m_tick;
    i32 m_snapshotDone;
    i32 m_seq;
    i32 m_period;
    CNetCmdSlot m_slots[NET_SLOT_COUNT];
    CGruntzCommand* m_idMap[0x80];
    GruntRec m_records[0x80];

    CNetCmdSlot* FindCmdSlot(i32 playerId);
    void ResetCmdBuffers();
    i32 AllSlotsReachedSeq(i32 seq);
    void AdvanceAllSlots(i32 id);
    void RaiseAllSlotsMax(i32 v);
    i32 CheckLatency(i32 cap);
    CNetCmdSlot* CreateSlot(i32 index, NetSlotState state);
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

    void ArmSlot(CGruntzCommand* node, u8 parity);

    i32 Checksum();

    void BuildGruntzCrcInfo();

    // No class-level operator new/delete: retail's unwind funclet for the
    // `new CNetSession()` in CMulti::CreateSession calls the GLOBAL
    // ??3@YAXPAX@Z, which a class-level forwarder cannot produce - cl emits
    // ??3CNetSession@@SAXPAX@Z and points the funclet at that instead.

    CNetSession() {
        ResetAll();
    }
};

struct NetDPName {
    u32 dwSize;
    u32 dwFlags;
    char* lpszShortNameA;
    char* lpszLongNameA;
};

struct CNetSessionDesc;
struct CNetCaps;

typedef BOOL(__stdcall* NetEnumSessionsCallback)(
    CNetSessionDesc* lpThisSD,
    u32* lpdwTimeout,
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

    STDMETHOD(QueryInterface)(const GUID* riid, void** out) PURE;
    STDMETHOD(AddRef)() PURE;
    STDMETHOD(Release)() PURE;
    STDMETHOD(v03)() PURE;
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
    STDMETHOD(EnumGroupsCb)
    (GUID* lpguidInstance, NetEnumPlayersCallback callback, void* ctx, i32 flags) PURE;
    STDMETHOD(EnumPlayers)(
        CNetSessionDesc* lpsd,
        u32 dwTimeout,
        NetEnumSessionsCallback callback,
        void* ctx,
        u32 dwFlags
    ) PURE;
    STDMETHOD(Enum2)(CNetCaps* lpCaps, void* ctx) PURE;
    STDMETHOD(v0f)() PURE;
    STDMETHOD(v10)() PURE;
    STDMETHOD(GetMessageCount)(i32 idPlayer, i32* lpCount) PURE;
    STDMETHOD(v12)() PURE;
    STDMETHOD(GetGroupData)(i32 id, CNetCaps* lpCaps, i32 flags) PURE;
    STDMETHOD(GetData2)(i32 id, void* lpData, u32* lpSize, u32 fl) PURE;
    STDMETHOD(v15)() PURE;
    STDMETHOD(GetPlayerData2)(void* lpData, i32* lpdwDataSize) PURE;
    STDMETHOD(v17)() PURE;
    STDMETHOD(EnumGroups)(CNetSessionDesc* desc, i32 flags) PURE;
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

class CNetPlayerListNode : public CObject {
public:
    CNetSessionDesc m_desc;

    __POSITION* m_listPosition;

    CNetPlayerListNode() {
        memset(&m_desc, 0, sizeof(m_desc));
        m_listPosition = NULL;
    }
    virtual ~CNetPlayerListNode() OVERRIDE;
    i32 Init(CNetSessionDesc* desc);

    void FreeStrings();

    char* GroupName();
};

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
        m_listPosition = NULL;
        m_ownedBufferA = NULL;
        m_ownedBufferB = NULL;
    }
    virtual ~CNetSessionNode() OVERRIDE;

    i32 InitSession(i32 id, const char* nameA, const char* nameB, i32 flags);

    CString GetName();
};

extern BOOL __stdcall
NetEnumPlayerCb(CNetSessionDesc* lpThisSD, u32* lpdwTimeout, DWORD dwFlags, CNetMgr* ctx);

extern BOOL __stdcall
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

struct MenuSelectEvent {
    char m_pad0[0x4];
    i32 m_armed;
    i32 m_id;
    char m_pad0c[0x20 - 0xc];
    char* m_nameA;
    char* m_nameB;
};

class CFontConfig;

extern char g_recvBuffer[];

extern CNetChannelStatPacket g_chanStat422;
extern CNetChannelStatPacket g_chanStat423;

struct CChatPacket {
    u8 m_flag;
    char m_pad1[3];
    i32 m_id;
    i32 m_val;
    char m_buf[0x100]; // capacity unproven
};

extern CChatPacket g_chatPacket;

extern i32 g_playerLeftFlag;
extern i32 g_activePlayerCount;

struct InterfaceObject;

class CNetMgr : public CObject {
public:
    virtual ~CNetMgr() OVERRIDE;

    CNetSessionNode* FindPlayerById(i32 id);
    struct InterfaceObject* Find(i32 kind);

    i32 RemovePlayerObj(CNetSessionNode* obj);
    i32 RemovePlayerById(i32 id);
    i32 RemovePlayerNode(CNetPlayerListNode* node);
    i32 GetMaxPlayers();
    i32 EnumSessions2(void* ctx);
    CNetSessionNode* GetPlayerData(i32 id);
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
    i32 GetGroupInfo(CNetSessionNode* a, CNetCaps* caps, i32 flags);
    i32 EnumSessions(CNetCaps* caps, void* ctx);

    void Destroy();
    void ClearGroupList();
    void ClearPlayerList();
    void ClearSessionList();

    i32 ReadGroupSel(HWND hList);
    i32 ReadPlayerSel(HWND hList);

    i32 EnumPlayersInto(u32 dwTimeout, u32 dwFlags);

    // Retail mangles the lobby argument as PAX; the body recovers the
    // IDirectPlayLobby identity before dispatching Connect.
    i32 Init(void* lobby, NetGuid appGuid);

    CNetPlayerListNode* AddPlayerNode(CNetSessionDesc* playerDesc);
    void PopulatePlayerList(HWND hList);
    CNetSessionNode*
    EnumPlayersCb(CNetPlayerListNode* a, const char* name, const char* longName, i32 d);
    i32 EnumGroupsAll();
    i32 EnumGroupsRange(CNetPlayerListNode* rec, i32 flags);
    CNetSessionNode* AddSessionNode(i32 id, const char* nameA, const char* nameB, i32 flags);

    CNetSessionNode* CreatePlayer(char* name, const char* longName, i32 c);
    void PopulateSessionList(HWND hList);

    i32 InitFromProvider(InterfaceObject* a, GUID appGuid);
    i32 EnumServiceProviders(i32 validated);
    InterfaceObject* AddGroupNode(GUID* guid, const char* name);
    CNetPlayerListNode*
    EnumGroupsInto(i32 maxPlayers, char* sessionName, i32 user1, const char* password);

    static void ReportError(const char* file, i32 line, HRESULT hr, HWND hWnd);

    static void SetReportMode(b32 log, b32 msgBox, b32 beep, b32 unknownOption);
    void PopulateGroupList(HWND hList, i32 flag);

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
        m_releaseIface = NULL;
        m_directPlay = NULL;
    }
};

extern i32 g_spEnumValidated;
class CNetMgr;
struct NetDPName;
static i32 __stdcall
EnumProviderCb(GUID* lpGuid, char* lpName, DWORD dwMajor, DWORD dwMinor, void* lpContext);

#endif // NET_NETMGR_H
