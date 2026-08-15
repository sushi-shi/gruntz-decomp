#include <rva.h>

#include <Net/NetMgr.h>

#include <AddrWord.h>
#include <Enums.h>
#include <Font/Font.h>
#include <MsgParam.h>
#include <Net/DPlaySessionFlags.h>
#include <Net/InterfaceObject.h>
#include <Net/NetGuids.h>
#include <Net/NetProviderFindKind.h>

#include <dplay.h>
#include <new>
#include <string.h>

DATA(0x002bf840)
i32 g_spEnumValidated = 0;

// clang-format off
DATA(0x00224d58)
u8 g_directPlayIpxProviderGuid[16] = {0x00, 0xc4, 0x5b, 0x68, 0x2c, 0x9d, 0xcf, 0x11,
                                   0xa9, 0xcd, 0x00, 0xaa, 0x00, 0x68, 0x86, 0xe3};
DATA(0x00224d68)
u8 g_directPlayTcpIpProviderGuid[16] = {0xe0, 0x5e, 0xe9, 0x36, 0x77, 0x85, 0xcf, 0x11,
                                   0x96, 0x0c, 0x00, 0x80, 0xc7, 0x53, 0x4e, 0x82};
DATA(0x00224d78)
u8 g_directPlayModemProviderGuid[16] = {0x60, 0xa7, 0xea, 0x44, 0x68, 0xcb, 0xcf, 0x11,
                                   0x9c, 0x4e, 0x00, 0xa0, 0xc9, 0x05, 0x42, 0x5e};
DATA(0x00224d88)
u8 g_directPlaySerialProviderGuid[16] = {0x60, 0x68, 0x1d, 0x0f, 0xd9, 0x88, 0xcf, 0x11,
                                   0x9c, 0x4e, 0x00, 0xa0, 0xc9, 0x05, 0x42, 0x5e};
DATA(0x00224d98)
u8 g_unclassifiedProviderGuid[16] = {0x00, 0xb4, 0x23, 0xd2, 0x7d, 0x0a, 0xd1, 0x11,
                                   0x90, 0xc3, 0x00, 0x60, 0x97, 0x72, 0x58, 0x40};











RVA(0x001780b0, 0xbb)
i32 CNetMgr::InitFromProvider(InterfaceObject* a, GUID appGuid) {
    GUID* guid = a->m_guid;
    if (guid == NULL) {
        return 0;
    }
    i32 hr = DirectPlayCreate(guid, &m_releaseIface, 0);
    if (hr != 0 || m_releaseIface == NULL) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x41, hr, 0);
        return 0;
    }




    // API-forced: COM QueryInterface out-param.
    hr = m_releaseIface->QueryInterface(IID_IDirectPlay4A, reinterpret_cast<void**>(&m_directPlay));
    if (hr != 0) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x50, hr, 0);
        Destroy();
        return 0;
    }

    m_groupSelId = NULL;
    m_playerSelId = NULL;
    m_sessionSelId = NULL;


    m_appGuid.m_guid = appGuid;
    m_groupSel = a;
    m_playerSel = NULL;
    m_sessionSel = NULL;
    return 1;
}

















RVA(0x00178170, 0xba)
i32 CNetMgr::Init(void* a, NetGuid appGuid) {
    IDirectPlay4Z* lobby = static_cast<IDirectPlay4Z*>(a);



    IDirectPlay4Z* opened = 0;
    i32 hr = lobby->Open(0, &opened, 0);
    if (hr != 0) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x78, hr, 0);
        Destroy();
        return 0;
    }
    hr = opened->QueryInterface(&IID_IDirectPlay4A, &m_directPlay);
    if (hr != 0) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x81, hr, 0);
        Destroy();
        return 0;
    }






    m_groupSelId = NULL;
    m_playerSelId = NULL;
    m_sessionSelId = NULL;
    m_appGuid.m_guid = appGuid.m_guid;
    m_groupSel = NULL;
    m_playerSel = NULL;
    m_sessionSel = NULL;
    return 1;
}

RVA(0x00178230, 0x49)
void CNetMgr::Destroy() {
    ClearGroupList();
    ClearPlayerList();
    ClearSessionList();

    if (m_releaseIface != NULL) {
        m_releaseIface->Release();
        m_releaseIface = NULL;
    }



    if (m_directPlay != NULL) {
        m_directPlay->v04();
        IDirectPlay4Z* again = m_directPlay;
        again->Release();
        m_directPlay = NULL;
    }
}

RVA(0x00178280, 0x43)
i32 CNetMgr::EnumServiceProviders(i32 validated) {
    ClearGroupList();

    g_spEnumValidated = validated;
    i32 hr = DirectPlayEnumerate(&EnumProviderCb, this);
    if (hr != 0) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0xda, hr, 0);
        return hr;
    }
    return 0;
}

RVA(0x001782d0, 0x86)
static i32 __stdcall
EnumProviderCb(GUID* lpGuid, char* lpName, DWORD dwMajor, DWORD dwMinor, void* lpContext) {
    CNetMgr* self = static_cast<CNetMgr*>(lpContext);
    if (self == NULL) {
        return 0;
    }

    if (g_spEnumValidated == 0) {
        IDirectPlay* dp = 0;
        i32 hr = DirectPlayCreate(lpGuid, &dp, 0);
        if (hr != 0) {
            CNetMgr::ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0xfe, hr, 0);
            return 1;
        }
        if (dp == NULL) {
            return 1;
        }
        dp->Release();
    }

    return self->AddGroupNode(lpGuid, lpName) != NULL;
}


RVA(0x00178360, 0xc8)
InterfaceObject* CNetMgr::AddGroupNode(void* guid, void* name) {
    InterfaceObject* node = new InterfaceObject();

    if (guid == NULL || name == NULL) {
        delete node;
        return 0;
    }

    node->m_guid = static_cast<GUID*>(guid);
    node->m_name = static_cast<const char*>(name);
    node->m_listPosition = m_groups.AddTail(static_cast<CObject*>(node));
    return node;
}

RVA(0x00178430, 0x3a)
void CNetMgr::ClearGroupList() {
    POSITION pos = m_groups.GetHeadPosition();
    while (pos != NULL) {


        delete static_cast<InterfaceObject*>(m_groups.GetNext(pos));
    }
    m_groups.RemoveAll();
    m_groupSelId = NULL;
    m_groupSel = NULL;
}










// @early-stop
RVA(0x00178470, 0x11e)
void CNetMgr::PopulateGroupList(HWND hList, i32 flag) {
    if (hList == NULL) {
        return;
    }
    SendMessageA(hList, LB_RESETCONTENT, 0, 0);

    m_groupSelId = m_groups.GetHeadPosition();
    InterfaceObject* obj =
        m_groupSelId != NULL ? static_cast<InterfaceObject*>(m_groups.GetNext(m_groupSelId)) : 0;

    while (obj != NULL) {
        if (((flag & 1) && obj->IsTcpIpProvider()) || ((flag & 2) && obj->IsIpxProvider())) {

            if (m_groupSelId != NULL) {
                InterfaceObject* next = static_cast<InterfaceObject*>(m_groups.GetAt(m_groupSelId));
                m_groups.GetNext(m_groupSelId);
                obj = next;
            } else {
                obj = NULL;
            }
        } else {



            MsgParam name;
            i32 idx = static_cast<i32>(SendMessageA(
                hList,
                LB_ADDSTRING,
                0,
                (name.m_str = static_cast<LPCTSTR>(obj->GetName()), name.m_lparam)
            ));
            if (idx != -1) {
                MsgParam cookie;
                cookie.m_interface = obj;
                SendMessageA(hList, LB_SETITEMDATA, idx, cookie.m_lparam);
            }
            if (m_groupSelId != NULL) {
                InterfaceObject* next = static_cast<InterfaceObject*>(m_groups.GetAt(m_groupSelId));
                m_groups.GetNext(m_groupSelId);
                obj = next;
            } else {
                obj = NULL;
            }
        }
    }
}

RVA(0x00178590, 0x78)
i32 CNetMgr::ReadGroupSel(void* hList) {
    if (hList == NULL) {
        return 0;
    }
    i32 sel = static_cast<i32>(SendMessageA(static_cast<HWND>(hList), LB_GETCURSEL, 0, 0));
    if (sel == -1) {
        return 0;
    }
    if (sel < 0) {
        return 0;
    }
    if (sel >= static_cast<i32>(m_groups.GetCount())) {
        return 0;
    }
    i32 data = static_cast<i32>(SendMessageA(static_cast<HWND>(hList), LB_GETITEMDATA, sel, 0));
    if (data == -1) {
        return 0;
    }
    if (data == 0) {
        return 0;
    }



    AddrWord<InterfaceObject> cookie;
    cookie.m_word = data;
    m_groupSel = cookie.m_addr;
    return data;
}









RVA(0x00178610, 0x8c)
i32 CNetMgr::EnumPlayersInto(void* a, void* b) {
    ClearPlayerList();

    CNetSessionDesc desc;
    memset(&desc, 0, sizeof(desc));
    desc.m_dwSize = sizeof(desc);
    desc.m_guidApplication = m_appGuid.m_guid;

    IDirectPlay4Z* com = m_directPlay;
    i32 hr = com->EnumPlayers(&desc, a, &NetEnumPlayerCb, this, b);
    if (hr) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x1c9, hr, 0);
        return hr;
    }
    return 0;
}

RVA(0x001786a0, 0x2a)
BOOL __stdcall NetEnumPlayerCb(void* lpThisSD, void* lpdwTimeout, DWORD dwFlags, CNetMgr* ctx) {
    if (ctx != NULL && (dwFlags & 1) == 0 && lpThisSD != NULL) {
        ctx->AddPlayerNode(lpThisSD);
        return TRUE;
    }
    return FALSE;
}








// @early-stop
RVA(0x001786d0, 0x77)
CNetPlayerListNode* CNetMgr::AddPlayerNode(void* playerDesc) {
    if (playerDesc == NULL) {
        return 0;
    }

    CNetPlayerListNode* node = new CNetPlayerListNode();

    if (node->Init(static_cast<CNetSessionDesc*>(playerDesc)) == 0) {
        delete node;
        return 0;
    }

    node->m_listPosition = static_cast<__POSITION*>(m_players.AddTail(static_cast<CObject*>(node)));
    return node;
}

RVA(0x00178750, 0x3d)
void CNetMgr::ClearPlayerList() {
    POSITION pos = m_players.GetHeadPosition();
    while (pos != NULL) {

        delete static_cast<CNetPlayerListNode*>(m_players.GetNext(pos));
    }
    m_players.RemoveAll();
    m_playerSelId = NULL;
    m_playerSel = NULL;
}








RVA(0x00178790, 0x89)
void CNetMgr::PopulatePlayerList(void* hList) {
    if (hList == NULL) {
        return;
    }

    SendMessageA(static_cast<HWND>(hList), LB_RESETCONTENT, 0, 0);

    m_playerSelId = m_players.GetHeadPosition();
    CNetPlayerListNode* payload =
        m_playerSelId != NULL ? static_cast<CNetPlayerListNode*>(m_players.GetNext(m_playerSelId)) : 0;

    while (payload != NULL) {
        MsgParam name;
        i32 r = static_cast<i32>(SendMessageA(
            static_cast<HWND>(hList),
            LB_ADDSTRING,
            0,
            (name.m_str = payload->m_desc.m_lpszName, name.m_lparam)
        ));
        if (r != -1) {
            MsgParam cookie;
            cookie.m_player = payload;
            SendMessageA(static_cast<HWND>(hList), LB_SETITEMDATA, r, cookie.m_lparam);
        }



        if (m_playerSelId != NULL) {
            CNetPlayerListNode* next =
                static_cast<CNetPlayerListNode*>(m_players.GetAt(m_playerSelId));
            m_players.GetNext(m_playerSelId);
            payload = next;
        } else {
            payload = NULL;
        }
    }
}

RVA(0x00178820, 0x78)
i32 CNetMgr::ReadPlayerSel(void* hList) {
    if (hList == NULL) {
        return 0;
    }
    i32 sel = static_cast<i32>(SendMessageA(static_cast<HWND>(hList), LB_GETCURSEL, 0, 0));
    if (sel == -1) {
        return 0;
    }
    if (sel < 0) {
        return 0;
    }
    if (sel >= static_cast<i32>(m_players.GetCount())) {
        return 0;
    }
    i32 data = static_cast<i32>(SendMessageA(static_cast<HWND>(hList), LB_GETITEMDATA, sel, 0));
    if (data == -1) {
        return 0;
    }
    if (data == 0) {
        return 0;
    }

    AddrWord<CNetPlayerListNode> cookie;
    cookie.m_word = data;
    m_playerSel = cookie.m_addr;
    return data;
}












RVA(0x001788a0, 0x13c)
CNetPlayerListNode*
CNetMgr::EnumGroupsInto(i32 maxPlayers, char* sessionName, i32 user1, const char* password) {
    CNetSessionDesc buf;
    memset(&buf, 0, sizeof(buf));
    buf.m_dwSize = sizeof(buf);
    buf.m_dwFlags = DPSESSION_MIGRATEHOST | DPSESSION_KEEPALIVE
                    | DPSESSION_OPTIMIZELATENCY | DPSESSION_DIRECTPLAYPROTOCOL;
    buf.m_guidApplication = m_appGuid.m_guid;
    buf.m_dwMaxPlayers = maxPlayers;
    buf.m_lpszName = sessionName;
    buf.m_dwUser1 = user1;
    if (password != NULL && *password != 0) {
        buf.m_lpszPassword = const_cast<char*>(password);
    }

    IDirectPlay4Z* iface = m_directPlay;
    i32 hr = iface->EnumGroups(&buf, 2);
    if (hr != 0) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x29e, hr, 0);
        return 0;
    }

    i32 size = 0;
    iface = m_directPlay;
    iface->GetPlayerData2(0, &size);
    if (size == 0) {
        return 0;
    }
    u8* blob = new u8[size];
    if (blob == NULL) {
        return 0;
    }
    iface = m_directPlay;
    hr = iface->GetPlayerData2(blob, &size);
    if (hr != 0) {
        delete[] blob;
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x2b1, hr, 0);
        return 0;
    }
    CNetPlayerListNode* r = AddPlayerNode(blob);
    delete[] blob;
    return r;
}

RVA(0x001789e0, 0x59)
CNetSessionNode*
CNetMgr::EnumPlayersCb(CNetPlayerListNode* a, const char* b, const char* c, i32 d) {
    if (a == NULL) {
        return 0;
    }

    IDirectPlay4Z* iface = m_directPlay;
    i32 hr = iface->EnumGroups(&a->m_desc, 1);
    if (hr != 0) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x2dc, hr, 0);
        return 0;
    }
    return CreatePlayer(const_cast<char*>(b), c, d);
}

RVA(0x00178a40, 0x3e)
i32 CNetMgr::EnumGroupsAll() {
    ClearSessionList();

    IDirectPlay4Z* iface = m_directPlay;
    i32 hr = iface->EnumGroupsCb(0, &NetEnumCb, this, 0);
    if (hr != 0) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x30a, hr, 0);
        return hr;
    }
    return 0;
}






RVA(0x00178a80, 0x73)
i32 CNetMgr::EnumGroupsRange(void* rec, i32 flags) {
    ClearSessionList();



    GUID desc = (static_cast<CNetPlayerListNode*>(rec))->m_desc.m_guidInstance;

    IDirectPlay4Z* iface = m_directPlay;
    i32 hr = iface->EnumGroupsCb(&desc, &NetEnumCb, this, flags);
    if (hr != 0) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x327, hr, 0);
        return hr;
    }
    return 0;
}

RVA(0x00178b00, 0x30)
BOOL __stdcall NetEnumCb(u32 dpId, DWORD dwType, NetDPName* lpName, DWORD dwFlags, CNetMgr* ctx) {
    if (ctx == NULL) {
        return FALSE;
    }
    ctx->AddSessionNode(dpId, lpName->lpszShortNameA, lpName->lpszLongNameA, dwFlags);
    return TRUE;
}















// @early-stop
RVA(0x00178b30, 0x140)
CNetSessionNode* CNetMgr::AddSessionNode(i32 id, const char* nameA, const char* nameB, i32 flags) {
    CNetSessionNode* node = new CNetSessionNode();

    if (node->InitSession(id, nameA, nameB, flags) != 0) {
        i32 hr = m_directPlay->GetData5(node->m_id, &node, 4, 1);
        if (hr != 0) {



            ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x36c, hr, 0);
        } else {
            __POSITION* pos =
                static_cast<__POSITION*>(m_sessions.AddTail(static_cast<CObject*>(node)));


            if (pos == NULL) {
                delete node;
                return 0;
            }
            node->m_listPosition = pos;
            return node;
        }
    }
    delete node;
    return 0;
}

RVA(0x00178c70, 0x3d)
void CNetMgr::ClearSessionList() {
    POSITION pos = m_sessions.GetHeadPosition();
    while (pos != NULL) {

        delete static_cast<CNetSessionNode*>(m_sessions.GetNext(pos));
    }
    m_sessions.RemoveAll();
    m_sessionSelId = NULL;
    m_sessionSel = NULL;
}












RVA(0x00178cb0, 0x8b)
CNetSessionNode* CNetMgr::CreatePlayer(char* a, const char* b, i32 c) {
    i32 id;
    NetDPName desc;
    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.lpszShortNameA = a;
    desc.lpszLongNameA = const_cast<char*>(b);

    IDirectPlay4Z* iface = m_directPlay;
    i32 hr = iface->CreatePlayer(&id, &desc, c, 0, 0, 0);
    if (hr != 0) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x3bb, hr, 0);
        return 0;
    }
    return AddSessionNode(id, a, b, 0);
}















RVA(0x00178d40, 0xdf)
void CNetMgr::PopulateSessionList(void* hList) {
    if (hList == NULL) {
        return;
    }

    SendMessageA(static_cast<HWND>(hList), LB_RESETCONTENT, 0, 0);

    m_sessionSelId = m_sessions.GetHeadPosition();
    CNetSessionNode* payload =
        m_sessionSelId != NULL ? static_cast<CNetSessionNode*>(m_sessions.GetNext(m_sessionSelId)) : 0;

    while (payload != NULL) {
        MsgParam name;
        i32 r = static_cast<i32>(SendMessageA(
            static_cast<HWND>(hList),
            LB_ADDSTRING,
            0,
            (name.m_str = static_cast<const char*>(payload->GetName()), name.m_lparam)
        ));
        if (r != -1) {
            MsgParam cookie;
            cookie.m_session = payload;
            SendMessageA(static_cast<HWND>(hList), LB_SETITEMDATA, r, cookie.m_lparam);
        }



        if (m_sessionSelId != NULL) {
            CNetSessionNode* next = static_cast<CNetSessionNode*>(m_sessions.GetAt(m_sessionSelId));
            m_sessions.GetNext(m_sessionSelId);
            payload = next;
        } else {
            payload = NULL;
        }
    }
}

RVA(0x00178e20, 0x33)
i32 CNetMgr::RemovePlayerObj(CNetSessionNode* obj) {
    if (obj == NULL) {
        return 0;
    }

    __POSITION* pos = obj->m_listPosition;
    delete obj;
    if (pos != NULL) {
        m_sessions.RemoveAt(pos);
    }
    return 1;
}

RVA(0x00178e60, 0x23)
i32 CNetMgr::RemovePlayerById(i32 id) {
    CNetSessionNode* obj = static_cast<CNetSessionNode*>(GetPlayerData(id));
    if (obj != NULL) {
        return RemovePlayerObj(obj);
    }
    return 0;
}

RVA(0x00178e90, 0x20)
CNetSessionNode* CNetMgr::FindPlayerById(i32 id) {
    POSITION pos = m_sessions.GetHeadPosition();
    while (pos != NULL) {
        CNetSessionNode* entry = static_cast<CNetSessionNode*>(m_sessions.GetNext(pos));
        if (entry->m_id == id) {
            return entry;
        }
    }
    return 0;
}

RVA(0x00178eb0, 0x3f)
void* CNetMgr::GetPlayerData(i32 id) {
    u32 size;
    void* data;
    data = NULL;
    size = 4;
    i32 hr = m_directPlay->GetData2(id, &data, &size, 1);
    return hr ? 0 : data;
}

RVA(0x00178ef0, 0x5c)
i32 CNetMgr::SetGroupData2(CNetSessionNode* a, CNetSessionNode* b, i32 c, void* data, i32 size) {
    i32 ida = a ? a->m_id : 0;
    i32 idb = b ? b->m_id : 0;
    i32 hr = m_directPlay->SetData5(ida, idb, c, data, size);
    if (hr) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x46d, hr, 0);
    }
    return hr;
}

RVA(0x00178f50, 0x63)
i32 CNetMgr::SendEx(
    i32 idFrom,
    i32 idTo,
    i32 flags,
    LPVOID lpData,
    i32 size,
    i32 pri,
    i32 timeout,
    LPVOID ctx,
    LPDWORD lpMsgId
) {
    i32 hr = m_directPlay->SendEx(idFrom, idTo, flags, lpData, size, pri, timeout, ctx, lpMsgId);
    if (hr && hr != static_cast<i32>(0x8000000a)) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x481, hr, 0);
    }
    return hr;
}

RVA(0x00178fc0, 0x44)
i32 CNetMgr::SetData(i32 a, i32 b, i32 c, void* data, i32 size) {
    i32 hr = m_directPlay->SetData5(a, b, c, data, size);
    if (hr) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x492, hr, 0);
    }
    return hr;
}

RVA(0x00179010, 0x78)
i32 CNetMgr::Receive(
    CNetSessionNode* from,
    CNetSessionNode* to,
    i32 flags,
    void* lpData,
    i32* lpSize
) {
    i32 idFrom = from ? from->m_id : 0;
    i32 idTo = to ? to->m_id : 0;
    i32 hr = m_directPlay->Receive(&idFrom, &idTo, flags, lpData, lpSize);
    if (hr) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x4b7, hr, 0);
    }
    return hr;
}

RVA(0x00179090, 0x4c)
i32 CNetMgr::SetGroupDataFrom(CNetSessionNode* a, i32 c, void* data, i32 size) {
    i32 ida = a ? a->m_id : 0;
    i32 hr = m_directPlay->SetData5(ida, 0, c, data, size);
    if (hr) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x4da, hr, 0);
    }
    return hr;
}

RVA(0x001790e0, 0x4f)
i32 CNetMgr::RemovePlayerNode(CNetPlayerListNode* node) {
    if (node == NULL) {
        return 0;
    }
    if (m_playerSel == node) {
        m_playerSel = NULL;
    }
    m_directPlay->v04();
    __POSITION* pos = node->m_listPosition;
    delete node;
    if (pos != NULL) {
        m_players.RemoveAt(pos);
    }
    return 1;
}

RVA(0x00179130, 0x5d)
i32 CNetMgr::EnumSessions(void* desc, void* ctx) {
    if (desc == NULL) {
        return 0;
    }

    memset(desc, 0, 0x28);
    *static_cast<i32*>(desc) = 0x28;
    i32 hr = m_directPlay->Enum2(desc, ctx);
    if (hr) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x52a, hr, 0);
        return 0;
    }
    return 1;
}

RVA(0x00179190, 0x84)
i32 CNetMgr::GetGroupInfo(CNetSessionNode* a, void* desc, i32 flags) {
    if (!a) {
        return 0;
    }
    if (!a->m_id) {
        return 0;
    }
    if (!desc) {
        return 0;
    }
    memset(desc, 0, 0x28);
    *static_cast<i32*>(desc) = 0x28;
    IDirectPlay4Z* dp = m_directPlay;
    i32 id = a->m_id;
    i32 hr = dp->GetGroupData(id, desc, flags);
    if (hr) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x553, hr, 0);
        return 0;
    }
    return 1;
}

RVA(0x00179240, 0x22)
i32 CNetMgr::EnumSessions2(void* ctx) {







    CNetCaps caps;
    i32 ok = EnumSessions(&caps, ctx);
    return ok ? caps.m_dwLatency : 0;
}




RVA(0x00179270, 0x89)
InterfaceObject* CNetMgr::Find(i32 kind) {
    m_groupSelId = m_groups.GetHeadPosition();
    InterfaceObject* item =
        m_groupSelId != NULL ? static_cast<InterfaceObject*>(m_groups.GetNext(m_groupSelId)) : 0;
    while (item) {
        switch (kind) {
            case NETPROVIDER_FIND_TCPIP:
                if (item->IsTcpIpProvider()) {
                    return item;
                }
                break;
            case NETPROVIDER_FIND_IPX:
                if (item->IsIpxProvider()) {
                    return item;
                }
                break;
            case NETPROVIDER_FIND_GENERIC:
                if (item->MatchesUnclassifiedProvider()) {
                    return item;
                }
                break;
        }

        if (m_groupSelId != NULL) {
            InterfaceObject* next = static_cast<InterfaceObject*>(m_groups.GetAt(m_groupSelId));
            m_groups.GetNext(m_groupSelId);
            item = next;
        } else {
            item = NULL;
        }
    }
    return 0;
}











RVA(0x00179300, 0x20)
CString InterfaceObject::GetName() {
    return m_name;
}

RVA_COMPGEN(0x00179320, 0x1e, ??_GInterfaceObject@@UAEPAXI@Z)
RVA(0x00179340, 0x48)
InterfaceObject::~InterfaceObject() {
    m_guid = NULL;
    m_listPosition = NULL;
}

RVA_COMPGEN(0x00179390, 0x1e, ??_GCNetPlayerListNode@@UAEPAXI@Z)

RVA(0x001793b0, 0x46)
CNetPlayerListNode::~CNetPlayerListNode() {
    FreeStrings();
}

RVA_COMPGEN(0x00179400, 0x1e, ??_GCNetSessionNode@@UAEPAXI@Z)

RVA(0x00179420, 0x8a)
CNetSessionNode::~CNetSessionNode() {
    m_id = 0;
    m_listPosition = NULL;
    if (m_ownedBufferA) {
        ::operator delete(m_ownedBufferA);
    }
    m_ownedBufferA = NULL;
    if (m_ownedBufferB) {
        ::operator delete(m_ownedBufferB);
    }
    m_ownedBufferB = NULL;
}
RVA(0x001794b0, 0x21)
i32 InterfaceObject::IsIpxProvider() {
    if (!m_guid) {
        return 0;
    }
    return memcmp(m_guid, g_directPlayIpxProviderGuid, 16) == 0 ? 1 : 0;
}

RVA(0x001794e0, 0x21)
i32 InterfaceObject::IsTcpIpProvider() {
    if (!m_guid) {
        return 0;
    }
    return memcmp(m_guid, g_directPlayTcpIpProviderGuid, 16) == 0 ? 1 : 0;
}

RVA(0x00179510, 0x21)
i32 InterfaceObject::IsModemProvider() {
    if (!m_guid) {
        return 0;
    }
    return memcmp(m_guid, g_directPlayModemProviderGuid, 16) == 0 ? 1 : 0;
}

RVA(0x00179540, 0x21)
i32 InterfaceObject::IsSerialProvider() {
    if (!m_guid) {
        return 0;
    }
    return memcmp(m_guid, g_directPlaySerialProviderGuid, 16) == 0 ? 1 : 0;
}

RVA(0x00179570, 0x21)
i32 InterfaceObject::MatchesUnclassifiedProvider() {
    if (!m_guid) {
        return 0;
    }
    return memcmp(m_guid, g_unclassifiedProviderGuid, 16) == 0 ? 1 : 0;
}

RVA(0x001795a0, 0xdb)
i32 CNetPlayerListNode::Init(CNetSessionDesc* src) {
    if (!src) {
        return 0;
    }
    memcpy(&m_desc, src, sizeof(*src));
    m_desc.m_dwSize = sizeof(m_desc);
    m_desc.m_lpszName = NULL;
    m_desc.m_lpszPassword = NULL;
    if (src->m_lpszName && strlen(src->m_lpszName)) {
        m_desc.m_lpszName = new char[strlen(src->m_lpszName) + 8];
        strcpy(m_desc.m_lpszName, src->m_lpszName);
    }
    if (src->m_lpszPassword && strlen(src->m_lpszPassword)) {
        m_desc.m_lpszPassword = new char[strlen(src->m_lpszPassword) + 8];
        strcpy(m_desc.m_lpszPassword, src->m_lpszPassword);
    }
    return 1;
}

RVA(0x00179680, 0x3a)
void CNetPlayerListNode::FreeStrings() {
    if (m_desc.m_lpszName) {
        delete[] m_desc.m_lpszName;
        m_desc.m_lpszName = NULL;
    }
    if (m_desc.m_lpszPassword) {
        delete[] m_desc.m_lpszPassword;
        m_desc.m_lpszPassword = NULL;
    }
    m_desc.m_dwSize = 0;
}

RVA(0x001796c0, 0x3f)
i32 CNetSessionNode::InitSession(i32 id, const char* nameA, const char* nameB, i32 flags) {
    m_id = id;
    m_name = nameA;
    m_longName = nameB;
    m_enumFlags = flags;
    m_ownedBufferA = NULL;
    m_reserved1c = 0;
    m_ownedBufferB = NULL;
    return 1;
}
