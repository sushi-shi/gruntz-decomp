// NetMgr.cpp - the engine CNetMgr: the reusable DirectPlay session/lobby wrapper.
// This TU is retail C:\Proj\NetMgr\NetMgr.cpp - proven by the ReportError
// __FILE__/__LINE__ pairs, monotonic 0x41..0x553 through the 0x178xxx cluster.
// It is the small DirectPlay object: DirectPlayCreate/Open + QueryInterface into
// the IDirectPlay4 session, the service-provider / session / group / player
// enumeration + list-box population, the node factories (Add*Node onto the
// +0x1c/+0x38/+0x54 CObLists), and the raw Send/Receive/SetData/Find primitives.
//
// SPLIT NOTE (2026-07-10): the retail CNetMgr method set was reconstructed
// conflated with the Gruntz multiplayer game-state (the 0x0b5xxx-0x0bdxxx method
// cluster - really CMulti, RTTI CMulti:CPlay:CState) inside one god-TU. That
// game-side half now lives in src/Net/NetMgrGame.cpp (unit netmgrgame); this TU
// keeps only the real engine CNetMgr (the 0x178xxx DirectPlay cluster). The two
// blocks are separate retail objects (a ~0xbaf30 .text gap between them).
#include <Net/NetMgr.h>          // CNetMgr + DirectPlay/list node types (pulls <Mfc.h>, RezMgr)
#include <Net/NetGuids.h>        // g_guid1..g_guid5 (owner-only decl header)
#include <Net/InterfaceObject.h> // Find() returns the InterfaceObject group-node
#include <Font/Font.h> // CWapNodeB decl (a NetMgr node type, still homed here - 0x1794b0-0x179680
                       // NetMgr.cpp-tail bodies below; see the seam note there)
#include <rva.h>
#include <string.h> // memset (the inlined rep stos node/packet zeroing) + memcmp (IsInterfaceX)

// DirectPlay DLL imports (DPLAYX.dll ordinals), reached through the IAT jump
// thunks at 0x1937c0 / 0x1937c6 -> the `call rel32` reloc-masks. __stdcall.
//   DirectPlayCreate   (#1): create a DirectPlay object for a service-provider GUID
//   DirectPlayEnumerate(#2): enumerate the installed service providers
extern "C" i32 __stdcall DirectPlayCreate(void* lpGUID, void* lplpDP, void* pUnk);
extern "C" i32 __stdcall DirectPlayEnumerate(void* lpEnumCallback, void* lpContext);

// The "already-validated" gate the provider-enum callback reads: when nonzero the
// callback skips the DirectPlayCreate round-trip probe (0x6bf840).
extern "C" {
    DATA(0x002bf840)
    i32 g_spEnumValidated = 0; // 0x6bf840  (owner-TU definition, C linkage _g_spEnumValidated)
}

// The DirectPlayEnumerate callback (0x1782d0, DPENUMDPCALLBACK, __stdcall): its
// address is handed to DirectPlayEnumerate by EnumServiceProviders (below), so the
// forward decl lets that `push offset` reloc-mask.
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
i32 CNetMgr::InitFromProvider(void* a, GUID appGuid) {
    void* guid = *(void**)((char*)a + 4);
    if (guid == 0) {
        return 0;
    }
    i32 hr = DirectPlayCreate(guid, &m_releaseIface, 0);
    if (hr != 0 || m_releaseIface == 0) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x41, hr, 0);
        return 0;
    }
    IDirectPlay4Z* raw = (IDirectPlay4Z*)m_releaseIface;
    hr = raw->QueryInterface((void*)&g_netDirectPlayRiid, &m_directPlay);
    if (hr != 0) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x50, hr, 0);
        Destroy();
        return 0;
    }

    m_groupSelId = 0;
    m_playerSelId = 0;
    m_sessionSelId = 0;
    i32* base = (i32*)((char*)this + 4);
    const i32* g = (const i32*)&appGuid; // the app GUID's 4 dwords -> the m_4 setup block
    base[0] = g[0];
    m_groupSel = (i32)a;
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
i32 CNetMgr::Init(void* a, i32 c, i32 d, i32 e, i32 f) {
    IDirectPlay4Z* iface = (IDirectPlay4Z*)a;
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
    i32* base = (i32*)((char*)this + 4);
    base[0] = c;
    m_groupSel = 0;
    m_playerSel = 0;
    base[1] = d;
    m_sessionSel = 0;
    base[2] = e;
    base[3] = f;
    return 1;
}

// ---------------------------------------------------------------------------
// CNetMgr::Destroy  (__thiscall).
// Full session teardown: clears the three managed lists (+0x1c/+0x38/+0x54) via
// their clear-loops, then releases the two COM interfaces. The +0x14 interface
// is released through its vtable slot 2 (IUnknown::Release form); the +0x18
// IDirectPlay4 through slot 4 then slot 2. Both pointers are nulled after release.
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
    INetReleasable*& dp = *(INetReleasable**)&m_directPlay;
    if (dp != 0) {
        dp->Slot10();
        INetReleasable* again = dp;
        again->Release();
        dp = 0;
    }
}

// ---------------------------------------------------------------------------
// CNetMgr::EnumServiceProviders  (__thiscall; ret 0x4, 1 arg).
// Refills the +0x1c group list with the installed DirectPlay service providers:
// clears the group list, records the "already-validated" gate, then runs
// DirectPlayEnumerate with the per-provider callback and `this` as the context.
// On a nonzero HRESULT reports it (NetMgr.cpp line 0xda) and returns it, else 0.
RVA(0x00178280, 0x43)
i32 CNetMgr::EnumServiceProviders(i32 validated) {
    ClearGroupList();

    g_spEnumValidated = validated;
    i32 hr = DirectPlayEnumerate((void*)&EnumProviderCb, this);
    if (hr != 0) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0xda, hr, 0);
        return hr;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// EnumProviderCb  (DPENUMDPCALLBACK, __stdcall; ret 0x14, 5 args).
// One step of the service-provider enumeration. Stops (returns FALSE) on a null
// context. Unless the validated gate is set, it probes the provider by
// DirectPlayCreate'ing a throwaway object for its GUID: a create failure is
// reported (NetMgr.cpp line 0xfe) and the provider skipped (return TRUE); a
// created object is Released (slot 2) before use. It then adds the provider (GUID +
// name) as a group node (AddGroupNode) and returns whether that succeeded.
RVA(0x001782d0, 0x86)
static i32 __stdcall EnumProviderCb(void* guid, char* name, u32 major, u32 minor, void* context) {
    CNetMgr* self = (CNetMgr*)context;
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
        ((INetReleasable*)dp)->Release();
    }

    return self->AddGroupNode(guid, name) != 0;
}

// The global throwing operator new (NAFXCW, 0x1b9b46 == RezAlloc) is declared by
// <Mfc.h> (via NetMgr.h); placement new (used to build the group node in place)
// is declared here. Reloc-masked.
inline void* operator new(u32, void* p) {
    return p;
}

// ---------------------------------------------------------------------------
// CNetMgr::AddGroupNode  (__thiscall; ret 0x8, 2 args; /GX EH frame in retail).
// `new`-constructs the 0x10-byte DirectPlay group node - the canonical polymorphic
// InterfaceObject (own vtable ??_7InterfaceObject@0x5f0748, owned by InterfaceObject.cpp's
// VTBL; base CObject vtbl 0x5e8cb4). The inline InterfaceObject ctor stamps both vptrs
// around the +0x8 name CString ctor and zeroes m_4/m_c; the /GX new-cleanup frame is
// retail. Given a non-null GUID + name it records the GUID, assigns the name, and
// AddTail's the node onto the +0x1c group CObList (caching the position at +0xc). On a
// null GUID/name it deletes the node (the slot-1 scalar-deleting dtor) and returns 0.
// (Was a CNetGroupNode ctor-side view of this same node; folded onto InterfaceObject so
// the own-vptr stamp binds to ??_7InterfaceObject rather than reloc-masking.)
RVA(0x00178360, 0xc8)
i32 CNetMgr::AddGroupNode(void* guid, void* name) {
    InterfaceObject* node = new InterfaceObject();

    if (guid == 0 || name == 0) {
        delete node;
        return 0;
    }

    node->m_4 = (i32)guid;
    node->m_name = static_cast<const char*>(name);
    node->m_c = (i32)m_groups.AddTail((::CObject*)node);
    return (i32)node;
}

// ---------------------------------------------------------------------------
// CNetMgr::ClearGroupList  (__thiscall).
// Tears down the managed CObList at +0x1c: walks its nodes (head at +0x20),
// self-destructs each node's payload sub-object (vtable slot 1, flag 1), then
// RemoveAll's the list and zeroes the count/id pair (+0x7c, +0x70).
RVA(0x00178430, 0x3a)
void CNetMgr::ClearGroupList() {
    CNetListNode* node = reinterpret_cast<CNetListNode*>(m_groups.GetHeadPosition());
    while (node != 0) {
        CNetListNode* cur = node;
        node = node->m_next;
        if (cur->m_data != 0) {
            cur->m_data->SelfDestruct(1);
        }
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
                idx = static_cast<i32>(SendMessageA(hList, LB_ADDSTRING, 0, (LPARAM)(LPCTSTR)name));
            }
            if (idx != -1) {
                SendMessageA(hList, LB_SETITEMDATA, idx, (LPARAM)obj);
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

// ---------------------------------------------------------------------------
// CNetMgr::ReadGroupSel  (__thiscall).
// Reads the current selection of the supplied list box (Win32) and, if its item
// data is a valid in-range index (< the +0x28 count), latches it into +0x70.
// Returns the latched value, or 0 on any failure / out-of-range / no selection.
RVA(0x00178590, 0x78)
i32 CNetMgr::ReadGroupSel(void* hList) {
    if (hList == 0) {
        return 0;
    }
    i32 sel = static_cast<i32>(SendMessageA((HWND)hList, LB_GETCURSEL, 0, 0));
    if (sel == -1) {
        return 0;
    }
    if (sel < 0) {
        return 0;
    }
    if (sel >= static_cast<i32>(m_groups.GetCount())) {
        return 0;
    }
    i32 data = static_cast<i32>(SendMessageA((HWND)hList, LB_GETITEMDATA, sel, 0));
    if (data == -1) {
        return 0;
    }
    if (data == 0) {
        return 0;
    }
    m_groupSel = data;
    return data;
}
// NetEnumPlayerCb (0x1786a0, below): forward decl so EnumPlayersInto's `push offset`
// reloc-masks (the callback body follows in RVA order).
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
    i32* guid = (i32*)((char*)this + 4);
    *(i32*)(desc + 0x00) = 0x50;
    *(i32*)(desc + 0x18) = guid[0];
    *(i32*)(desc + 0x1c) = guid[1];
    *(i32*)(desc + 0x20) = guid[2];
    *(i32*)(desc + 0x24) = guid[3];

    IDirectPlay4Z* com = m_directPlay;
    i32 hr = com->EnumPlayers(desc, a, (void*)&NetEnumPlayerCb, this, b);
    if (hr) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x1c9, hr, 0);
        return hr;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// NetEnumPlayerCb  (EnumSessionsCallback2; __stdcall, 4 args -> ret 0x10).
// The per-session callback EnumPlayersInto hands to the DirectPlay EnumSessions
// slot: skips the timed-out sweep (dwFlags & DPESC_TIMEDOUT) and forwards each
// live session descriptor to AddPlayerNode. Returns TRUE to keep enumerating,
// FALSE on a null context/descriptor or the timed-out pass.
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
i32 CNetMgr::AddPlayerNode(void* playerDesc) {
    if (playerDesc == 0) {
        return 0;
    }

    CNetPlayerListNode* node = new CNetPlayerListNode();

    if (node->Init((CNetSessionDesc*)playerDesc) == 0) {
        delete node;
        return 0;
    }

    node->m_54 = (__POSITION*)m_players.AddTail((::CObject*)node);
    return (i32)node;
}

// ---------------------------------------------------------------------------
// CNetMgr::ClearPlayerList  (__thiscall).
// Tears down the managed CObList at +0x38 (head at +0x3c): self-destructs each
// node's payload, RemoveAll's the list, zeroes the count/id pair (+0x80, +0x74).
RVA(0x00178750, 0x3d)
void CNetMgr::ClearPlayerList() {
    CNetListNode* node = reinterpret_cast<CNetListNode*>(m_players.GetHeadPosition());
    while (node != 0) {
        CNetListNode* cur = node;
        node = node->m_next;
        if (cur->m_data != 0) {
            cur->m_data->SelfDestruct(1);
        }
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

    SendMessageA((HWND)hList, LB_RESETCONTENT, 0, 0);

    CNetListNode* node = reinterpret_cast<CNetListNode*>(m_players.GetHeadPosition());
    m_playerSelId = node;
    CNetPlayerDesc* payload;
    if (node != 0) {
        m_playerSelId = node->m_next;
        payload = (CNetPlayerDesc*)node->m_data;
    } else {
        payload = 0;
    }

    while (payload != 0) {
        i32 r = static_cast<i32>(SendMessageA((HWND)hList, LB_ADDSTRING, 0, (LPARAM)payload->m_profile));
        if (r != -1) {
            SendMessageA((HWND)hList, LB_SETITEMDATA, r, (LPARAM)payload);
        }
        CNetListNode* cur = m_playerSelId;
        if (cur != 0) {
            payload = (CNetPlayerDesc*)cur->m_data;
            m_playerSelId = cur->m_next;
        } else {
            payload = 0;
        }
    }
}

// ---------------------------------------------------------------------------
// CNetMgr::ReadPlayerSel  (__thiscall).
// As ReadGroupSel but for the +0x44 count / +0x74 latch list box.
RVA(0x00178820, 0x78)
i32 CNetMgr::ReadPlayerSel(void* hList) {
    if (hList == 0) {
        return 0;
    }
    i32 sel = static_cast<i32>(SendMessageA((HWND)hList, LB_GETCURSEL, 0, 0));
    if (sel == -1) {
        return 0;
    }
    if (sel < 0) {
        return 0;
    }
    if (sel >= static_cast<i32>(m_players.GetCount())) {
        return 0;
    }
    i32 data = static_cast<i32>(SendMessageA((HWND)hList, LB_GETITEMDATA, sel, 0));
    if (data == -1) {
        return 0;
    }
    if (data == 0) {
        return 0;
    }
    m_playerSel = data;
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
i32 CNetMgr::EnumGroupsInto(void* a, void* b, i32 c, i32 d) {
    char buf[0x50];
    memset(buf, 0, 0x50);
    i32* guid = (i32*)((char*)this + 4);
    *(i32*)(buf + 0x00) = 0x50;
    *(i32*)(buf + 0x04) = 0xa044;
    *(i32*)(buf + 0x18) = guid[0];
    *(i32*)(buf + 0x1c) = guid[1];
    *(i32*)(buf + 0x20) = guid[2];
    *(i32*)(buf + 0x24) = guid[3];
    *(void**)(buf + 0x28) = a;
    *(void**)(buf + 0x30) = b;
    *(i32*)(buf + 0x40) = c;
    if (d != 0 && *(char*)d != 0) {
        *(i32*)(buf + 0x34) = d;
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
    i32 r = AddPlayerNode(blob);
    RezFree(blob);
    return r;
}

// ---------------------------------------------------------------------------
// CNetMgr::EnumPlayersCb  (__thiscall).
// One step of the player enumeration: no-op (0) on a null group descriptor.
// Otherwise enumerates the group's players (EnumGroups slot, group+0x4, flag 1);
// on a nonzero HRESULT reports it (NetMgr.cpp line 0x2dc) and returns 0. On
// success it forwards the three trailing args to CreatePlayer.
RVA(0x001789e0, 0x59)
i32 CNetMgr::EnumPlayersCb(void* a, i32 b, i32 c, i32 d) {
    if (a == 0) {
        return 0;
    }

    IDirectPlay4Z* iface = m_directPlay;
    i32 hr = iface->EnumGroups((char*)a + 4, 1);
    if (hr != 0) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x2dc, hr, 0);
        return 0;
    }
    return CreatePlayer((void*)b, c, d);
}

// ---------------------------------------------------------------------------
// CNetMgr::EnumGroupsAll  (__thiscall).
// Clears the session list then enumerates all groups (EnumGroups slot 0xc) with
// the NetEnumCb callback and `this` as the enum context; on a nonzero HRESULT
// reports it (NetMgr.cpp line 0x30a) and returns it, else 0.
RVA(0x00178a40, 0x3e)
i32 CNetMgr::EnumGroupsAll() {
    ClearSessionList();

    IDirectPlay4Z* iface = m_directPlay;
    i32 hr = iface->EnumGroupsCb(0, (void*)&NetEnumCb, this, 0);
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

    i32* r = (i32*)((char*)rec + 0xc);
    i32 desc[4];
    desc[0] = r[0];
    desc[1] = r[1];
    desc[2] = r[2];
    desc[3] = r[3];

    IDirectPlay4Z* iface = m_directPlay;
    i32 hr = iface->EnumGroupsCb(desc, (void*)&NetEnumCb, this, flags);
    if (hr != 0) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x327, hr, 0);
        return hr;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// NetEnumCb  (EnumGroupsCallback2; __stdcall, 5 args -> ret 0x14).
// The group callback the EnumGroups wrappers hand to the DirectPlay EnumGroups
// slot: forwards the group id + the DPNAME short/long names + flags to
// AddSessionNode. No-op (FALSE) on a null context.
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
// m_sessionId into a local scalar (was ((CNetSessionNode*)a)->m_sessionId + node).
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
        i32 hr = iface->GetData5(node->m_sessionId, &blob, 4, 1);
        if (hr != 0) {
            ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x36c, hr, 0);
        }
    }

    if (node != 0) {
        __POSITION* pos = (__POSITION*)m_sessions.AddTail((::CObject*)node);
        if (pos == 0) {
            delete node;
        } else {
            node->m_listPosition = (i32)pos;
        }
    }
    return (i32)node;
}

// ---------------------------------------------------------------------------
// CNetMgr::ClearSessionList  (__thiscall).
// Tears down the managed CObList at +0x54 (head at +0x58): self-destructs each
// node's payload, RemoveAll's the list, zeroes the count/id pair (+0x84, +0x78).
RVA(0x00178c70, 0x3d)
void CNetMgr::ClearSessionList() {
    CNetListNode* node = reinterpret_cast<CNetListNode*>(m_sessions.GetHeadPosition());
    while (node != 0) {
        CNetListNode* cur = node;
        node = node->m_next;
        if (cur->m_data != 0) {
            cur->m_data->SelfDestruct(1);
        }
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
    desc[2] = (i32)a;
    desc[3] = b;

    IDirectPlay4Z* iface = m_directPlay;
    i32 hr = iface->GetSessionDesc(&desc[0], &out, c, 0, 0);
    if (hr != 0) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x3bb, hr, 0);
        return 0;
    }
    return AddSessionNode(out, reinterpret_cast<const char*>(a), reinterpret_cast<const char*>(b), 0);
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

    SendMessageA((HWND)hList, LB_RESETCONTENT, 0, 0);

    CNetListNode* node = reinterpret_cast<CNetListNode*>(m_sessions.GetHeadPosition());
    m_sessionSelId = node;
    CNetSessionNode* payload;
    if (node != 0) {
        m_sessionSelId = node->m_next;
        payload = (CNetSessionNode*)node->m_data;
    } else {
        payload = 0;
    }

    while (payload != 0) {
        CString name = ((CNetMgr*)payload)->GetName();
        i32 r = static_cast<i32>(SendMessageA((HWND)hList, LB_ADDSTRING, 0, (LPARAM)static_cast<const char*>(name)));
        if (r != -1) {
            SendMessageA((HWND)hList, LB_SETITEMDATA, r, (LPARAM)payload);
        }
        CNetListNode* cur = m_sessionSelId;
        if (cur != 0) {
            payload = (CNetSessionNode*)cur->m_data;
            m_sessionSelId = cur->m_next;
        } else {
            payload = 0;
        }
    }
}

// ---------------------------------------------------------------------------
// CNetMgr::RemovePlayerObj  (__thiscall).
// Tears down one managed player object: no-op if null; otherwise self-destructs
// it (vtable slot 1, flag 1) and - if it has a cached list position (+0x20) -
// unlinks it from the embedded m_54 CObList. Returns 1 when an object was given.
RVA(0x00178e20, 0x33)
i32 CNetMgr::RemovePlayerObj(CNetPlayerObj* obj) {
    if (obj == 0) {
        return 0;
    }

    __POSITION* pos = obj->m_20;
    obj->SelfDestruct(1);
    if (pos != 0) {
        m_sessions.RemoveAt(pos);
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CNetMgr::RemovePlayerById  (__thiscall).
// Looks up the player object DirectPlay stores as `id`'s player-data blob
// (GetPlayerData) and, if present, tears it down via RemovePlayerObj (returning
// its result); returns 0 when no object is registered for the id.
RVA(0x00178e60, 0x23)
i32 CNetMgr::RemovePlayerById(i32 id) {
    CNetPlayerObj* obj = (CNetPlayerObj*)GetPlayerData(id);
    if (obj != 0) {
        return RemovePlayerObj(obj);
    }
    return 0;
}

// ---------------------------------------------------------------------------
// CNetMgr::FindPlayerById  (pure leaf; __thiscall).
// Walks the m_58 player list and returns the first entry whose id (+0x4) equals
// the requested id, or null if the list is empty / no entry matches.
RVA(0x00178e90, 0x20)
CNetPlayerEntry* CNetMgr::FindPlayerById(i32 id) {
    CNetPlayerNode* node = reinterpret_cast<CNetPlayerNode*>(m_sessions.GetHeadPosition());
    while (node != 0) {
        CNetPlayerNode* cur = node;
        node = node->m_next;
        CNetPlayerEntry* entry = cur->m_8;
        if (entry->m_4 == id) {
            return entry;
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------
// CNetMgr::GetPlayerData  (__thiscall).
// Thin IDirectPlay4 wrapper: fetches the per-player data blob for `id` into a
// stack out-ptr (size probe pre-set to 4, flags 1). Returns the blob pointer on
// success, null on any HRESULT failure (the negl/sbbl/notl/and mask form).
RVA(0x00178eb0, 0x3f)
void* CNetMgr::GetPlayerData(i32 id) {
    u32 size;
    void* data;
    data = 0;
    size = 4;
    i32 hr = m_directPlay->GetData2(id, &data, &size, 1);
    return hr ? 0 : data;
}

// ---------------------------------------------------------------------------
// CNetMgr::SetGroupData2  (__thiscall).
// IDirectPlay4 set-data wrapper taking two CNetPlayerEntry handles whose +0x4
// ids are forwarded (0 if null), trailed by three raw dwords; on a nonzero
// HRESULT it routes the error through the static diagnostic reporter
// (NetMgr.cpp:1133).
RVA(0x00178ef0, 0x5c)
i32 CNetMgr::SetGroupData2(CNetPlayerEntry* a, CNetPlayerEntry* b, i32 c, i32 d, i32 e) {
    i32 ida = a ? a->m_4 : 0;
    i32 idb = b ? b->m_4 : 0;
    i32 hr = m_directPlay->SetData5(ida, idb, c, d, e);
    if (hr) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x46d, hr, 0);
    }
    return hr;
}

// ---------------------------------------------------------------------------
// CNetMgr::SendEx  (__thiscall, 0x178f50).
// Bare IDirectPlay4 SendEx wrapper: forwards nine args straight through; on a
// nonzero HRESULT other than the benign DPERR_PENDING (0x8000000a) routes the
// error through the diagnostic reporter.
RVA(0x00178f50, 0x61)
i32 CNetMgr::SendEx(i32 a, i32 b, i32 c, i32 d, i32 e, i32 f, i32 g, i32 h, i32 i) {
    i32 hr = m_directPlay->SendEx(a, b, c, (void*)d, e, f, g, (void*)h, (i32*)i);
    if (hr && hr != static_cast<i32>(0x8000000a)) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x481, hr, 0);
    }
    return hr;
}

// ---------------------------------------------------------------------------
// CNetMgr::SetData  (__thiscall).
// Bare IDirectPlay4 set-data wrapper: forwards five args straight through; on a
// nonzero HRESULT routes the error through the diagnostic reporter
// (NetMgr.cpp:1170).
RVA(0x00178fc0, 0x44)
i32 CNetMgr::SetData(i32 a, i32 b, i32 c, i32 d, i32 e) {
    i32 hr = m_directPlay->SetData5(a, b, c, d, e);
    if (hr) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x492, hr, 0);
    }
    return hr;
}

// ---------------------------------------------------------------------------
// CNetMgr::Receive  (__thiscall, 0x179010).
// IDirectPlay4 Receive wrapper: seeds the from/to player-id out-cells from the
// two CNetPlayerEntry handles (0 if null), then receives into the caller buffer;
// on a nonzero HRESULT routes the error through the diagnostic reporter.
RVA(0x00179010, 0x76)
i32 CNetMgr::Receive(
    CNetPlayerEntry* from,
    CNetPlayerEntry* to,
    i32 flags,
    void* lpData,
    i32* lpSize
) {
    i32 idFrom = from ? from->m_4 : 0;
    i32 idTo = to ? to->m_4 : 0;
    i32 hr = m_directPlay->Receive(&idFrom, &idTo, flags, lpData, lpSize);
    if (hr) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x4b7, hr, 0);
    }
    return hr;
}

// ---------------------------------------------------------------------------
// CNetMgr::SetGroupDataFrom  (__thiscall).
// IDirectPlay4 set-data wrapper whose first arg is a CNetPlayerEntry handle
// (its +0x4 id is forwarded, 0 if null) followed by a literal 0 and three raw
// dwords; on a nonzero HRESULT routes the error through the diagnostic reporter
// (NetMgr.cpp:1242).
RVA(0x00179090, 0x4c)
i32 CNetMgr::SetGroupDataFrom(CNetPlayerEntry* a, i32 c, i32 d, i32 e) {
    i32 ida = a ? a->m_4 : 0;
    i32 hr = m_directPlay->SetData5(ida, 0, c, d, e);
    if (hr) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x4da, hr, 0);
    }
    return hr;
}

// ---------------------------------------------------------------------------
// CNetMgr::RemovePlayerNode  (__thiscall).
// Drops one CNetPlayerListNode from the +0x38 player list: clears the selection
// latch (+0x74) if it points at this node, closes the DirectPlay side (m_18 slot
// 4), then self-destructs the node and unlinks its cached +0x38 position.
RVA(0x001790e0, 0x4f)
i32 CNetMgr::RemovePlayerNode(CNetPlayerListNode* node) {
    if (node == 0) {
        return 0;
    }
    if (m_playerSel == (i32)node) {
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

// ---------------------------------------------------------------------------
// CNetMgr::EnumSessions  (__thiscall).
// IDirectPlay4 enumeration wrapper: zero-inits the 0x28-byte descriptor `desc`,
// stamps its dwSize, then calls the enum slot with the caller context. On a
// nonzero HRESULT routes the error through the diagnostic reporter
// (NetMgr.cpp:1322) and returns 0; otherwise returns 1.
RVA(0x00179130, 0x5d)
i32 CNetMgr::EnumSessions(void* desc, void* ctx) {
    if (desc == 0) {
        return 0;
    }

    memset(desc, 0, 0x28);
    *(i32*)desc = 0x28;
    i32 hr = m_directPlay->Enum2(desc, ctx);
    if (hr) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x52a, hr, 0);
        return 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CNetMgr::GetGroupInfo  (__thiscall, 0x179190).
// IDirectPlay4 GetGroupData wrapper: requires the group handle (its +0x4 id),
// zero-inits the 0x28-byte descriptor and stamps its dwSize, then queries the
// data slot. On a nonzero HRESULT routes the error and returns 0; else 1.
RVA(0x00179190, 0x84)
i32 CNetMgr::GetGroupInfo(CNetPlayerEntry* a, void* desc, i32 flags) {
    if (!a) {
        return 0;
    }
    if (!a->m_4) {
        return 0;
    }
    if (!desc) {
        return 0;
    }
    memset(desc, 0, 0x28);
    *(i32*)desc = 0x28;
    IDirectPlay4Z* dp = m_directPlay;
    i32 id = a->m_4;
    i32 hr = dp->GetGroupData(id, desc, flags);
    if (hr) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x553, hr, 0);
        return 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CNetMgr::EnumSessions2  (__thiscall).
// Enumerates sessions into a scratch 0x28-byte descriptor (EnumSessions) with the
// caller context and, on success, returns the descriptor's +0x18 field (0 on any
// enum failure - branchless neg/sbb/and select).
RVA(0x00179240, 0x22)
i32 CNetMgr::EnumSessions2(void* ctx) {
    char desc[0x28];
    i32 ok = EnumSessions(desc, ctx);
    return ok ? *(i32*)(desc + 0x18) : 0;
}

// ---------------------------------------------------------------------------
// CNetMgr::Find (0x179270, __thiscall, ret 4) - the group-list lookup.
// The +0x1c group CObList holds InterfaceObject payload nodes (CNode shape:
// next@+0, data@+8). For each payload, the service-provider class `kind` selects
// one of the GUID predicates (Font.cpp InterfaceObject::IsInterfaceX, reloc-
// masked): kind 1 -> IsInterface2, kind 2 -> IsInterface1, kind 5 -> IsInterface5;
// return the first payload that matches. The running POSITION is cached in the
// typed m_groupSelId cursor (+0x7c) as the GetNext walk position.
// ---------------------------------------------------------------------------
// CGroupNode (the +0x1c group-list node view) is declared in <Net/NetMgr.h>.

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
// the exact cl name via @data-symbol (authority-checked against netmgr.obj).
// @data-symbol: ?g_guid1@@3PBEB 0x00224d58
// @data-symbol: ?g_guid2@@3PBEB 0x00224d68
// @data-symbol: ?g_guid3@@3PBEB 0x00224d78
// @data-symbol: ?g_guid4@@3PBEB 0x00224d88
// @data-symbol: ?g_guid5@@3PBEB 0x00224d98
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
// clang-format on

// =========================================================================
// IsInterface1
RVA(0x001794b0, 0x21)
i32 InterfaceObject::IsInterface1() {
    if (!m_4) {
        return 0;
    }
    return memcmp((const void*)m_4, g_guid1, 16) == 0 ? 1 : 0;
}

// =========================================================================
// IsInterface2
RVA(0x001794e0, 0x21)
i32 InterfaceObject::IsInterface2() {
    if (!m_4) {
        return 0;
    }
    return memcmp((const void*)m_4, g_guid2, 16) == 0 ? 1 : 0;
}

// =========================================================================
// IsInterface3
RVA(0x00179510, 0x21)
i32 InterfaceObject::IsInterface3() {
    if (!m_4) {
        return 0;
    }
    return memcmp((const void*)m_4, g_guid3, 16) == 0 ? 1 : 0;
}

// =========================================================================
// IsInterface4
RVA(0x00179540, 0x21)
i32 InterfaceObject::IsInterface4() {
    if (!m_4) {
        return 0;
    }
    return memcmp((const void*)m_4, g_guid4, 16) == 0 ? 1 : 0;
}

// =========================================================================
// IsInterface5
RVA(0x00179570, 0x21)
i32 InterfaceObject::IsInterface5() {
    if (!m_4) {
        return 0;
    }
    return memcmp((const void*)m_4, g_guid5, 16) == 0 ? 1 : 0;
}

// =========================================================================
// CNetPlayerListNode::FreeStrings (0x179680)
// Free the two strdup'd descriptor names (m_desc.m_lpszName @+0x34 /
// m_desc.m_lpszPassword @+0x38, the ones Init duplicated in place) and clear the
// dwSize marker (+0x04). IDENTITY (ex "CWapNodeB::FreeStrings"): retail's SINGLE
// caller is ??1CNetPlayerListNode @0x1793db, the fields it touches live INSIDE
// this node's 0x50-byte DPSESSIONDESC2 copy, and the dtor chain has exactly two
// vptr stamps (own 0x5f0760 -> CObject 0x5e8cb4) - so the "CWapNodeB" that
// carried it was a duplicate view of THIS class, not a base; DISSOLVED.
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

// ---------------------------------------------------------------------------
// CNetSessionNode::InitSession (0x1796c0, __thiscall) - the 4-arg body the
// session-node ctor (AddSessionNode) runs on the fresh 0x24-byte node: store the
// dword id (+0x4) and the second dword (+0x10), assign the two name CStrings
// (+0x8/+0xc), zero the +0x14/+0x18/+0x1c scratch, return TRUE.
// ---------------------------------------------------------------------------
RVA(0x001796c0, 0x3f)
i32 CNetSessionNode::InitSession(i32 id, const char* nameA, const char* nameB, i32 d) {
    m_sessionId = id;
    m_8 = nameA;
    m_c = nameB;
    m_10 = d;
    m_ownedBufferA = 0;
    m_1c = 0;
    m_ownedBufferB = 0;
    return 1;
}
