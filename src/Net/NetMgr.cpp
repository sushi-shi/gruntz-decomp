#include <StdAfx.h>

#include <Ints.h>

#include <Net/NetMgr.h>

#include <ComOutRef.h>
#include <Enums.h>
#include <Font/Font.h>
#include <MsgParam.h>
#include <Net/DPlaySessionFlags.h>
#include <Net/NetGuids.h>
#include <Net/NetProviderFindKind.h>
#include <Net/NetProviderNode.h>
#include <SafeDelete.h>

#include <dplay.h>
#include <dplobby.h>
#include <objbase.h>
#include <string.h>
#include <windowsx.h>

b32 g_validateProviders = false;

GUID g_directPlayIpxProviderGuid = {
    0x685bc400, 0x9d2c, 0x11cf, {0xa9, 0xcd, 0x00, 0xaa, 0x00, 0x68, 0x86, 0xe3}};

GUID g_directPlayTcpIpProviderGuid = {
    0x36e95ee0, 0x8577, 0x11cf, {0x96, 0x0c, 0x00, 0x80, 0xc7, 0x53, 0x4e, 0x82}};

GUID g_directPlayModemProviderGuid = {
    0x44eaa760, 0xcb68, 0x11cf, {0x9c, 0x4e, 0x00, 0xa0, 0xc9, 0x05, 0x42, 0x5e}};

GUID g_directPlaySerialProviderGuid = {
    0x0f1d6860, 0x88d9, 0x11cf, {0x9c, 0x4e, 0x00, 0xa0, 0xc9, 0x05, 0x42, 0x5e}};

GUID g_unclassifiedProviderGuid = {
    0xd223b400, 0x0a7d, 0x11d1, {0x90, 0xc3, 0x00, 0x60, 0x97, 0x72, 0x58, 0x40}};

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

    m_providerCursor = m_providers.end();
    m_sessionCursor = m_sessionListings.end();
    m_playerCursor = m_players.end();

    m_appGuid = appGuid;
    m_selectedProvider = provider;
    m_selectedSession = NULL;
    m_selectedPlayer = NULL;
    return 1;
}

i32 CNetMgr::Initialize(void* lobbyIface, GUID appGuid) {
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

    m_providerCursor = m_providers.end();
    m_sessionCursor = m_sessionListings.end();
    m_playerCursor = m_players.end();
    m_appGuid = appGuid;
    m_selectedProvider = NULL;
    m_selectedSession = NULL;
    m_selectedPlayer = NULL;
    return 1;
}

void CNetMgr::Destroy() {
    ClearProviders();
    ClearSessionListings();
    ClearPlayers();

    SAFE_RELEASE(m_directPlayBase);

    if (m_directPlay != NULL) {
        m_directPlay->Close();
        IDirectPlay4A* again = m_directPlay;
        again->Release();
        m_directPlay = NULL;
    }
}

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

CNetProviderNode* CNetMgr::AddProvider(GUID* providerGuid, const char* providerName) {
    CNetProviderNode* node = new CNetProviderNode();

    if (providerGuid == NULL || providerName == NULL) {
        delete node;
        return NULL;
    }

    node->m_providerGuid = providerGuid;
    node->m_providerName = providerName ? providerName : "";
    node->m_listPosition = m_providers.insert(m_providers.end(), (node));
    return node;
}

void CNetMgr::ClearProviders() {
    std::list<CNetProviderNode*>::iterator pos = m_providers.begin();
    while (pos != m_providers.end()) {

        delete static_cast<CNetProviderNode*>(*(pos++));
    }
    m_providers.clear();
    m_providerCursor = m_providers.end();
    m_selectedProvider = NULL;
}

void CNetMgr::PopulateProviderList(HWND hList, i32 excludedProviderKinds) {
    if (hList == NULL) {
        return;
    }

    ListBox_ResetContent(hList);

    CNetProviderNode* provider = GetFirstProvider();
    while (provider != NULL) {
        if (((excludedProviderKinds & 1) && provider->IsTcpIpProvider())
            || ((excludedProviderKinds & 2) && provider->IsIpxProvider())) {
            provider = GetNextProvider();
        } else {
            i32 idx = ListBox_AddString(hList, (provider->ProviderName()).c_str());
            if (idx != LB_ERR) {
                ListBox_SetItemData(hList, idx, reinterpret_cast<LPARAM>(provider));
            }
            provider = GetNextProvider();
        }
    }
}

i32 CNetMgr::ReadProviderSelection(HWND hList) {
    if (hList == NULL) {
        return 0;
    }
    i32 selection = ListBox_GetCurSel(hList);
    if (selection == LB_ERR) {
        return 0;
    }
    if (selection < 0) {
        return 0;
    }
    if (selection >= static_cast<i32>(static_cast<i32>(m_providers.size()))) {
        return 0;
    }
    i32 itemData = static_cast<i32>(ListBox_GetItemData(hList, selection));
    if (itemData == LB_ERR) {
        return 0;
    }
    if (itemData == 0) {
        return 0;
    }

    m_selectedProvider = reinterpret_cast<CNetProviderNode*>(itemData);
    return itemData;
}

i32 CNetMgr::EnumerateSessions(DWORD timeoutMs, DWORD flags) {
    ClearSessionListings();

    DPSESSIONDESC2 desc;
    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.guidApplication = m_appGuid;

    IDirectPlay4A* directPlay = m_directPlay;
    i32 hr = directPlay->EnumSessions(&desc, timeoutMs, &NetEnumSessionCallback, this, flags);
    if (hr) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x1c9, hr, NULL);
        return hr;
    }
    return 0;
}

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

CNetSessionListNode* CNetMgr::AddSessionListing(LPCDPSESSIONDESC2 sessionDesc) {
    if (sessionDesc == NULL) {
        return NULL;
    }

    CNetSessionListNode* node = new CNetSessionListNode();

    if (node->Initialize(sessionDesc) == 0) {
        delete node;
        return NULL;
    }

    std::list<CNetSessionListNode*>::iterator pos = m_sessionListings.insert(m_sessionListings.end(), (node));
    node->m_listPosition = pos;
    return node;
}

void CNetMgr::ClearSessionListings() {
    std::list<CNetSessionListNode*>::iterator pos = m_sessionListings.begin();
    while (pos != m_sessionListings.end()) {

        delete static_cast<CNetSessionListNode*>(*(pos++));
    }
    m_sessionListings.clear();
    m_sessionCursor = m_sessionListings.end();
    m_selectedSession = NULL;
}

void CNetMgr::PopulateSessionList(HWND hList) {
    if (hList == NULL) {
        return;
    }

    ListBox_ResetContent(hList);

    CNetSessionListNode* listing = GetFirstSession();

    while (listing != NULL) {
        MsgParam name;
        i32 itemIndex = ListBox_AddString(
            hList,
            (name.m_str = listing->m_sessionDesc.lpszSessionNameA, name.m_lparam)
        );
        if (itemIndex != LB_ERR) {
            MsgParam cookie;
            cookie.m_sessionListing = listing;
            ListBox_SetItemData(hList, itemIndex, cookie.m_lparam);
        }

        listing = GetNextSession();
    }
}

i32 CNetMgr::ReadSessionSelection(HWND hList) {
    if (hList == NULL) {
        return 0;
    }
    i32 selection = ListBox_GetCurSel(hList);
    if (selection == LB_ERR) {
        return 0;
    }
    if (selection < 0) {
        return 0;
    }
    if (selection >= static_cast<i32>(static_cast<i32>(m_sessionListings.size()))) {
        return 0;
    }
    i32 itemData = static_cast<i32>(ListBox_GetItemData(hList, selection));
    if (itemData == LB_ERR) {
        return 0;
    }
    if (itemData == 0) {
        return 0;
    }

    m_selectedSession = reinterpret_cast<CNetSessionListNode*>(itemData);
    return itemData;
}

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
    buf.guidApplication = m_appGuid;
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

    CNetSessionListNode* listing =
        AddSessionListing(reinterpret_cast<LPDPSESSIONDESC2>(descriptionBytes));
    delete[] descriptionBytes;
    return listing;
}

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
    return CreatePlayer(shortName, longName, eventHandle);
}

i32 CNetMgr::EnumerateAllPlayers() {
    ClearPlayers();

    IDirectPlay4A* directPlay = m_directPlay;
    i32 hr = directPlay->EnumPlayers(NULL, &NetEnumPlayerCallback, this, DPENUMPLAYERS_ALL);
    if (hr != 0) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x30a, hr, NULL);
        return hr;
    }
    return 0;
}

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
        std::list<CNetPlayerNode*>::iterator pos =
            m_players.insert(m_players.end(), node);
        if (pos == m_players.end()) {
            delete node;
            return NULL;
        }
        node->m_listPosition = pos;
        return node;
    }
    delete node;
    return NULL;
}

void CNetMgr::ClearPlayers() {
    std::list<CNetPlayerNode*>::iterator pos = m_players.begin();
    while (pos != m_players.end()) {

        delete static_cast<CNetPlayerNode*>(*(pos++));
    }
    m_players.clear();
    m_playerCursor = m_players.end();
    m_selectedPlayer = NULL;
}

CNetPlayerNode*
CNetMgr::CreatePlayer(const char* shortName, const char* longName, HANDLE eventHandle) {
    DPID playerId;
    DPNAME name;
    memset(&name, 0, sizeof(name));
    name.dwSize = sizeof(name);
    name.lpszShortNameA = const_cast<char*>(shortName);
    name.lpszLongNameA = const_cast<char*>(longName);

    IDirectPlay4A* directPlay = m_directPlay;
    i32 hr = directPlay->CreatePlayer(&playerId, &name, eventHandle, NULL, 0, 0);
    if (hr != 0) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x3bb, hr, NULL);
        return NULL;
    }
    return AddPlayer(playerId, shortName, longName, 0);
}

void CNetMgr::PopulatePlayerList(HWND hList) {
    if (hList == NULL) {
        return;
    }

    ListBox_ResetContent(hList);

    CNetPlayerNode* player = GetFirstPlayer();

    while (player != NULL) {
        MsgParam name;
        i32 itemIndex = ListBox_AddString(
            hList,
            (name.m_str = player->m_shortName.c_str(), name.m_lparam)
        );
        if (itemIndex != LB_ERR) {
            MsgParam cookie;
            cookie.m_player = player;
            ListBox_SetItemData(hList, itemIndex, cookie.m_lparam);
        }

        player = GetNextPlayer();
    }
}

i32 CNetMgr::RemovePlayer(CNetPlayerNode* player) {
    if (player == NULL) {
        return 0;
    }

    std::list<CNetPlayerNode*>::iterator pos = std::find(m_players.begin(), m_players.end(), player);
    delete player;
    if (pos != m_players.end()) {
        if (m_playerCursor == pos) ++m_playerCursor;
        m_players.erase(pos);
    }
    return 1;
}

i32 CNetMgr::RemovePlayerById(DPID playerId) {
    CNetPlayerNode* player = GetPlayerNodeData(playerId);
    if (player != NULL) {
        return RemovePlayer(player);
    }
    return 0;
}

CNetPlayerNode* CNetMgr::FindPlayerById(DPID playerId) {
    std::list<CNetPlayerNode*>::iterator pos = m_players.begin();
    while (pos != m_players.end()) {
        CNetPlayerNode* entry = static_cast<CNetPlayerNode*>(*(pos++));
        if (entry->m_playerId == playerId) {
            return entry;
        }
    }
    return NULL;
}

CNetPlayerNode* CNetMgr::GetPlayerNodeData(DPID playerId) {
    CNetPlayerNode* player = NULL;
    DWORD dataSize = 4;
    i32 hr = m_directPlay->GetPlayerData(playerId, &player, &dataSize, DPGET_LOCAL);
    return hr ? NULL : player;
}

static inline DPID PlayerIdOf(CNetPlayerNode* player) {
    return player ? player->m_playerId : 0;
}

i32 CNetMgr::Send(
    CNetPlayerNode* sender,
    CNetPlayerNode* recipient,
    DWORD flags,
    void* message,
    DWORD messageSize
) {
    DPID senderId = PlayerIdOf(sender);
    DPID recipientId = PlayerIdOf(recipient);
    i32 hr = m_directPlay->Send(senderId, recipientId, flags, message, messageSize);
    if (hr) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x46d, hr, NULL);
    }
    return hr;
}

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
    if (hr && hr != DPERR_PENDING) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x481, hr, NULL);
    }
    return hr;
}

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

i32 CNetMgr::Receive(
    CNetPlayerNode* sender,
    CNetPlayerNode* recipient,
    DWORD flags,
    void* message,
    LPDWORD messageSize
) {
    DPID senderId = PlayerIdOf(sender);
    DPID recipientId = PlayerIdOf(recipient);
    i32 hr = m_directPlay->Receive(&senderId, &recipientId, flags, message, messageSize);
    if (hr) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x4b7, hr, NULL);
    }
    return hr;
}

i32 CNetMgr::BroadcastFrom(
    CNetPlayerNode* sender,
    DWORD flags,
    void* message,
    DWORD messageSize
) {
    DPID senderId = PlayerIdOf(sender);
    i32 hr = m_directPlay->Send(senderId, DPID_ALLPLAYERS, flags, message, messageSize);
    if (hr) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x4da, hr, NULL);
    }
    return hr;
}

i32 CNetMgr::RemoveSessionListing(CNetSessionListNode* node) {
    if (node == NULL) {
        return 0;
    }
    if (m_selectedSession == node) {
        m_selectedSession = NULL;
    }
    m_directPlay->Close();
    std::list<CNetSessionListNode*>::iterator pos = std::find(m_sessionListings.begin(), m_sessionListings.end(), node);
    delete node;
    if (pos != m_sessionListings.end()) {
        if (m_sessionCursor == pos) ++m_sessionCursor;
        m_sessionListings.erase(pos);
    }
    return 1;
}

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

i32 CNetMgr::GetMaxPlayers() {
    DPCAPS caps;
    i32 ok = GetCaps(&caps, 0);
    return ok ? caps.dwMaxPlayers : 0;
}

i32 CNetMgr::GetConnectionLatency(DWORD flags) {

    DPCAPS caps;
    i32 ok = GetCaps(&caps, flags);
    return ok ? caps.dwLatency : 0;
}

CNetProviderNode* CNetMgr::FindProvider(i32 providerKind) {
    CNetProviderNode* provider = GetFirstProvider();
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

        provider = GetNextProvider();
    }
    return NULL;
}

std::string CNetProviderNode::ProviderName() {
    return m_providerName;
}

CNetProviderNode::~CNetProviderNode() {
    m_providerGuid = NULL;

}

CNetSessionListNode::~CNetSessionListNode() {
    FreeSessionStrings();
}

CNetPlayerNode::~CNetPlayerNode() {
    m_playerId = 0;

    if (m_ownedBufferA) {
        delete[] m_ownedBufferA;
    }
    m_ownedBufferA = NULL;
    if (m_ownedBufferB) {
        delete[] m_ownedBufferB;
    }
    m_ownedBufferB = NULL;
}

i32 CNetProviderNode::IsIpxProvider() {
    if (!m_providerGuid) {
        return 0;
    }
    return IsEqualGUID(*m_providerGuid, g_directPlayIpxProviderGuid);
}

i32 CNetProviderNode::IsTcpIpProvider() {
    if (!m_providerGuid) {
        return 0;
    }
    return IsEqualGUID(*m_providerGuid, g_directPlayTcpIpProviderGuid);
}

i32 CNetProviderNode::IsModemProvider() {
    if (!m_providerGuid) {
        return 0;
    }
    return IsEqualGUID(*m_providerGuid, g_directPlayModemProviderGuid);
}

i32 CNetProviderNode::IsSerialProvider() {
    if (!m_providerGuid) {
        return 0;
    }
    return IsEqualGUID(*m_providerGuid, g_directPlaySerialProviderGuid);
}

i32 CNetProviderNode::MatchesUnclassifiedProvider() {
    if (!m_providerGuid) {
        return 0;
    }
    return IsEqualGUID(*m_providerGuid, g_unclassifiedProviderGuid);
}

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

void CNetSessionListNode::FreeSessionStrings() {
    SAFE_DELETE_ARRAY(m_sessionDesc.lpszSessionNameA);
    SAFE_DELETE_ARRAY(m_sessionDesc.lpszPasswordA);
    m_sessionDesc.dwSize = 0;
}

i32 CNetPlayerNode::Initialize(
    DPID playerId,
    const char* shortName,
    const char* longName,
    DWORD flags
) {
    m_playerId = playerId;
    m_shortName = shortName ? shortName : "";
    m_longName = longName ? longName : "";
    m_flags = flags;
    m_ownedBufferA = NULL;
    m_reserved1c = 0;
    m_ownedBufferB = NULL;
    return 1;
}
