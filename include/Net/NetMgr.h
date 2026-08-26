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
#include <Net/NetPacketLayout.h>
#include <Net/NetSlotState.h>
#include <Rez/RezMgr.h>
#include <Utils/RegistryHelper.h>
#include <Wap32/Object.h>

#include <basetyps.h>
#include <dplay.h>
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

struct CNetVersionPacket {
    GZ_ENUM_STORAGE(NetPacketFlags, u8) m_flags;
    char m_pad1[3];

    NetMsgId m_messageId;
    i32 m_butePos;
    i32 m_cfgWord;
    char m_pad10[8];
    i32 m_remoteVersion;
    i32 m_localVersion;
};

class CNetPlayerNode;

struct CNetValuePacket {
    GZ_ENUM_STORAGE(NetPacketFlags, u8) m_flags;
    char m_pad1[3];
    NetMsgId m_messageId;
    i32 m_value;
    char m_padc[4];
};

struct CNetOptionsStatePacket {
    GZ_ENUM_STORAGE(NetPacketFlags, u8) m_flags;
    char m_pad1[3];
    NetMsgId m_messageId;
    i32 m_value;
};

#pragma pack(push, 1)
struct CNetPlayerRegistrationPacket {
    GZ_ENUM_STORAGE(NetPacketFlags, u8) m_flags;
    char m_pad01[3];
    NetMsgId m_messageId;
    u8 m_active;
    GZ_ENUM_STORAGE(ColorTint, u8) m_color;
    u8 m_humanControlled;
    GZ_ENUM_STORAGE(BattlezDifficulty, u8) m_difficulty;
    u8 m_preferredPlayerIndex;
    u8 m_maxGruntz;
    u8 m_ready;
    char m_pad0f[1];
    i32 m_networkPlayerId;
    char m_name[0x28 - 0x14];
};

struct CNetPlayerUpdatePacket {
    GZ_ENUM_STORAGE(NetPacketFlags, u8) m_flags;
    char m_pad01[3];
    NetMsgId m_messageId;
    i32 m_playerIndex;
    u8 m_active;
    GZ_ENUM_STORAGE(ColorTint, u8) m_color;
    u8 m_humanControlled;
    GZ_ENUM_STORAGE(BattlezDifficulty, u8) m_difficulty;
    char m_pad10[1];
    u8 m_maxGruntz;
    u8 m_ready;
    char m_pad13[1];
    i32 m_networkPlayerId;
    char m_name[0x2c - 0x18];
};

struct CNetPlayerRecord {
    u8 m_active;
    GZ_ENUM_STORAGE(ColorTint, u8) m_color;
    u8 m_humanControlled;
    GZ_ENUM_STORAGE(BattlezDifficulty, u8) m_difficulty;
    u8 m_pad04;
    u8 m_maxGruntz;
    u8 m_ready;
    u8 m_pad07;
    i32 m_networkPlayerId;
    char m_name[0x20 - 0x0c];
};

struct CNetPlayerTablePacket {
    GZ_ENUM_STORAGE(NetPacketFlags, u8) m_flags;
    char m_pad01[3];
    NetMsgId m_messageId;
    CNetPlayerRecord m_rows[4];
};
#pragma pack(pop)

struct CNetCmdSlot {
    NetSlotState m_state;
    b32 m_isDraining;

    i32 m_drainSequence;
    GruntzPlayer* m_player;

    i32 m_latency;

    i32 m_contiguousSequence;
    i32 m_peerWindowBase;

    CMulti* m_owner;

    CPtrList m_records;
    i32 m_drainAckFlags[NET_SLOT_COUNT];
    i32 m_receivedAhead[3];
    i32 m_peerReceivedAhead[3];

    CNetCmdSlot();
    ~CNetCmdSlot();
    void ResetSlot();
    void RecordReceivedSequence(i32 sequence);
    void RecordPeerWindowBase(i32 sequence);
    void ClearSequenceSet(i32* sequences);

    i32 ContainsSequence(i32* sequences, i32 sequence);
    void AddSequence(i32* sequences, i32 sequence);
    void RemoveSequence(i32* sequences, i32 sequence);
    void AddRecord(GruntRec* record);
    void RemoveRecord(i32 sequence);
    void GetRecordRange(i32* minimum, i32* maximum);
    GruntRec* FindRecord(i32 sequence);
    void ClearRecords();
    void BeginDrain();
    void ClearSyncState();
    void ClearDrainAcks();
    CString GetPlayerName();
    i32 Initialize(CMulti* owner, GruntzPlayer* player, NetSlotState state);
    i32 ProcessPacket(i32 playerId, char* packet, i32 packetSize);

    i32 DrainAcknowledged();
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
    i32 m_sequence;
    i32 m_checksum;
    unsigned char m_entryCount;
    char m_pad09[3];
    i32 m_payloadLength;
    char m_payload[NET_COMMAND_RECORD_PAYLOAD_BYTES];
};

class CGruntzCommand;
struct CNetPacketPrefix;
struct CNetMsg;
struct CNetGameConfigPacket;
struct CNetChatPacket;

union CNetWireMsg {
    char* m_bytes;
    CNetMsg* m_msg;
    CNetPacketPrefix* m_prefix;
    LPDPMSG_GENERIC m_system;
    LPDPMSG_CREATEPLAYERORGROUP m_playerCreated;
    LPDPMSG_DESTROYPLAYERORGROUP m_playerDestroyed;
    CNetPlayerRegistrationPacket* m_playerRegistration;
    CNetPlayerUpdatePacket* m_playerUpdate;
    CNetPlayerTablePacket* m_playerTable;
    CNetVersionPacket* m_versionCheck;
    CNetCmdHdr* m_cmdHdr;
    CNetGameConfigPacket* m_gameConfig;
    CNetChatPacket* m_chat;
};

GruntRec* AllocateGruntRecord(i32 clear);
void RecycleGruntRecord(GruntRec* cmd);

struct CNetSession {

    CGruntzMgr* m_mgr;

    CMulti* m_owner;
    CNetMgr* m_netMgr;
    CNetPlayerNode* m_localPlayer;

    i32 m_commandTick;
    b32 m_batchBuilt;
    i32 m_sequence;
    i32 m_commandPeriod;
    CNetCmdSlot m_slots[NET_SLOT_COUNT];
    CGruntzCommand* m_commandByTick[0x80];
    GruntRec m_commandRecords[0x80];

    CNetCmdSlot* FindSlotByPlayerId(i32 playerId);
    void ResetLatencies();
    i32 AllPeerWindowsReached(i32 sequence);
    void RecordSequenceForAllSlots(i32 sequence);
    void RecordPeerWindowForAllSlots(i32 sequence);
    i32 AllActiveLatenciesWithin(i32 latencyLimit);
    CNetCmdSlot* CreateSlot(i32 index, NetSlotState state);
    i32 VerifyChecksums();
    void InitializeFields();
    void ResetRound();
    i32 ReadyForSequence(i32 sequence);

    CNetCmdSlot* FindLaggingSlot(u32 latencyThreshold);

    i32 Initialize(CGruntzMgr* mgr, class CMulti* owner, CNetMgr* netMgr);

    ~CNetSession();
    void Shutdown();
    i32 Poll(i32 elapsedMs);
    i32 Dispatch(i32 senderId, CNetPacketPrefix* message, i32 messageSize);
    i32 DispatchSystemMessage(LPDPMSG_GENERIC message, i32 messageSize);
    i32 SendTick();
    i32 RelayDrainingRecords();

    i32 SendGruntRecord(i32 sequence, GruntRec* record, u8 flags, i32 sourceSlot, i32 recipientId);
    i32 SendPendingRecords();
    i32 SendRecord(CNetCmdSlot* slot, i32 sequence);
    void ReconcileDrainingSlots();
    i32 AdvanceTick();
    CGruntzCommand* GetCommandAtTick(i32 commandTick);

    void ScheduleCommand(CGruntzCommand* command, u8 tickOffset);

    i32 ComputeChecksum();

    void BuildGruntzCrcInfo();

    CNetSession() {
        InitializeFields();
    }
};

class CNetSessionListNode;

class CNetSessionListNode : public CObject {
public:
    DPSESSIONDESC2 m_sessionDesc;

    __POSITION* m_listPosition;

    CNetSessionListNode() {
        memset(&m_sessionDesc, 0, sizeof(m_sessionDesc));
        m_listPosition = NULL;
    }
    virtual ~CNetSessionListNode() OVERRIDE;
    i32 Initialize(LPCDPSESSIONDESC2 sessionDesc);

    void FreeSessionStrings();

    char* SessionName();
};

class CNetPlayerNode : public CObject {
public:
    DPID m_playerId;
    CString m_shortName;
    CString m_longName;
    DWORD m_flags;
    // @identity-TODO: both slots are independently zeroed and deleted, but no
    // surviving retail writer or reader proves what either buffer contains.
    char* m_ownedBufferB;
    char* m_ownedBufferA;
    i32 m_reserved1c;
    __POSITION* m_listPosition;

    CNetPlayerNode() {
        m_playerId = 0;
        m_listPosition = NULL;
        m_ownedBufferA = NULL;
        m_ownedBufferB = NULL;
    }
    virtual ~CNetPlayerNode() OVERRIDE;

    i32 Initialize(DPID playerId, const char* shortName, const char* longName, DWORD flags);

    CString ShortName();
};

extern BOOL __stdcall NetEnumSessionCallback(
    LPCDPSESSIONDESC2 sessionDesc,
    LPDWORD timeoutMs,
    DWORD flags,
    LPVOID context
);

extern BOOL __stdcall
NetEnumPlayerCallback(DPID playerId, DWORD playerType, LPCDPNAME name, DWORD flags, LPVOID context);

struct IDirectPlay;

struct CNetPacketPrefix {
    u8 m_routeFlags;
    u8 m_routeSlot;
};

class CFontConfig;

extern char g_recvBuffer[NET_RECEIVE_BUFFER_BYTES];

extern CNetOptionsStatePacket g_optionsOpenedPacket;
extern CNetOptionsStatePacket g_optionsClosedPacket;

struct CNetChatPacket {
    GZ_ENUM_STORAGE(NetPacketFlags, u8) m_flags;
    char m_pad1[3];
    NetMsgId m_messageId;
    i32 m_value;
    char m_text[0x100]; // capacity unproven
};

extern CNetChatPacket g_netChatPacket;

extern b32 g_playerRosterChanged;
extern i32 g_playersInOptionsCount;

struct CNetProviderNode;

class CNetMgr : public CObject {
public:
    virtual ~CNetMgr() OVERRIDE;

    CNetPlayerNode* FindPlayerById(DPID playerId);
    struct CNetProviderNode* FindProvider(i32 providerKind);

    i32 RemovePlayer(CNetPlayerNode* player);
    i32 RemovePlayerById(DPID playerId);
    i32 RemoveSessionListing(CNetSessionListNode* node);
    i32 GetMaxPlayers();
    i32 GetConnectionLatency(DWORD flags);
    CNetPlayerNode* GetPlayerNodeData(DPID playerId);
    i32 Send(
        CNetPlayerNode* sender,
        CNetPlayerNode* recipient,
        DWORD flags,
        void* message,
        DWORD messageSize
    );

    i32 SendEx(
        DPID senderId,
        DPID recipientId,
        DWORD flags,
        LPVOID message,
        DWORD messageSize,
        DWORD priority,
        DWORD timeoutMs,
        LPVOID context,
        LPDWORD messageId
    );
    i32 SendById(DPID senderId, DPID recipientId, DWORD flags, void* message, DWORD messageSize);
    i32 Receive(
        CNetPlayerNode* sender,
        CNetPlayerNode* recipient,
        DWORD flags,
        void* message,
        LPDWORD messageSize
    );
    i32 BroadcastFrom(CNetPlayerNode* sender, DWORD flags, void* message, DWORD messageSize);
    i32 GetPlayerCaps(CNetPlayerNode* player, LPDPCAPS caps, DWORD flags);
    i32 GetCaps(LPDPCAPS caps, DWORD flags);

    void Destroy();
    void ClearProviders();
    void ClearSessionListings();
    void ClearPlayers();

    i32 ReadProviderSelection(HWND hList);
    i32 ReadSessionSelection(HWND hList);

    i32 EnumerateSessions(DWORD timeoutMs, DWORD flags);

    i32 Initialize(void* lobby, NetGuid appGuid);

    CNetSessionListNode* AddSessionListing(LPCDPSESSIONDESC2 sessionDesc);
    void PopulateSessionList(HWND hList);
    CNetPlayerNode* JoinSessionAndCreatePlayer(
        CNetSessionListNode* session,
        const char* shortName,
        const char* longName,
        HANDLE eventHandle
    );
    i32 EnumerateAllPlayers();
    i32 EnumerateSessionPlayers(CNetSessionListNode* session, DWORD flags);
    CNetPlayerNode*
    AddPlayer(DPID playerId, const char* shortName, const char* longName, DWORD flags);

    CNetPlayerNode* CreatePlayer(char* shortName, const char* longName, HANDLE eventHandle);
    void PopulatePlayerList(HWND hList);

    i32 InitializeFromProvider(CNetProviderNode* provider, GUID appGuid);
    i32 EnumServiceProviders(b32 validateProviders);
    CNetProviderNode* AddProvider(GUID* providerGuid, const char* providerName);
    CNetSessionListNode*
    CreateSession(i32 maxPlayers, char* sessionName, i32 applicationData, const char* password);

    static void ReportError(const char* file, i32 line, HRESULT hr, HWND hWnd);

    static void SetReportMode(b32 log, b32 msgBox, b32 beep, b32 debugOutput);
    void PopulateProviderList(HWND hList, i32 excludedProviderKinds);

    NetGuid m_appGuid;
    IDirectPlay* m_directPlayBase;
    IDirectPlay4A* m_directPlay;

    CObList m_providers;
    CObList m_sessionListings;
    CObList m_players;

    CNetProviderNode* m_selectedProvider;

    CNetSessionListNode* m_selectedSession;
    CNetPlayerNode* m_selectedPlayer;
    POSITION m_providerCursor;
    POSITION m_sessionCursor;
    POSITION m_playerCursor;
    i32 m_reserved88;

    CNetMgr() {
        m_directPlayBase = NULL;
        m_directPlay = NULL;
    }
};

static BOOL __stdcall NetEnumProviderCallback(
    LPGUID providerGuid,
    LPSTR providerName,
    DWORD majorVersion,
    DWORD minorVersion,
    LPVOID context
);

extern b32 g_validateProviders;

#endif // NET_NETMGR_H
