#include <Net/NetMgr.h>          // CNetMgr + DirectPlay/list node types (pulls <Mfc.h>, RezMgr)
#include <Net/NetGuids.h>        // g_guid1..g_guid5 (owner-only decl header)
#include <Net/InterfaceObject.h> // Find() returns the InterfaceObject group-node
#include <Font/Font.h> // CWapNodeB decl (a NetMgr node type, still homed here - 0x1794b0-0x179680
#include <rva.h>
#include <string.h> // memset (the inlined rep stos node/packet zeroing) + memcmp (IsInterfaceX)

extern "C" i32 __stdcall DirectPlayCreate(void* lpGUID, void* lplpDP, void* pUnk);
extern "C" i32 __stdcall DirectPlayEnumerate(void* lpEnumCallback, void* lpContext);

extern "C" {
VTBL(CNetPlayerListNode, 0x001f0760); // ??_7CNetPlayerListNode@@6B@ (5-slot CObject-derived)
VTBL(CNetSessionNode, 0x001f0778); // own (final) vtable
    DATA(0x002bf840)
    i32 g_spEnumValidated = 0; // 0x6bf840  (owner-TU definition, C linkage _g_spEnumValidated)
}

static i32 __stdcall EnumProviderCb(void* guid, char* name, u32 major, u32 minor, void* context);

// ---------------------------------------------------------------------------
// CNetMgr::InitFromProvider  (__thiscall; ret 0x14, 5 args).
// The service-provider Init variant: DirectPlayCreate's a fresh DirectPlay object
// for the selected group's GUID (a->+0x4) into m_releaseIface, then queries its
// IDirectPlay4 session interface (riid 0x5f0588) into m_directPlay. On any HRESULT
// it reports the error (NetMgr.cpp line 0x41 / 0x50) - the QueryInterface failure
// path also tears the manager down (Destroy) - and returns 0. On success it records
// the four caller setup dwords into the m_4 sub-object (+0x4..+0x10), latches the
// provider descriptor into m_groupSel, and zeroes the two other selection latches,
// then returns 1.
// @early-stop
// base-ptr materialization / regalloc plateau (~96.4%): the whole control flow, the
// DirectPlayCreate + QI(slot 0, riid 0x5f0588) sequence, both failure paths (incl.
// the Destroy tear-down) and the selection-latch zeroing are byte-exact; the only
// residual is the +0x4..+0x10 setup-dword block - retail materializes the base
// sub-object pointer (`lea eax,[esi+4]`) and assigns c/d->ecx/edx, where cl folds the
// base into esi-relative stores and assigns c/d->eax/ecx. Not source-steerable (a
// scheduling/addressing choice); §2a scoring-tail. Final sweep.
RVA(0x001780b0, 0xbb)
i32 CNetMgr::InitFromProvider(InterfaceObject* a, GUID appGuid) {
    GUID* guid = a->m_guid;
    if (guid == 0) {
        return 0;
    }
    i32 hr = DirectPlayCreate(guid, &m_releaseIface, 0);
    if (hr != 0 || m_releaseIface == 0) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x41, hr, 0);
        return 0;
    }
    IDirectPlay4Z* raw = reinterpret_cast<IDirectPlay4Z*>(m_releaseIface);
    hr = raw->QueryInterface(static_cast<void*>(&g_netDirectPlayRiid), &m_directPlay);
    if (hr != 0) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x50, hr, 0);
        Destroy();
        return 0;
    }

    m_groupSelId = 0;
    m_playerSelId = 0;
    m_sessionSelId = 0;
    i32* base = reinterpret_cast<i32*>((reinterpret_cast<char*>(this) + 4));
    const i32* g =
        reinterpret_cast<const i32*>(&appGuid); // the app GUID's 4 dwords -> the m_4 setup block
    base[0] = g[0];
    m_groupSel = a;
    m_playerSel = 0;
    base[1] = g[1];
    m_sessionSel = 0;
    base[2] = g[2];
    base[3] = g[3];
    return 1;
}

// ---------------------------------------------------------------------------
// CNetMgr::Init  (__thiscall).
// Brings the supplied DirectPlay interface online: opens it (slot 3, args
// 0/&iface/0) and on success queries its session interface into m_directPlay
// (slot 0 with the static riid 0x5f0588). On any HRESULT it reports the error
// (NetMgr.cpp line 0x78 / 0x81) and tears the manager down (Destroy), returning
// 0. On success it records the four caller setup dwords into the m_4 sub-object
// (+0x4..+0x10) and zeroes the three list-box selection latch/id pairs, then 1.
// @early-stop
// regalloc/spill wall (~80%): the Open(slot 3) + QueryInterface(slot 0, riid
// 0x5f0588) sequence, both ReportError+Destroy failure paths and the +0x4..+0x10
// store + selection-latch zeroing are all reproduced, but retail pins this->esi,
// the COM iface arg->edi and the 0 constant->ebx (callee-saved across the two COM
// calls) where cl spills the iface to a stack slot and reloads it; the register
// assignment is not steerable from C source. Final sweep.
RVA(0x00178170, 0xba)
i32 CNetMgr::Init(void* a, GUID appGuid) {
    IDirectPlay4Z* iface = static_cast<IDirectPlay4Z*>(a);
    void* out = a;
    i32 hr = iface->Open(0, &out, 0);
    if (hr != 0) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x78, hr, 0);
        Destroy();
        return 0;
    }
    hr = iface->QueryInterface(g_netDirectPlayRiid, &m_directPlay);
    if (hr != 0) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x81, hr, 0);
        Destroy();
        return 0;
    }

    m_groupSelId = 0;
    m_playerSelId = 0;
    m_sessionSelId = 0;
    i32* base = reinterpret_cast<i32*>((reinterpret_cast<char*>(this) + 4));
    const i32* g =
        reinterpret_cast<const i32*>(&appGuid); // the app GUID's 4 dwords -> the m_4 setup block
    base[0] = g[0];
    m_groupSel = 0;
    m_playerSel = 0;
    base[1] = g[1];
    m_sessionSel = 0;
    base[2] = g[2];
    base[3] = g[3];
    return 1;
}

RVA(0x00178230, 0x49)
void CNetMgr::Destroy() {
    ClearGroupList();
    ClearPlayerList();
    ClearSessionList();

    if (m_releaseIface != 0) {
        m_releaseIface->Release();
        m_releaseIface = 0;
    }
    // The DirectPlay interface releases through the same IUnknown-shaped vtable
    // (Slot10 then a re-read + Release) - the same COM object, viewed as
    // INetReleasable. The reference re-reads m_directPlay before each call,
    // matching retail's reload of [this+0x18].
    INetReleasable*& dp = *reinterpret_cast<INetReleasable**>(&m_directPlay);
    if (dp != 0) {
        dp->Slot10();
        INetReleasable* again = dp;
        again->Release();
        dp = 0;
    }
}

RVA(0x00178280, 0x43)
i32 CNetMgr::EnumServiceProviders(i32 validated) {
    ClearGroupList();

    g_spEnumValidated = validated;
    i32 hr = DirectPlayEnumerate(static_cast<void*>(&EnumProviderCb), this);
    if (hr != 0) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0xda, hr, 0);
        return hr;
    }
    return 0;
}

RVA(0x001782d0, 0x86)
static i32 __stdcall EnumProviderCb(void* guid, char* name, u32 major, u32 minor, void* context) {
    CNetMgr* self = static_cast<CNetMgr*>(context);
    if (self == 0) {
        return 0;
    }

    if (g_spEnumValidated == 0) {
        void* dp = 0;
        i32 hr = DirectPlayCreate(guid, &dp, 0);
        if (hr != 0) {
            CNetMgr::ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0xfe, hr, 0);
            return 1;
        }
        if (dp == 0) {
            return 1;
        }
        (static_cast<INetReleasable*>(dp))->Release();
    }

    return self->AddGroupNode(guid, name) != 0;
}

inline void* operator new(u32, void* p) {
    return p;
}

RVA(0x00178360, 0xc8)
InterfaceObject* CNetMgr::AddGroupNode(void* guid, void* name) {
    InterfaceObject* node = new InterfaceObject();

    if (guid == 0 || name == 0) {
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
    CGroupNode* node = reinterpret_cast<CGroupNode*>(m_groups.GetHeadPosition());
    while (node != 0) {
        CGroupNode* cur = node;
        node = node->m_next;
        // `delete`'s implicit null-guard IS retail's one test+je (then the virtual
        // scalar-deleting-dtor dispatch, push 1 / call [vtbl+4]).
        delete cur->m_data;
    }
    m_groups.RemoveAll();
    m_groupSelId = 0;
    m_groupSel = 0;
}

// ---------------------------------------------------------------------------
// CNetMgr::PopulateGroupList (0x178470, __thiscall, /GX) - re-homed from the ex
// LobbyGroupList.cpp holding TU (its CLobbyGroupMgr/LobbyNode/LobbyIface views were
// fake facets of CNetMgr/CNetListNode/InterfaceObject - src-claim + mid-cluster RVA
// proven). Fill the service-provider list box from the +0x1c group CObList: for each
// InterfaceObject payload, unless the caller's filter rejects it (flag bit1 -> drop
// IsInterface2, bit2 -> drop IsInterface1), add its GetName() text and stash the
// object pointer as the item data. The +0x7c cursor (m_groupSelId) latches the walk
// position (same slot Find reuses); the GetName CString temp gives the /GX EH frame.
// @early-stop
// regalloc/stack-slot coin-flip wall (docs/patterns/zero-register-pinning.md): every
// instruction, offset, callee, EH state and the inlined-advance-at-two-sites structure
// is faithful, but retail assigns this->edi/obj->esi and lays the CString/flags&1 temps
// one slot apart from cl's colouring (the correlated swaps have no source lever; an
// explicit flags&1 local made it WORSE). ~78.6%.
RVA(0x00178470, 0x11e)
void CNetMgr::PopulateGroupList(HWND hList, i32 flag) {
    if (hList == 0) {
        return;
    }
    SendMessageA(hList, LB_RESETCONTENT, 0, 0);

    CGroupNode* node = reinterpret_cast<CGroupNode*>(m_groups.GetHeadPosition());
    m_groupSelId = node;
    InterfaceObject* obj;
    if (node != 0) {
        m_groupSelId = node->m_next;
        obj = node->m_data;
    } else {
        obj = 0;
    }

    while (obj != 0) {
        if (((flag & 1) && obj->IsInterface2()) || ((flag & 2) && obj->IsInterface1())) {
            CGroupNode* cur = m_groupSelId;
            if (cur != 0) {
                obj = cur->m_data;
                m_groupSelId = cur->m_next;
            } else {
                obj = 0;
            }
        } else {
            i32 idx;
            {
                CString name = obj->GetName();
                idx = static_cast<i32>(SendMessageA(
                    hList,
                    LB_ADDSTRING,
                    0,
                    reinterpret_cast<LPARAM>(static_cast<LPCTSTR>(name))
                ));
            }
            if (idx != -1) {
                SendMessageA(hList, LB_SETITEMDATA, idx, reinterpret_cast<LPARAM>(obj));
            }
            CGroupNode* cur = m_groupSelId;
            if (cur != 0) {
                obj = cur->m_data;
                m_groupSelId = cur->m_next;
            } else {
                obj = 0;
            }
        }
    }
}

RVA(0x00178590, 0x78)
i32 CNetMgr::ReadGroupSel(void* hList) {
    if (hList == 0) {
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
    m_groupSel = reinterpret_cast<InterfaceObject*>(data); // the LB item data IS the group node
    return data;
}
extern "C" BOOL __stdcall
NetEnumPlayerCb(void* lpThisSD, void* lpdwTimeout, DWORD dwFlags, CNetMgr* ctx);

// ---------------------------------------------------------------------------
// CNetMgr::EnumPlayersInto  (__thiscall).
// Re-enumerates the session's players: first clears the +0x38 player list, then
// builds a 0x50-byte session descriptor on the stack (dwSize + the session GUID
// copied from this+4) and fires the IDirectPlay4 EnumPlayers slot (+0x34) with the
// per-player callback (NetEnumPlayerCb), the two caller args, and `this` as the
// enum context. On a nonzero HRESULT it routes the error through the static
// diagnostic reporter (NetMgr.cpp:0x1c9) and returns it; otherwise returns 0.
// @early-stop
// stack-anchor/scheduling wall (~67.7%): logic, memset, dwSize, the GUID copy,
// the 6-arg COM call and ReportError are all reproduced - but MSVC anchors the
// 0x50 stack desc at esp+0xc (retail esp+0x8) and interleaves the GUID stores
// with the arg pushes differently. struct-vs-raw-buffer and base-ptr GUID-copy
// levers did not move it; see docs/patterns/stack-buffer-size-drives-frame.md +
// statement-schedule-faithful.md. Deferred to the final sweep.
RVA(0x00178610, 0x8c)
i32 CNetMgr::EnumPlayersInto(void* a, void* b) {
    ClearPlayerList();

    char desc[0x50];
    memset(desc, 0, 0x50);
    i32* guid = reinterpret_cast<i32*>((reinterpret_cast<char*>(this) + 4));
    *reinterpret_cast<i32*>((desc + 0x00)) = 0x50;
    *reinterpret_cast<i32*>((desc + 0x18)) = guid[0];
    *reinterpret_cast<i32*>((desc + 0x1c)) = guid[1];
    *reinterpret_cast<i32*>((desc + 0x20)) = guid[2];
    *reinterpret_cast<i32*>((desc + 0x24)) = guid[3];

    IDirectPlay4Z* com = m_directPlay;
    i32 hr = com->EnumPlayers(desc, a, static_cast<void*>(&NetEnumPlayerCb), this, b);
    if (hr) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x1c9, hr, 0);
        return hr;
    }
    return 0;
}

RVA(0x001786a0, 0x2a)
extern "C" BOOL __stdcall
NetEnumPlayerCb(void* lpThisSD, void* lpdwTimeout, DWORD dwFlags, CNetMgr* ctx) {
    if (ctx != 0 && (dwFlags & 1) == 0 && lpThisSD != 0) {
        ctx->AddPlayerNode(lpThisSD);
        return TRUE;
    }
    return FALSE;
}

// ---------------------------------------------------------------------------
// CNetMgr::AddPlayerNode  (__thiscall).
// Adds one enumerated player to the +0x38 player list. No-op (0) on a null
// descriptor. RezAlloc's a 0x58-byte node (vptr 0x5f0760, body zeroed),
// inits it from the descriptor (copies the 0x50-byte player desc in and trims
// its name); if the init fails it self-destructs the node and returns 0,
// otherwise AddTail's it onto the +0x38 CObList and caches the position at +0x54.
// @early-stop
// regalloc wall (~92%): the `new` node (real-polymorphic ctor: coalesced vptr
// stamp + zero-loop), the Init call with the delete-on-fail, and the
// AddTail-into-+0x38 are all byte-aligned, but retail keeps playerDesc->ebx /
// this->ebp where cl swaps them (ebp/ebx), and the vptr store / lea schedule one
// pair differently. Not steerable. Final sweep.
RVA(0x001786d0, 0x77)
CNetPlayerListNode* CNetMgr::AddPlayerNode(void* playerDesc) {
    if (playerDesc == 0) {
        return 0;
    }

    CNetPlayerListNode* node = new CNetPlayerListNode();

    if (node->Init(static_cast<CNetSessionDesc*>(playerDesc)) == 0) {
        delete node;
        return 0;
    }

    node->m_54 = static_cast<__POSITION*>(m_players.AddTail(static_cast<CObject*>(node)));
    return node;
}

RVA(0x00178750, 0x3d)
void CNetMgr::ClearPlayerList() {
    CNetListNode* node = reinterpret_cast<CNetListNode*>(m_players.GetHeadPosition());
    while (node != 0) {
        CNetListNode* cur = node;
        node = node->m_next;
        delete cur->m_data; // implicit null-guard + virtual deleting-dtor dispatch
    }
    m_players.RemoveAll();
    m_playerSelId = 0;
    m_playerSel = 0;
}

// ---------------------------------------------------------------------------
// CNetMgr::PopulatePlayerList  (__thiscall).
// Refills a Win32 player list box from the +0x38 player CObList. No-op on a null
// handle. Resets the box (LB_RESETCONTENT) and walks the list (head at +0x3c,
// cursor cached in m_80): for each node's payload (+0x8) it adds the player name
// (payload+0x34) as a string (LB_ADDSTRING) and, if added, stashes the payload
// pointer as the item's data (LB_SETITEMDATA).
// @early-stop
// regalloc + node-walk schedule wall (~93%): the reset, the m_80-cursor walk, the
// LB_ADDSTRING/LB_SETITEMDATA pair and the per-node advance are all reproduced, but
// retail keeps this->ebp where cl uses ebx, and orders the loop-bottom field reads
// (payload then next) one step differently. Not steerable. Final sweep.
RVA(0x00178790, 0x89)
void CNetMgr::PopulatePlayerList(void* hList) {
    if (hList == 0) {
        return;
    }

    SendMessageA(static_cast<HWND>(hList), LB_RESETCONTENT, 0, 0);

    CNetListNode* node = reinterpret_cast<CNetListNode*>(m_players.GetHeadPosition());
    m_playerSelId = node;
    CNetPlayerListNode* payload;
    if (node != 0) {
        m_playerSelId = node->m_next;
        payload = node->m_data;
    } else {
        payload = 0;
    }

    while (payload != 0) {
        i32 r = static_cast<i32>(SendMessageA(
            static_cast<HWND>(hList),
            LB_ADDSTRING,
            0,
            reinterpret_cast<LPARAM>(payload->m_desc.m_lpszName)
        ));
        if (r != -1) {
            SendMessageA(
                static_cast<HWND>(hList),
                LB_SETITEMDATA,
                r,
                reinterpret_cast<LPARAM>(payload)
            );
        }
        CNetListNode* cur = m_playerSelId;
        if (cur != 0) {
            payload = cur->m_data;
            m_playerSelId = cur->m_next;
        } else {
            payload = 0;
        }
    }
}

RVA(0x00178820, 0x78)
i32 CNetMgr::ReadPlayerSel(void* hList) {
    if (hList == 0) {
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
    m_playerSel =
        reinterpret_cast<CNetPlayerListNode*>(data); // the LB item data IS the player node
    return data;
}

// ---------------------------------------------------------------------------
// CNetMgr::EnumGroupsInto  (__thiscall; ret 0x10, 4 args).
// Enumerates the players of a group into a fresh player node. Builds a 0x50-byte
// EnumGroups descriptor on the stack (dwSize 0x50, flags 0xa044, the session GUID
// copied from this+4, the four caller args at +0x28/+0x30/+0x34/+0x40 - the name
// arg only when it is a non-empty string) and fires the IDirectPlay4 EnumGroups slot
// (+0x60) with flag 2; reports a nonzero HRESULT (NetMgr.cpp line 0x29e). It then
// two-phase reads the group's player-data blob via GetPlayerData2 (+0x58): a size
// probe (in=0), an operator-new of that size, then the real read into the buffer -
// reporting a nonzero HRESULT (line 0x2b1) - and hands the blob to AddPlayerNode,
// freeing the blob (RezFree) on every exit.
// @early-stop
// stack-anchor / arg-slot-reuse plateau (~91%): the 0x50 desc build, the conditional
// name store, the EnumGroups(+0x60) call, the two-phase GetPlayerData2(+0x58) size
// probe + operator-new + read, both ReportError paths and the AddPlayerNode/RezFree
// tail are all reproduced, but retail anchors the 0x50 desc at a different esp offset
// and reuses the (dead) arg0 stack slot as the size in/out where cl spends a fresh
// local. Same family as EnumPlayersInto/EnumGroupsRange; stack-buffer-size-drives-
// frame.md. Final sweep.
RVA(0x001788a0, 0x13c)
CNetPlayerListNode* CNetMgr::EnumGroupsInto(void* a, void* b, i32 c, i32 d) {
    char buf[0x50];
    memset(buf, 0, 0x50);
    i32* guid = reinterpret_cast<i32*>((reinterpret_cast<char*>(this) + 4));
    *reinterpret_cast<i32*>((buf + 0x00)) = 0x50;
    *reinterpret_cast<i32*>((buf + 0x04)) = 0xa044;
    *reinterpret_cast<i32*>((buf + 0x18)) = guid[0];
    *reinterpret_cast<i32*>((buf + 0x1c)) = guid[1];
    *reinterpret_cast<i32*>((buf + 0x20)) = guid[2];
    *reinterpret_cast<i32*>((buf + 0x24)) = guid[3];
    *reinterpret_cast<void**>((buf + 0x28)) = a;
    *reinterpret_cast<void**>((buf + 0x30)) = b;
    *reinterpret_cast<i32*>((buf + 0x40)) = c;
    if (d != 0 && *reinterpret_cast<char*>(d) != 0) {
        *reinterpret_cast<i32*>((buf + 0x34)) = d;
    }

    IDirectPlay4Z* iface = m_directPlay;
    i32 hr = iface->EnumGroups(buf, 2);
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
    void* blob = operator new(size);
    if (blob == 0) {
        return 0;
    }
    iface = m_directPlay;
    hr = iface->GetPlayerData2(blob, &size);
    if (hr != 0) {
        RezFree(blob);
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x2b1, hr, 0);
        return 0;
    }
    CNetPlayerListNode* r = AddPlayerNode(blob); // the node IS the nonzero result
    RezFree(blob);
    return r;
}

RVA(0x001789e0, 0x59)
i32 CNetMgr::EnumPlayersCb(void* a, i32 b, i32 c, i32 d) {
    if (a == 0) {
        return 0;
    }

    IDirectPlay4Z* iface = m_directPlay;
    i32 hr = iface->EnumGroups(reinterpret_cast<char*>(a) + 4, 1);
    if (hr != 0) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x2dc, hr, 0);
        return 0;
    }
    return CreatePlayer(reinterpret_cast<void*>(b), c, d);
}

RVA(0x00178a40, 0x3e)
i32 CNetMgr::EnumGroupsAll() {
    ClearSessionList();

    IDirectPlay4Z* iface = m_directPlay;
    i32 hr = iface->EnumGroupsCb(0, static_cast<void*>(&NetEnumCb), this, 0);
    if (hr != 0) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x30a, hr, 0);
        return hr;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// CNetMgr::EnumGroupsRange  (__thiscall).
// As EnumGroupsAll but seeds the enum descriptor from a caller record's +0xc
// quad (copied onto the stack) before the EnumGroups call (slot 0xc, NetEnumCb
// callback, `this` context); reports a nonzero HRESULT (NetMgr.cpp line 0x327).
// @early-stop
// stack-anchor/scheduling wall (~84%): the ClearSessionList, the 4-dword record
// copy onto the stack desc, the 5-arg EnumGroups(slot 0xc) call and the ReportError
// are all reproduced, but cl anchors the stack desc at a different esp offset and
// interleaves the four record-field loads with the arg pushes differently than
// retail. Same class as EnumPlayersInto. Final sweep.
RVA(0x00178a80, 0x73)
i32 CNetMgr::EnumGroupsRange(void* rec, i32 flags) {
    ClearSessionList();

    // rec is the player-list node (roster m_playerSel / the create-ctx record) -
    // the 4 dwords are its descriptor GUID
    i32* r =
        reinterpret_cast<i32*>(&(static_cast<CNetPlayerListNode*>(rec))->m_desc.m_guidInstance);
    i32 desc[4];
    desc[0] = r[0];
    desc[1] = r[1];
    desc[2] = r[2];
    desc[3] = r[3];

    IDirectPlay4Z* iface = m_directPlay;
    i32 hr = iface->EnumGroupsCb(desc, static_cast<void*>(&NetEnumCb), this, flags);
    if (hr != 0) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x327, hr, 0);
        return hr;
    }
    return 0;
}

RVA(0x00178b00, 0x30)
extern "C" BOOL __stdcall
NetEnumCb(u32 dpId, DWORD dwType, NetDPName* lpName, DWORD dwFlags, CNetMgr* ctx) {
    if (ctx == 0) {
        return FALSE;
    }
    ctx->AddSessionNode(dpId, lpName->lpszShortNameA, lpName->lpszLongNameA, dwFlags);
    return TRUE;
}

// ---------------------------------------------------------------------------
// CNetMgr::AddSessionNode  (__thiscall; /GX EH frame).
// Adds one enumerated session to the +0x54 list. RezAlloc's a 0x24-byte node
// (base-dtor vptr 0x5e8cb4 while its two CString members are constructed, final
// vptr 0x5f0778), inits its body (InitSession: id + two CStrings, zeroing
// +0x14/+0x18/+0x1c). Reads the session's data blob (GetData5 slot 0x74, args
// +0x4, &out, 4, 1) - reporting a nonzero HRESULT (NetMgr.cpp line 0x36c) - then
// AddTail's the node onto the +0x54 list, self-destructing it if AddTail fails
// and clearing its +0x20. The two CString temps' dtors run under the /GX frame.
// @early-stop
// /GX EH-cookie + frame-size wall (~69%): AddSessionNode is now the real 4-arg
// method - it forwards all four params (id, nameA, nameB, d) straight to
// InitSession (was a bogus (a,b,b,b)), and GetData5 reads the NEW node's own
// m_id into a local scalar (was ((CNetSessionNode*)a)->m_id + node).
// The node ctor, InitSession, GetData5 probe + ReportError and AddTail/delete tail
// all match. Residual: the /GX unwind funclet cookie immediate (push 0xb vs push 0,
// module-global index) and retail's 2-dword EH-state reserve (sub esp,8) vs our
// 1-dword (push ecx), a 4-byte frame delta that cascades the stack offsets. Final sweep.
RVA(0x00178b30, 0x140)
i32 CNetMgr::AddSessionNode(i32 id, const char* nameA, const char* nameB, i32 d) {
    CNetSessionNode* node = new CNetSessionNode();

    if (node->InitSession(id, nameA, nameB, d) != 0) {
        IDirectPlay4Z* iface = m_directPlay;
        i32 blob;
        i32 hr = iface->GetData5(node->m_id, &blob, 4, 1);
        if (hr != 0) {
            ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x36c, hr, 0);
        }
    }

    if (node != 0) {
        __POSITION* pos =
            static_cast<__POSITION*>(m_sessions.AddTail(static_cast<CObject*>(node)));
        if (pos == 0) {
            delete node;
        } else {
            node->m_listPosition = pos;
        }
    }
    return reinterpret_cast<i32>(node);
}

RVA(0x00178c70, 0x3d)
void CNetMgr::ClearSessionList() {
    CNetPlayerNode* node = reinterpret_cast<CNetPlayerNode*>(m_sessions.GetHeadPosition());
    while (node != 0) {
        CNetPlayerNode* cur = node;
        node = node->m_next;
        delete cur->m_8; // implicit null-guard + virtual deleting-dtor dispatch
    }
    m_sessions.RemoveAll();
    m_sessionSelId = 0;
    m_sessionSel = 0;
}

// ---------------------------------------------------------------------------
// CNetMgr::CreatePlayer  (__thiscall; ret 0xc, 3 args).
// Reads a session's player-data blob via the DirectPlay interface (slot 6) into a
// 0x10-byte descriptor (size 0x10, the two trailing args at +0x8/+0xc) plus a
// scalar output, passing the third arg through. On a nonzero HRESULT reports it
// (NetMgr.cpp line 0x3bb) and returns 0; on success hands the output plus the two
// trailing args to AddSessionNode.
// @early-stop
// stack-buffer-size-drives-frame wall (~83%): the 0x10 descriptor is now filled in
// retail order - desc = {0x10, 0, a, b} (was {0x10,0,b,c}), GetSessionDesc(slot 6)
// takes `c` as its scalar arg (was `a`), and the 4-arg AddSessionNode tail passes
// (out, a, b, 0). The full control flow, the GetSessionDesc probe, the ReportError
// failure path and the AddSessionNode call all match. Residual: retail overlaps the
// out-var onto a dead arg slot (frame 0x10 vs our 0x14) and materializes the zero
// once in eax to seed every zeroed local where our /O2 stores immediates. Final sweep.
RVA(0x00178cb0, 0x8b)
i32 CNetMgr::CreatePlayer(void* a, i32 b, i32 c) {
    i32 out = 0;
    i32 desc[4];
    desc[0] = 0x10;
    desc[1] = 0;
    desc[2] = reinterpret_cast<i32>(a);
    desc[3] = b;

    IDirectPlay4Z* iface = m_directPlay;
    i32 hr = iface->GetSessionDesc(&desc[0], &out, c, 0, 0);
    if (hr != 0) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x3bb, hr, 0);
        return 0;
    }
    return AddSessionNode(
        out,
        reinterpret_cast<const char*>(a),
        reinterpret_cast<const char*>(b),
        0
    );
}

// ---------------------------------------------------------------------------
// CNetMgr::PopulateSessionList  (__thiscall; /GX EH frame).
// Refills a Win32 session list box from the +0x54 session CObList. No-op on a
// null handle. Resets the box (LB_RESETCONTENT) and walks the list (head at
// +0x58, cursor cached in m_84): for each node's payload (+0x8) it fetches the
// session name by value (GetName -> a scoped CString temp, whose dtor runs under
// the /GX frame), adds it (LB_ADDSTRING) and, if added, stashes the payload as
// the item's data (LB_SETITEMDATA).
// @early-stop
// /GX EH-temp + regalloc wall (~77%): the reset, the m_84-cursor walk, the
// per-node GetName CString temp (with its /GX-framed dtor), the LB_ADDSTRING/
// LB_SETITEMDATA pair and the advance are all reproduced, but the EH-state cookie
// numbering and the this-register / loop-bottom schedule differ from retail. Final
// sweep.
RVA(0x00178d40, 0xdf)
void CNetMgr::PopulateSessionList(void* hList) {
    if (hList == 0) {
        return;
    }

    SendMessageA(static_cast<HWND>(hList), LB_RESETCONTENT, 0, 0);

    CNetPlayerNode* node = reinterpret_cast<CNetPlayerNode*>(m_sessions.GetHeadPosition());
    m_sessionSelId = node;
    CNetSessionNode* payload;
    if (node != 0) {
        m_sessionSelId = node->m_next;
        payload = node->m_8;
    } else {
        payload = 0;
    }

    while (payload != 0) {
        CString name = payload->GetName();
        i32 r = static_cast<i32>(SendMessageA(
            static_cast<HWND>(hList),
            LB_ADDSTRING,
            0,
            reinterpret_cast<LPARAM>(static_cast<const char*>(name))
        ));
        if (r != -1) {
            SendMessageA(
                static_cast<HWND>(hList),
                LB_SETITEMDATA,
                r,
                reinterpret_cast<LPARAM>(payload)
            );
        }
        CNetPlayerNode* cur = m_sessionSelId;
        if (cur != 0) {
            payload = cur->m_8;
            m_sessionSelId = cur->m_next;
        } else {
            payload = 0;
        }
    }
}

RVA(0x00178e20, 0x33)
i32 CNetMgr::RemovePlayerObj(CNetSessionNode* obj) {
    if (obj == 0) {
        return 0;
    }

    __POSITION* pos = obj->m_listPosition;
    delete obj; // virtual deleting-dtor dispatch (the retail push 1 / call [vtbl+4])
    if (pos != 0) {
        m_sessions.RemoveAt(pos);
    }
    return 1;
}

RVA(0x00178e60, 0x23)
i32 CNetMgr::RemovePlayerById(i32 id) {
    CNetSessionNode* obj = static_cast<CNetSessionNode*>(GetPlayerData(id));
    if (obj != 0) {
        return RemovePlayerObj(obj);
    }
    return 0;
}

RVA(0x00178e90, 0x20)
CNetSessionNode* CNetMgr::FindPlayerById(i32 id) {
    CNetPlayerNode* node = reinterpret_cast<CNetPlayerNode*>(m_sessions.GetHeadPosition());
    while (node != 0) {
        CNetPlayerNode* cur = node;
        node = node->m_next;
        CNetSessionNode* entry = cur->m_8;
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
    data = 0;
    size = 4;
    i32 hr = m_directPlay->GetData2(id, &data, &size, 1);
    return hr ? 0 : data;
}

RVA(0x00178ef0, 0x5c)
i32 CNetMgr::SetGroupData2(CNetSessionNode* a, CNetSessionNode* b, i32 c, i32 d, i32 e) {
    i32 ida = a ? a->m_id : 0;
    i32 idb = b ? b->m_id : 0;
    i32 hr = m_directPlay->SetData5(ida, idb, c, d, e);
    if (hr) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x46d, hr, 0);
    }
    return hr;
}

RVA(0x00178f50, 0x61)
i32 CNetMgr::SendEx(i32 a, i32 b, i32 c, i32 d, i32 e, i32 f, i32 g, i32 h, i32 i) {
    i32 hr = m_directPlay->SendEx(
        a,
        b,
        c,
        reinterpret_cast<void*>(d),
        e,
        f,
        g,
        reinterpret_cast<void*>(h),
        reinterpret_cast<i32*>(i)
    );
    if (hr && hr != static_cast<i32>(0x8000000a)) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x481, hr, 0);
    }
    return hr;
}

RVA(0x00178fc0, 0x44)
i32 CNetMgr::SetData(i32 a, i32 b, i32 c, i32 d, i32 e) {
    i32 hr = m_directPlay->SetData5(a, b, c, d, e);
    if (hr) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x492, hr, 0);
    }
    return hr;
}

RVA(0x00179010, 0x76)
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
i32 CNetMgr::SetGroupDataFrom(CNetSessionNode* a, i32 c, i32 d, i32 e) {
    i32 ida = a ? a->m_id : 0;
    i32 hr = m_directPlay->SetData5(ida, 0, c, d, e);
    if (hr) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x4da, hr, 0);
    }
    return hr;
}

RVA(0x001790e0, 0x4f)
i32 CNetMgr::RemovePlayerNode(CNetPlayerListNode* node) {
    if (node == 0) {
        return 0;
    }
    if (m_playerSel == node) {
        m_playerSel = 0;
    }
    m_directPlay->v04();
    __POSITION* pos = node->m_54;
    delete node;
    if (pos != 0) {
        m_players.RemoveAt(pos);
    }
    return 1;
}

RVA(0x00179130, 0x5d)
i32 CNetMgr::EnumSessions(void* desc, void* ctx) {
    if (desc == 0) {
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
    char desc[0x28];
    i32 ok = EnumSessions(desc, ctx);
    return ok ? *reinterpret_cast<i32*>((desc + 0x18)) : 0;
}

// @early-stop
// linked-list advance regalloc wall (~94.9%): the head-load + the kind switch +
// the three predicate calls are byte-exact; only the GetNext advance differs -
// retail conservatively reloads the +0x7c cursor into a 2nd register (edx) for
// the ->next read while routing ->data through eax, the recompile derefs both
// from one register. Aliasing-conservatism choice, not source-steerable. See
// docs/patterns/linked-list-walk-node-eax-rotation.md. Logic complete.
RVA(0x00179270, 0x89)
InterfaceObject* CNetMgr::Find(i32 kind) {
    CGroupNode* node = reinterpret_cast<CGroupNode*>(m_groups.GetHeadPosition());
    m_groupSelId = node;
    InterfaceObject* item;
    if (node) {
        m_groupSelId = node->m_next;
        item = node->m_data;
    } else {
        item = 0;
    }
    while (item) {
        switch (kind) {
            case 1:
                if (item->IsInterface2()) {
                    return item;
                }
                break;
            case 2:
                if (item->IsInterface1()) {
                    return item;
                }
                break;
            case 5:
                if (item->IsInterface5()) {
                    return item;
                }
                break;
        }
        CGroupNode* cur = m_groupSelId;
        if (cur) {
            item = cur->m_data;
            m_groupSelId = cur->m_next;
        } else {
            item = 0;
        }
    }
    return 0;
}

// ===========================================================================
// The NetMgr.cpp tail (0x1794b0-0x1796c0): the five service-provider GUID
// predicates + the node string-free, re-homed from src/Font/Font.cpp per the
// docs/exe-map/interval-dossiers.md calibration case (they precede ??0Font
// @0x179700 - NetMgr.cpp-obj code, glued to the font unit only by the seam).
// These five RVAs are the canonical <Net/InterfaceObject.h> InterfaceObject's
// methods (its m_4 GUID pointer + these predicates). @identity-TODO remains for
// CWapNodeB (a NetMgr node type, declared in <Font/Font.h> for now).
// ===========================================================================

// GUIDs for the DirectPlay service-provider interface checks (IsInterfaceX). Given
// EXTERNAL linkage (`extern const`) so cl emits a STABLE mangled name (internal `const`
// gets a non-deterministic $S-suffixed local name that shifts every build). clang mangles
// the const-array storage class as `Q` where cl5 uses `P`, so DATA() would miss; bind by
// the exact cl name via DATA_SYMBOL (authority-checked against netmgr.obj).
DATA_SYMBOL(0x00224d58, 0x0, ?g_guid1@@3PBEB)
DATA_SYMBOL(0x00224d68, 0x0, ?g_guid2@@3PBEB)
DATA_SYMBOL(0x00224d78, 0x0, ?g_guid3@@3PBEB)
DATA_SYMBOL(0x00224d88, 0x0, ?g_guid4@@3PBEB)
DATA_SYMBOL(0x00224d98, 0x0, ?g_guid5@@3PBEB)
// clang-format off
// decls in <Net/NetGuids.h> (owner-only) - definitions drop the `extern` keyword.
const u8 g_guid1[16] = {0x00, 0xc4, 0x5b, 0x68, 0x2c, 0x9d, 0xcf, 0x11,
                                   0xa9, 0xcd, 0x00, 0xaa, 0x00, 0x68, 0x86, 0xe3};
const u8 g_guid2[16] = {0xe0, 0x5e, 0xe9, 0x36, 0x77, 0x85, 0xcf, 0x11,
                                   0x96, 0x0c, 0x00, 0x80, 0xc7, 0x53, 0x4e, 0x82};
const u8 g_guid3[16] = {0x60, 0xa7, 0xea, 0x44, 0x68, 0xcb, 0xcf, 0x11,
                                   0x9c, 0x4e, 0x00, 0xa0, 0xc9, 0x05, 0x42, 0x5e};
const u8 g_guid4[16] = {0x60, 0x68, 0x1d, 0x0f, 0xd9, 0x88, 0xcf, 0x11,
                                   0x9c, 0x4e, 0x00, 0xa0, 0xc9, 0x05, 0x42, 0x5e};
const u8 g_guid5[16] = {0x00, 0xb4, 0x23, 0xd2, 0x7d, 0x0a, 0xd1, 0x11,
                                   0x90, 0xc3, 0x00, 0x60, 0x97, 0x72, 0x58, 0x40};

RVA(0x001794b0, 0x21)
i32 InterfaceObject::IsInterface1() {
    if (!m_guid) {
        return 0;
    }
    return memcmp(reinterpret_cast<const void*>(m_guid), g_guid1, 16) == 0 ? 1 : 0;
}

RVA(0x001794e0, 0x21)
i32 InterfaceObject::IsInterface2() {
    if (!m_guid) {
        return 0;
    }
    return memcmp(reinterpret_cast<const void*>(m_guid), g_guid2, 16) == 0 ? 1 : 0;
}

RVA(0x00179510, 0x21)
i32 InterfaceObject::IsInterface3() {
    if (!m_guid) {
        return 0;
    }
    return memcmp(reinterpret_cast<const void*>(m_guid), g_guid3, 16) == 0 ? 1 : 0;
}

RVA(0x00179540, 0x21)
i32 InterfaceObject::IsInterface4() {
    if (!m_guid) {
        return 0;
    }
    return memcmp(reinterpret_cast<const void*>(m_guid), g_guid4, 16) == 0 ? 1 : 0;
}

RVA(0x00179570, 0x21)
i32 InterfaceObject::IsInterface5() {
    if (!m_guid) {
        return 0;
    }
    return memcmp(reinterpret_cast<const void*>(m_guid), g_guid5, 16) == 0 ? 1 : 0;
}

RVA(0x00179680, 0x3a)
void CNetPlayerListNode::FreeStrings() {
    if (m_desc.m_lpszName) {
        operator delete(m_desc.m_lpszName);
        m_desc.m_lpszName = 0;
    }
    if (m_desc.m_lpszPassword) {
        operator delete(m_desc.m_lpszPassword);
        m_desc.m_lpszPassword = 0;
    }
    m_desc.m_dwSize = 0;
}

RVA(0x001796c0, 0x3f)
i32 CNetSessionNode::InitSession(i32 id, const char* nameA, const char* nameB, i32 d) {
    m_id = id;
    m_name = nameA;
    m_longName = nameB;
    m_10 = d;
    m_ownedBufferA = 0;
    m_1c = 0;
    m_ownedBufferB = 0;
    return 1;
}
