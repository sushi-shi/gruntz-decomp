#include <rva.h>

#include <Net/NetMgr.h>

#include <AddrWord.h>
#include <ComOutRef.h>
#include <Enums.h>
#include <Font/Font.h>
#include <MsgParam.h>
#include <Net/DPlaySessionFlags.h>
#include <Net/NetGuids.h>
#include <Net/NetProviderFindKind.h>
#include <Net/NetProviderNode.h>

#include <dplay.h>
#include <dplobby.h>
#include <new>
#include <string.h>

DATA(0x002c0798)
b32 g_validateProviders = false;

// clang-format off
DATA(0x00225cb8)
u8 g_directPlayIpxProviderGuid[16] = {0x00, 0xc4, 0x5b, 0x68, 0x2c, 0x9d, 0xcf, 0x11,
                                   0xa9, 0xcd, 0x00, 0xaa, 0x00, 0x68, 0x86, 0xe3};
DATA(0x00225cc8)
u8 g_directPlayTcpIpProviderGuid[16] = {0xe0, 0x5e, 0xe9, 0x36, 0x77, 0x85, 0xcf, 0x11,
                                   0x96, 0x0c, 0x00, 0x80, 0xc7, 0x53, 0x4e, 0x82};
DATA(0x00225cd8)
u8 g_directPlayModemProviderGuid[16] = {0x60, 0xa7, 0xea, 0x44, 0x68, 0xcb, 0xcf, 0x11,
                                   0x9c, 0x4e, 0x00, 0xa0, 0xc9, 0x05, 0x42, 0x5e};
DATA(0x00225ce8)
u8 g_directPlaySerialProviderGuid[16] = {0x60, 0x68, 0x1d, 0x0f, 0xd9, 0x88, 0xcf, 0x11,
                                   0x9c, 0x4e, 0x00, 0xa0, 0xc9, 0x05, 0x42, 0x5e};
DATA(0x00225cf8)
u8 g_unclassifiedProviderGuid[16] = {0x00, 0xb4, 0x23, 0xd2, 0x7d, 0x0a, 0xd1, 0x11,
                                   0x90, 0xc3, 0x00, 0x60, 0x97, 0x72, 0x58, 0x40};











RVA(0x00178390, 0xbb)
i32 CNetMgr::InitializeFromProvider(CNetProviderNode* provider, GUID appGuid) {
    GUID* guid = provider->m_providerGuid;
    if (guid == NULL) {
        return 0;
    }
    i32 hr = DirectPlayCreate(guid, &m_directPlayBase, NULL);
    if (hr != 0 || m_directPlayBase == NULL) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x41, hr, NULL);
        return 0;
    }




    hr = m_directPlayBase->QueryInterface(IID_IDirectPlay4A, PtrOut(&m_directPlay));
    if (hr != 0) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x50, hr, NULL);
        Destroy();
        return 0;
    }

    m_providerCursor = NULL;
    m_sessionCursor = NULL;
    m_playerCursor = NULL;


    m_appGuid.m_guid = appGuid;
    m_selectedProvider = provider;
    m_selectedSession = NULL;
    m_selectedPlayer = NULL;
    return 1;
}

















RVA(0x00178450, 0xba)
i32 CNetMgr::Initialize(void* lobbyIface, NetGuid appGuid) {
    IDirectPlayLobby* lobby = static_cast<IDirectPlayLobby*>(lobbyIface);



    IDirectPlay2* opened = NULL;
    i32 hr = lobby->Connect(0, &opened, NULL);
    if (hr != 0) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x78, hr, NULL);
        Destroy();
        return 0;
    }
    hr = opened->QueryInterface(IID_IDirectPlay4A, PtrOut(&m_directPlay));
    if (hr != 0) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x81, hr, NULL);
        Destroy();
        return 0;
    }






    m_providerCursor = NULL;
    m_sessionCursor = NULL;
    m_playerCursor = NULL;
    m_appGuid.m_guid = appGuid.m_guid;
    m_selectedProvider = NULL;
    m_selectedSession = NULL;
    m_selectedPlayer = NULL;
    return 1;
}

RVA(0x00178510, 0x49)
void CNetMgr::Destroy() {
    ClearProviders();
    ClearSessionListings();
    ClearPlayers();

    if (m_directPlayBase != NULL) {
        m_directPlayBase->Release();
        m_directPlayBase = NULL;
    }



    if (m_directPlay != NULL) {
        m_directPlay->Close();
        IDirectPlay4A* again = m_directPlay;
        again->Release();
        m_directPlay = NULL;
    }
}

RVA(0x00178560, 0x43)
i32 CNetMgr::EnumServiceProviders(b32 validateProviders) {
    ClearProviders();

    g_validateProviders = validateProviders;
    i32 hr = DirectPlayEnumerate(&NetEnumProviderCallback, this);
    if (hr != 0) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0xda, hr, NULL);
        return hr;
    }
    return 0;
}

RVA(0x001785b0, 0x86)
static BOOL __stdcall NetEnumProviderCallback(
    LPGUID providerGuid,
    LPSTR providerName,
    DWORD majorVersion,
    DWORD minorVersion,
    LPVOID context
) {
    CNetMgr* manager = static_cast<CNetMgr*>(context);
    if (manager == NULL) {
        return false;
    }

    if (g_validateProviders == false) {
        IDirectPlay* dp = NULL;
        i32 hr = DirectPlayCreate(providerGuid, &dp, NULL);
        if (hr != 0) {
            CNetMgr::ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0xfe, hr, NULL);
            return true;
        }
        if (dp == NULL) {
            return true;
        }
        dp->Release();
    }

    return manager->AddProvider(providerGuid, providerName) != NULL;
}


RVA(0x00178640, 0xc8)
CNetProviderNode* CNetMgr::AddProvider(GUID* providerGuid, const char* providerName) {
    CNetProviderNode* node = new CNetProviderNode();

    if (providerGuid == NULL || providerName == NULL) {
        delete node;
        return NULL;
    }

    node->m_providerGuid = providerGuid;
    node->m_providerName = providerName;
    node->m_listPosition = m_providers.AddTail(static_cast<CObject*>(node));
    return node;
}

RVA(0x00178710, 0x3a)
void CNetMgr::ClearProviders() {
    POSITION pos = m_providers.GetHeadPosition();
    while (pos != NULL) {


        delete static_cast<CNetProviderNode*>(m_providers.GetNext(pos));
    }
    m_providers.RemoveAll();
    m_providerCursor = NULL;
    m_selectedProvider = NULL;
}










// @early-stop
RVA(0x00178750, 0x11e)
void CNetMgr::PopulateProviderList(HWND hList, i32 excludedProviderKinds) {
    if (hList == NULL) {
        return;
    }
    SendMessageA(hList, LB_RESETCONTENT, 0, 0);

    m_providerCursor = m_providers.GetHeadPosition();
    CNetProviderNode* provider =
        m_providerCursor != NULL ? static_cast<CNetProviderNode*>(m_providers.GetNext(m_providerCursor)) : NULL;

    while (provider != NULL) {
        if (((excludedProviderKinds & 1) && provider->IsTcpIpProvider())
            || ((excludedProviderKinds & 2) && provider->IsIpxProvider())) {

            if (m_providerCursor != NULL) {
                CNetProviderNode* next = static_cast<CNetProviderNode*>(m_providers.GetAt(m_providerCursor));
                m_providers.GetNext(m_providerCursor);
                provider = next;
            } else {
                provider = NULL;
            }
        } else {



            MsgParam name;
            i32 idx = static_cast<i32>(SendMessageA(
                hList,
                LB_ADDSTRING,
                0,
                (name.m_str = static_cast<LPCTSTR>(provider->ProviderName()), name.m_lparam)
            ));
            if (idx != -1) {
                MsgParam cookie;
                cookie.m_provider = provider;
                SendMessageA(hList, LB_SETITEMDATA, idx, cookie.m_lparam);
            }
            if (m_providerCursor != NULL) {
                CNetProviderNode* next = static_cast<CNetProviderNode*>(m_providers.GetAt(m_providerCursor));
                m_providers.GetNext(m_providerCursor);
                provider = next;
            } else {
                provider = NULL;
            }
        }
    }
}

RVA(0x00178870, 0x78)
i32 CNetMgr::ReadProviderSelection(HWND hList) {
    if (hList == NULL) {
        return 0;
    }
    i32 selection = static_cast<i32>(SendMessageA(hList, LB_GETCURSEL, 0, 0));
    if (selection == -1) {
        return 0;
    }
    if (selection < 0) {
        return 0;
    }
    if (selection >= static_cast<i32>(m_providers.GetCount())) {
        return 0;
    }
    i32 itemData = static_cast<i32>(SendMessageA(hList, LB_GETITEMDATA, selection, 0));
    if (itemData == -1) {
        return 0;
    }
    if (itemData == 0) {
        return 0;
    }



    AddrWord<CNetProviderNode> cookie;
    cookie.m_word = itemData;
    m_selectedProvider = cookie.m_addr;
    return itemData;
}









RVA(0x001788f0, 0x8c)
i32 CNetMgr::EnumerateSessions(DWORD timeoutMs, DWORD flags) {
    ClearSessionListings();

    DPSESSIONDESC2 desc;
    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.guidApplication = m_appGuid.m_guid;

    IDirectPlay4A* directPlay = m_directPlay;
    i32 hr = directPlay->EnumSessions(&desc, timeoutMs, &NetEnumSessionCallback, this, flags);
    if (hr) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x1c9, hr, NULL);
        return hr;
    }
    return 0;
}

RVA(0x00178980, 0x2a)
BOOL __stdcall
NetEnumSessionCallback(
    LPCDPSESSIONDESC2 sessionDesc,
    LPDWORD timeoutMs,
    DWORD flags,
    LPVOID context
) {
    CNetMgr* manager = static_cast<CNetMgr*>(context);
    if (manager != NULL && (flags & DPESC_TIMEDOUT) == 0 && sessionDesc != NULL) {
        manager->AddSessionListing(sessionDesc);
        return true;
    }
    return false;
}








// @early-stop
RVA(0x001789b0, 0x77)
CNetSessionListNode* CNetMgr::AddSessionListing(LPCDPSESSIONDESC2 sessionDesc) {
    if (sessionDesc == NULL) {
        return NULL;
    }

    CNetSessionListNode* node = new CNetSessionListNode();

    if (node->Initialize(sessionDesc) == 0) {
        delete node;
        return NULL;
    }

    node->m_listPosition = static_cast<__POSITION*>(m_sessionListings.AddTail(static_cast<CObject*>(node)));
    return node;
}

RVA(0x00178a30, 0x3d)
void CNetMgr::ClearSessionListings() {
    POSITION pos = m_sessionListings.GetHeadPosition();
    while (pos != NULL) {

        delete static_cast<CNetSessionListNode*>(m_sessionListings.GetNext(pos));
    }
    m_sessionListings.RemoveAll();
    m_sessionCursor = NULL;
    m_selectedSession = NULL;
}








// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00178a70, 0x89)
void CNetMgr::PopulateSessionList(HWND hList) {
    if (hList == NULL) {
        return;
    }

    SendMessageA(hList, LB_RESETCONTENT, 0, 0);

    m_sessionCursor = m_sessionListings.GetHeadPosition();
    CNetSessionListNode* listing =
        m_sessionCursor != NULL ? static_cast<CNetSessionListNode*>(m_sessionListings.GetNext(m_sessionCursor)) : NULL;

    while (listing != NULL) {
        MsgParam name;
        i32 itemIndex = static_cast<i32>(SendMessageA(
            hList,
            LB_ADDSTRING,
            0,
            (name.m_str = listing->m_sessionDesc.lpszSessionNameA, name.m_lparam)
        ));
        if (itemIndex != -1) {
            MsgParam cookie;
            cookie.m_sessionListing = listing;
            SendMessageA(hList, LB_SETITEMDATA, itemIndex, cookie.m_lparam);
        }



        if (m_sessionCursor != NULL) {
            CNetSessionListNode* next =
                static_cast<CNetSessionListNode*>(m_sessionListings.GetAt(m_sessionCursor));
            m_sessionListings.GetNext(m_sessionCursor);
            listing = next;
        } else {
            listing = NULL;
        }
    }
}

RVA(0x00178b00, 0x78)
i32 CNetMgr::ReadSessionSelection(HWND hList) {
    if (hList == NULL) {
        return 0;
    }
    i32 selection = static_cast<i32>(SendMessageA(hList, LB_GETCURSEL, 0, 0));
    if (selection == -1) {
        return 0;
    }
    if (selection < 0) {
        return 0;
    }
    if (selection >= static_cast<i32>(m_sessionListings.GetCount())) {
        return 0;
    }
    i32 itemData = static_cast<i32>(SendMessageA(hList, LB_GETITEMDATA, selection, 0));
    if (itemData == -1) {
        return 0;
    }
    if (itemData == 0) {
        return 0;
    }

    AddrWord<CNetSessionListNode> cookie;
    cookie.m_word = itemData;
    m_selectedSession = cookie.m_addr;
    return itemData;
}












RVA(0x00178b80, 0x13c)
CNetSessionListNode*
CNetMgr::CreateSession(
    i32 maxPlayers,
    char* sessionName,
    i32 applicationData,
    const char* password
) {
    DPSESSIONDESC2 buf;
    memset(&buf, 0, sizeof(buf));
    buf.dwSize = sizeof(buf);
    buf.dwFlags = DPSESSION_MIGRATEHOST | DPSESSION_KEEPALIVE
                    | DPSESSION_OPTIMIZELATENCY | DPSESSION_DIRECTPLAYPROTOCOL;
    buf.guidApplication = m_appGuid.m_guid;
    buf.dwMaxPlayers = maxPlayers;
    buf.lpszSessionNameA = sessionName;
    buf.dwUser1 = applicationData;
    if (password != NULL && *password != 0) {
        buf.lpszPasswordA = const_cast<char*>(password);
    }

    IDirectPlay4A* directPlay = m_directPlay;
    i32 hr = directPlay->Open(&buf, DPOPEN_CREATE);
    if (hr != 0) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x29e, hr, NULL);
        return NULL;
    }

    DWORD descriptionSize = 0;
    directPlay = m_directPlay;
    directPlay->GetSessionDesc(NULL, &descriptionSize);
    if (descriptionSize == 0) {
        return NULL;
    }
    u8* descriptionBytes = new u8[descriptionSize];
    if (descriptionBytes == NULL) {
        return NULL;
    }
    directPlay = m_directPlay;
    hr = directPlay->GetSessionDesc(descriptionBytes, &descriptionSize);
    if (hr != 0) {
        delete[] descriptionBytes;
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x2b1, hr, NULL);
        return NULL;
    }
    // byte-forced: the session description is variable length - the record and
    // then the strings it points into - so bytes are the allocation unit.
    CNetSessionListNode* listing =
        AddSessionListing(reinterpret_cast<LPDPSESSIONDESC2>(descriptionBytes));
    delete[] descriptionBytes;
    return listing;
}

RVA(0x00178cc0, 0x59)
CNetPlayerNode*
CNetMgr::JoinSessionAndCreatePlayer(
    CNetSessionListNode* session,
    const char* shortName,
    const char* longName,
    HANDLE eventHandle
) {
    if (session == NULL) {
        return NULL;
    }

    IDirectPlay4A* directPlay = m_directPlay;
    i32 hr = directPlay->Open(&session->m_sessionDesc, DPOPEN_JOIN);
    if (hr != 0) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x2dc, hr, NULL);
        return NULL;
    }
    return CreatePlayer(const_cast<char*>(shortName), longName, eventHandle);
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00178d20, 0x3e)
i32 CNetMgr::EnumerateAllPlayers() {
    ClearPlayers();

    IDirectPlay4A* directPlay = m_directPlay;
    i32 hr = directPlay->EnumPlayers(NULL, &NetEnumPlayerCallback, this, 0);
    if (hr != 0) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x30a, hr, NULL);
        return hr;
    }
    return 0;
}






RVA(0x00178d60, 0x73)
i32 CNetMgr::EnumerateSessionPlayers(CNetSessionListNode* session, DWORD flags) {
    ClearPlayers();



    GUID sessionGuid = session->m_sessionDesc.guidInstance;

    IDirectPlay4A* directPlay = m_directPlay;
    i32 hr = directPlay->EnumPlayers(&sessionGuid, &NetEnumPlayerCallback, this, flags);
    if (hr != 0) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x327, hr, NULL);
        return hr;
    }
    return 0;
}

RVA(0x00178de0, 0x30)
BOOL __stdcall NetEnumPlayerCallback(
    DPID playerId,
    DWORD playerType,
    LPCDPNAME name,
    DWORD flags,
    LPVOID context
) {
    CNetMgr* manager = static_cast<CNetMgr*>(context);
    if (manager == NULL) {
        return false;
    }
    manager->AddPlayer(playerId, name->lpszShortNameA, name->lpszLongNameA, flags);
    return true;
}















RVA(0x00178e10, 0x140)
CNetPlayerNode*
CNetMgr::AddPlayer(DPID playerId, const char* shortName, const char* longName, DWORD flags) {
    CNetPlayerNode* node = new CNetPlayerNode();

    if (node->Initialize(playerId, shortName, longName, flags) == 0) {
        delete node;
        return NULL;
    }

    i32 hr = m_directPlay->SetPlayerData(node->m_playerId, &node, 4, DPSET_LOCAL);
    if (hr != 0) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x36c, hr, NULL);
    } else {
        __POSITION* pos =
            static_cast<__POSITION*>(m_players.AddTail(static_cast<CObject*>(node)));
        if (pos == NULL) {
            delete node;
            return NULL;
        }
        node->m_listPosition = pos;
        return node;
    }
    delete node;
    return NULL;
}

RVA(0x00178f50, 0x3d)
void CNetMgr::ClearPlayers() {
    POSITION pos = m_players.GetHeadPosition();
    while (pos != NULL) {

        delete static_cast<CNetPlayerNode*>(m_players.GetNext(pos));
    }
    m_players.RemoveAll();
    m_playerCursor = NULL;
    m_selectedPlayer = NULL;
}












RVA(0x00178f90, 0x8b)
CNetPlayerNode*
CNetMgr::CreatePlayer(char* shortName, const char* longName, HANDLE eventHandle) {
    DPID playerId;
    DPNAME name;
    memset(&name, 0, sizeof(name));
    name.dwSize = sizeof(name);
    name.lpszShortNameA = shortName;
    name.lpszLongNameA = const_cast<char*>(longName);

    IDirectPlay4A* directPlay = m_directPlay;
    i32 hr = directPlay->CreatePlayer(&playerId, &name, eventHandle, NULL, 0, 0);
    if (hr != 0) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x3bb, hr, NULL);
        return NULL;
    }
    return AddPlayer(playerId, shortName, longName, 0);
}















// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00179020, 0xdf)
void CNetMgr::PopulatePlayerList(HWND hList) {
    if (hList == NULL) {
        return;
    }

    SendMessageA(hList, LB_RESETCONTENT, 0, 0);

    m_playerCursor = m_players.GetHeadPosition();
    CNetPlayerNode* player =
        m_playerCursor != NULL ? static_cast<CNetPlayerNode*>(m_players.GetNext(m_playerCursor)) : NULL;

    while (player != NULL) {
        MsgParam name;
        i32 itemIndex = static_cast<i32>(SendMessageA(
            hList,
            LB_ADDSTRING,
            0,
            (name.m_str = static_cast<const char*>(player->ShortName()), name.m_lparam)
        ));
        if (itemIndex != -1) {
            MsgParam cookie;
            cookie.m_player = player;
            SendMessageA(hList, LB_SETITEMDATA, itemIndex, cookie.m_lparam);
        }



        if (m_playerCursor != NULL) {
            CNetPlayerNode* next = static_cast<CNetPlayerNode*>(m_players.GetAt(m_playerCursor));
            m_players.GetNext(m_playerCursor);
            player = next;
        } else {
            player = NULL;
        }
    }
}

RVA(0x00179100, 0x33)
i32 CNetMgr::RemovePlayer(CNetPlayerNode* player) {
    if (player == NULL) {
        return 0;
    }

    __POSITION* pos = player->m_listPosition;
    delete player;
    if (pos != NULL) {
        m_players.RemoveAt(pos);
    }
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00179140, 0x23)
i32 CNetMgr::RemovePlayerById(DPID playerId) {
    CNetPlayerNode* player = GetPlayerNodeData(playerId);
    if (player != NULL) {
        return RemovePlayer(player);
    }
    return 0;
}

RVA(0x00179170, 0x20)
CNetPlayerNode* CNetMgr::FindPlayerById(DPID playerId) {
    POSITION pos = m_players.GetHeadPosition();
    while (pos != NULL) {
        CNetPlayerNode* entry = static_cast<CNetPlayerNode*>(m_players.GetNext(pos));
        if (entry->m_playerId == playerId) {
            return entry;
        }
    }
    return NULL;
}

RVA(0x00179190, 0x3f)
CNetPlayerNode* CNetMgr::GetPlayerNodeData(DPID playerId) {
    DWORD dataSize = 4;
    CNetPlayerNode* player = NULL;
    i32 hr = m_directPlay->GetPlayerData(playerId, &player, &dataSize, DPGET_LOCAL);
    return hr ? NULL : player;
}

RVA(0x001791d0, 0x5c)
i32 CNetMgr::Send(
    CNetPlayerNode* sender,
    CNetPlayerNode* recipient,
    DWORD flags,
    void* message,
    DWORD messageSize
) {
    DPID senderId = sender ? sender->m_playerId : 0;
    DPID recipientId = recipient ? recipient->m_playerId : 0;
    i32 hr = m_directPlay->Send(senderId, recipientId, flags, message, messageSize);
    if (hr) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x46d, hr, NULL);
    }
    return hr;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00179230, 0x63)
i32 CNetMgr::SendEx(
    DPID senderId,
    DPID recipientId,
    DWORD flags,
    LPVOID message,
    DWORD messageSize,
    DWORD priority,
    DWORD timeoutMs,
    LPVOID context,
    LPDWORD messageId
) {
    i32 hr = m_directPlay->SendEx(
        senderId,
        recipientId,
        flags,
        message,
        messageSize,
        priority,
        timeoutMs,
        context,
        messageId
    );
    if (hr && hr != static_cast<i32>(0x8000000a)) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x481, hr, NULL);
    }
    return hr;
}

RVA(0x001792a0, 0x44)
i32 CNetMgr::SendById(
    DPID senderId,
    DPID recipientId,
    DWORD flags,
    void* message,
    DWORD messageSize
) {
    i32 hr = m_directPlay->Send(senderId, recipientId, flags, message, messageSize);
    if (hr) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x492, hr, NULL);
    }
    return hr;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x001792f0, 0x78)
i32 CNetMgr::Receive(
    CNetPlayerNode* sender,
    CNetPlayerNode* recipient,
    DWORD flags,
    void* message,
    LPDWORD messageSize
) {
    DPID senderId = sender ? sender->m_playerId : 0;
    DPID recipientId = recipient ? recipient->m_playerId : 0;
    i32 hr = m_directPlay->Receive(&senderId, &recipientId, flags, message, messageSize);
    if (hr) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x4b7, hr, NULL);
    }
    return hr;
}

RVA(0x00179370, 0x4c)
i32 CNetMgr::BroadcastFrom(
    CNetPlayerNode* sender,
    DWORD flags,
    void* message,
    DWORD messageSize
) {
    DPID senderId = sender ? sender->m_playerId : 0;
    i32 hr = m_directPlay->Send(senderId, DPID_ALLPLAYERS, flags, message, messageSize);
    if (hr) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x4da, hr, NULL);
    }
    return hr;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x001793c0, 0x4f)
i32 CNetMgr::RemoveSessionListing(CNetSessionListNode* node) {
    if (node == NULL) {
        return 0;
    }
    if (m_selectedSession == node) {
        m_selectedSession = NULL;
    }
    m_directPlay->Close();
    __POSITION* pos = node->m_listPosition;
    delete node;
    if (pos != NULL) {
        m_sessionListings.RemoveAt(pos);
    }
    return 1;
}

RVA(0x00179410, 0x5d)
i32 CNetMgr::GetCaps(LPDPCAPS caps, DWORD flags) {
    if (caps == NULL) {
        return 0;
    }

    memset(caps, 0, sizeof(*caps));
    caps->dwSize = sizeof(*caps);
    i32 hr = m_directPlay->GetCaps(caps, flags);
    if (hr) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x52a, hr, NULL);
        return 0;
    }
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00179470, 0x84)
i32 CNetMgr::GetPlayerCaps(CNetPlayerNode* player, LPDPCAPS caps, DWORD flags) {
    if (!player) {
        return 0;
    }
    if (!player->m_playerId) {
        return 0;
    }
    if (!caps) {
        return 0;
    }
    memset(caps, 0, sizeof(*caps));
    caps->dwSize = sizeof(*caps);
    IDirectPlay4A* directPlay = m_directPlay;
    DPID playerId = player->m_playerId;
    i32 hr = directPlay->GetPlayerCaps(playerId, caps, flags);
    if (hr) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x553, hr, NULL);
        return 0;
    }
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00179500, 0x1d)
i32 CNetMgr::GetMaxPlayers() {
    DPCAPS caps;
    i32 ok = GetCaps(&caps, 0);
    return ok ? caps.dwMaxPlayers : 0;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00179520, 0x22)
i32 CNetMgr::GetConnectionLatency(DWORD flags) {







    DPCAPS caps;
    i32 ok = GetCaps(&caps, flags);
    return ok ? caps.dwLatency : 0;
}




// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00179550, 0x89)
CNetProviderNode* CNetMgr::FindProvider(i32 providerKind) {
    m_providerCursor = m_providers.GetHeadPosition();
    CNetProviderNode* provider =
        m_providerCursor != NULL ? static_cast<CNetProviderNode*>(m_providers.GetNext(m_providerCursor)) : NULL;
    while (provider) {
        switch (providerKind) {
            case NETPROVIDER_FIND_TCPIP:
                if (provider->IsTcpIpProvider()) {
                    return provider;
                }
                break;
            case NETPROVIDER_FIND_IPX:
                if (provider->IsIpxProvider()) {
                    return provider;
                }
                break;
            case NETPROVIDER_FIND_GENERIC:
                if (provider->MatchesUnclassifiedProvider()) {
                    return provider;
                }
                break;
        }

        if (m_providerCursor != NULL) {
            CNetProviderNode* next = static_cast<CNetProviderNode*>(m_providers.GetAt(m_providerCursor));
            m_providers.GetNext(m_providerCursor);
            provider = next;
        } else {
            provider = NULL;
        }
    }
    return NULL;
}











RVA(0x001795e0, 0x20)
CString CNetProviderNode::ProviderName() {
    return m_providerName;
}

RVA_COMPGEN(0x00179600, 0x1e, ??_GCNetProviderNode@@UAEPAXI@Z)
RVA(0x00179620, 0x48)
CNetProviderNode::~CNetProviderNode() {
    m_providerGuid = NULL;
    m_listPosition = NULL;
}

RVA_COMPGEN(0x00179670, 0x1e, ??_GCNetSessionListNode@@UAEPAXI@Z)

RVA(0x00179690, 0x46)
CNetSessionListNode::~CNetSessionListNode() {
    FreeSessionStrings();
}

RVA_COMPGEN(0x001796e0, 0x1e, ??_GCNetPlayerNode@@UAEPAXI@Z)

RVA(0x00179700, 0x8a)
CNetPlayerNode::~CNetPlayerNode() {
    m_playerId = 0;
    m_listPosition = NULL;
    if (m_ownedBufferA) {
        delete[] m_ownedBufferA;
    }
    m_ownedBufferA = NULL;
    if (m_ownedBufferB) {
        delete[] m_ownedBufferB;
    }
    m_ownedBufferB = NULL;
}
RVA(0x00179790, 0x21)
i32 CNetProviderNode::IsIpxProvider() {
    if (!m_providerGuid) {
        return 0;
    }
    return memcmp(m_providerGuid, g_directPlayIpxProviderGuid, 16) == 0 ? 1 : 0;
}

RVA(0x001797c0, 0x21)
i32 CNetProviderNode::IsTcpIpProvider() {
    if (!m_providerGuid) {
        return 0;
    }
    return memcmp(m_providerGuid, g_directPlayTcpIpProviderGuid, 16) == 0 ? 1 : 0;
}

RVA(0x001797f0, 0x21)
i32 CNetProviderNode::IsModemProvider() {
    if (!m_providerGuid) {
        return 0;
    }
    return memcmp(m_providerGuid, g_directPlayModemProviderGuid, 16) == 0 ? 1 : 0;
}

RVA(0x00179820, 0x21)
i32 CNetProviderNode::IsSerialProvider() {
    if (!m_providerGuid) {
        return 0;
    }
    return memcmp(m_providerGuid, g_directPlaySerialProviderGuid, 16) == 0 ? 1 : 0;
}

RVA(0x00179850, 0x21)
i32 CNetProviderNode::MatchesUnclassifiedProvider() {
    if (!m_providerGuid) {
        return 0;
    }
    return memcmp(m_providerGuid, g_unclassifiedProviderGuid, 16) == 0 ? 1 : 0;
}

RVA(0x00179880, 0xdb)
i32 CNetSessionListNode::Initialize(LPCDPSESSIONDESC2 sessionDesc) {
    if (!sessionDesc) {
        return 0;
    }
    memcpy(&m_sessionDesc, sessionDesc, sizeof(*sessionDesc));
    m_sessionDesc.dwSize = sizeof(m_sessionDesc);
    m_sessionDesc.lpszSessionNameA = NULL;
    m_sessionDesc.lpszPasswordA = NULL;
    if (sessionDesc->lpszSessionNameA && strlen(sessionDesc->lpszSessionNameA)) {
        m_sessionDesc.lpszSessionNameA = new char[strlen(sessionDesc->lpszSessionNameA) + 8];
        strcpy(m_sessionDesc.lpszSessionNameA, sessionDesc->lpszSessionNameA);
    }
    if (sessionDesc->lpszPasswordA && strlen(sessionDesc->lpszPasswordA)) {
        m_sessionDesc.lpszPasswordA = new char[strlen(sessionDesc->lpszPasswordA) + 8];
        strcpy(m_sessionDesc.lpszPasswordA, sessionDesc->lpszPasswordA);
    }
    return 1;
}

RVA(0x00179960, 0x3a)
void CNetSessionListNode::FreeSessionStrings() {
    if (m_sessionDesc.lpszSessionNameA) {
        delete[] m_sessionDesc.lpszSessionNameA;
        m_sessionDesc.lpszSessionNameA = NULL;
    }
    if (m_sessionDesc.lpszPasswordA) {
        delete[] m_sessionDesc.lpszPasswordA;
        m_sessionDesc.lpszPasswordA = NULL;
    }
    m_sessionDesc.dwSize = 0;
}

RVA(0x001799a0, 0x3f)
i32 CNetPlayerNode::Initialize(
    DPID playerId,
    const char* shortName,
    const char* longName,
    DWORD flags
) {
    m_playerId = playerId;
    m_shortName = shortName;
    m_longName = longName;
    m_flags = flags;
    m_ownedBufferA = NULL;
    m_reserved1c = 0;
    m_ownedBufferB = NULL;
    return 1;
}
