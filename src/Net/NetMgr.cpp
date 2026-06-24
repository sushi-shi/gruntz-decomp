// NetMgr.cpp - CNetMgr (DirectPlay networking manager) handlers + config writer.
// Matched methods:
//   CNetMgr::OnMultiOptions
//   CNetMgr::OnMultiPause
//   CNetMgr::OnOutOfSync
//   CNetMgr::ApplyCmdDelayDefaults
//   CNetMgr::GetMaxAckLatency   - max ack-latency over the four network slots
//   CNetMgr::ReportAckLatency   - ships that latency to the engine as stat 0x421
//   CNetMgr::FindPlayerById     - id lookup over the m_58 player list
//
// (CNetMgr::ReportError, the DirectPlay HRESULT->string formatter, lives in its
// own TU - src/Net/NetMgrReportError.cpp / the netmgrerror unit.)
//
// The three message handlers fire a multiplayer command through the engine
// dispatcher (MultiDispatch, external, via incremental-link thunk), guarded by
// a reentrancy flag, and (Pause/OutOfSync) forward a WM_COMMAND to the engine
// window via PostMessageA when the dispatch result matches. ApplyCmdDelayDefaults
// persists the command-timing config (m_5a4/m_5a8) to the game's RegistryHelper.
#include <Net/NetMgr.h>
#include <rva.h>
#include <string.h> // memset (inlined rep stosl for the version packet)

CGameMgr* g_pGameMgr;

// File-scope reentrancy guards.
static int g_optionzGuard;
static int g_pauseGuard;
static int g_sharedFlag;
static int g_dropGuard;  // OnDropPlayer reentrancy guard (DAT_00648d10)
static int g_syncToggle; // FrameSyncWait alternating low-bit flag (DAT_00648d0c)

// ---------------------------------------------------------------------------
// CNetMgr::OnMultiOptions
// Reentrancy-guarded fire of the MULTI_OPTIONZ command. Clears m_584, dispatches
// (return value ignored), then clears the shared flag.
RVA(0x000badd0, 0x43)
void CNetMgr::OnMultiOptions() {
    if (g_optionzGuard) {
        return;
    }

    m_584 = 0;
    g_optionzGuard = 1;
    MultiDispatch("MULTI_OPTIONZ", MultiOptionzCallback, 0);
    g_optionzGuard = 0;
    g_sharedFlag = 0;
}

// ---------------------------------------------------------------------------
// CNetMgr::OnMultiPause
// Reentrancy-guarded fire of MULTI_PAUSE. When the dispatch returns 0x4cc,
// forwards WM_COMMAND(0x80d7, m_1c) to the engine window.
RVA(0x000bad40, 0x6c)
void CNetMgr::OnMultiPause() {
    if (g_pauseGuard) {
        return;
    }

    m_584 = 0;
    g_pauseGuard = 1;
    int r = MultiDispatch("MULTI_PAUSE", MultiPauseCallback, 0);
    g_pauseGuard = 0;
    g_sharedFlag = 0;

    if (r == 0x4cc) {
        void* hwnd = ((CNetHwndHolder*)((CNetHwndHolder*)m_4)->m_4)->m_4;
        PostMessageA((HWND)hwnd, 0x111, 0x80d7, m_1c);
    }
}

// ---------------------------------------------------------------------------
// CNetMgr::OnOutOfSync
// Per-instance reentrancy-guarded fire of MULTI_OUTOFSYNC. Switches on the
// dispatch result: 0x4cc -> the same WM_COMMAND(0x80d7, m_1c) as Pause;
// 0x4cd -> nothing; otherwise -> WM_COMMAND(0x8023, 0).
RVA(0x000bae40, 0x84)
void CNetMgr::OnOutOfSync() {
    if (m_574) {
        return;
    }

    m_574 = 1;
    m_584 = 0;
    int r = MultiDispatch("MULTI_OUTOFSYNC", MultiOutOfSyncCallback, 0);
    g_sharedFlag = 0;

    switch (r) {
        case 0x4cc: {
            void* hwnd = ((CNetHwndHolder*)((CNetHwndHolder*)m_4)->m_4)->m_4;
            PostMessageA((HWND)hwnd, 0x111, 0x80d7, m_1c);
            break;
        }
        case 0x4cd:
            break;
        default: {
            void* hwnd = ((CNetHwndHolder*)((CNetHwndHolder*)m_4)->m_4)->m_4;
            PostMessageA((HWND)hwnd, 0x111, 0x8023, 0);
            break;
        }
    }
}

// ---------------------------------------------------------------------------
// CNetMgr::ApplyCmdDelayDefaults  (C++ EH frame).
// Persists the command-timing config to the game RegistryHelper (the singleton's
// +0x38 member). Builds three value-name strings "m_598 + _Suffix" via operator+
// (each a stack CString temp), writes m_5a4 under "_CmdDelay" and m_5a8 under
// "_Resend"; the "_DynCmdDelay" temp is built but its write is elided here. The
// three temporaries' dtors run under the C++ EH frame (=> /GX).
RVA(0x000b85a0, 0xd2)
void CNetMgr::ApplyCmdDelayDefaults() {
    Utils::RegistryHelper* reg = g_pGameMgr->m_38;

    CString cmdDelayName = m_598 + "_CmdDelay";
    CString resendName = m_598 + "_Resend";
    CString dynCmdName = m_598 + "_DynCmdDelay";

    reg->SetValueDword((char*)(const char*)cmdDelayName, m_5a4);
    reg->SetValueDword((char*)(const char*)resendName, m_5a8);
}

// -------------------------------------------------------------------------
// Engine-label backlog stubs.
// -------------------------------------------------------------------------
// @confidence: high
// @source: rtti-vptr
// @stub
RVA(0x000b5460, 0x914)
void CNetMgr::Stub_0b5460() {}

// @confidence: high
// @source: rtti-vptr
// @stub
RVA(0x000b6000, 0x6d)
void CNetMgr::Stub_0b6000() {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x000b78b0, 0x17f)
void CNetMgr::Stub_0b78b0() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x000b7b10, 0x27c)
void CNetMgr::Stub_0b7b10() {}

// @confidence: high
// @source: decomp-xref
// @stub
RVA(0x000b82e0, 0x230)
void CNetMgr::Stub_0b82e0() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x000b8b10, 0x175)
void CNetMgr::Stub_0b8b10() {}

// @confidence: low
// @source: decomp-xref
// @stub
RVA(0x000b8cf0, 0x23b)
void CNetMgr::Stub_0b8cf0() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x000b9750, 0x74e)
void CNetMgr::Stub_0b9750() {}

// ---------------------------------------------------------------------------
// CNetMgr::FrameSyncWait  (__thiscall).
// Paces the network frame: samples timeGetTime, records the delta since the
// last call (m_5e0) and the new stamp (m_5e4). If the frame came in under 0x1f
// ms it busy-waits the remainder (ActiveWait) and re-stamps; otherwise, if the
// frame ran long (> 0x28 ms) and the sync gate m_578 is set, it flips the
// global low-bit sync toggle and returns it.
RVA(0x000bc070, 0x73)
unsigned CNetMgr::FrameSyncWait() {
    unsigned now = timeGetTime();
    unsigned delta = now - m_5e4;
    unsigned ret = 0;
    m_5e0 = delta;
    m_5e4 = now;

    if (delta <= 0x1e) {
        Utils::WinAPI::ActiveWait(0x1f - delta);
        m_5e4 = (now - m_5e0) + 0x1f;
        return 0;
    }
    if (delta > 0x28 && m_578) {
        ret = g_syncToggle ^ 1;
        g_syncToggle = ret;
    }
    return ret;
}

// ---------------------------------------------------------------------------
// CNetMgr::OnDropPlayer  (__thiscall).
// Reentrancy-guarded fire of the MULTI_DROPPLAYER command. Clears m_584,
// dispatches, then switches on the result: 0x4cd just resets the command
// buffers; 0x4ce resets and posts WM_COMMAND(0x8023) to the engine window;
// 0x4ea reports the leaving player (stat 0x411 if still present), broadcasts
// the drop (stat 0x410), acks it, and resets the buffers.
RVA(0x000bc110, 0xf6)
void CNetMgr::OnDropPlayer() {
    if (g_dropGuard) {
        return;
    }

    m_584 = 0;
    g_dropGuard = 1;
    int r = MultiDispatch("MULTI_DROPPLAYER", MultiDropPlayerCallback, 0);
    g_dropGuard = 0;
    g_sharedFlag = 0;

    switch (r) {
        case 0x4cd:
            m_520->ResetCmdBuffers();
            break;
        case 0x4ce: {
            m_520->ResetCmdBuffers();
            void* hwnd = ((CNetHwndHolder*)((CNetHwndHolder*)m_4)->m_4)->m_4;
            PostMessageA((HWND)hwnd, 0x111, 0x8023, 0);
            break;
        }
        case 0x4ea:
            if (g_dropPlayerId != -999) {
                if (m_524->FindPlayerById(g_dropPlayerId)) {
                    SendStat3(g_dropPlayerId, 0x411, 1);
                }
            }
            SendNetStat(0x410, g_dropPlayerId, 1);
            AckDropPlayer(g_dropPlayerId);
            m_520->ResetCmdBuffers();
            break;
    }
}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x000bc460, 0x24e)
void CNetMgr::Stub_0bc460() {}

// ---------------------------------------------------------------------------
// CNetMgr::WaitForConnect  (__thiscall).
// Blocks (pumping the session) until the local player is admitted to the host's
// game or the attempt fails. Bails immediately if there is no DirectPlay
// interface (m_524) or local player descriptor (m_5bc). Announces "connecting"
// (stat 0x415), clears the admit flag m_58c, then loops: each pass times out at
// 60s or on Esc (-> status 0x8022, fail), pumps the receive queue, and reports +
// fails on any of the session-state flags (terminated / removed / closed / full
// / version-mismatch). Returns 1 once m_58c latches (admitted), 0 on any failure.
RVA(0x000bca50, 0x155)
int CNetMgr::WaitForConnect() {
    if (m_524 == 0) {
        return 0;
    }
    if (m_5bc == 0) {
        return 0;
    }

    SendStatFlag(0x415, 1);
    m_58c = 0;
    unsigned start = timeGetTime();
    if (m_58c != 0) {
        return 1;
    }

    do {
        unsigned now = timeGetTime();
        if (now > start + 60000 || ((int)GetAsyncKeyState(0x1b) & 0x80000000)) {
            ReportStatusId(0x8022, 0);
            return 0;
        }
        PollSession();
        if (m_52c) {
            ReportNetError("The game session has been terminated.", 0);
            return 0;
        }
        if (m_538) {
            ReportNetError("You have been removed from the game by the host.", 0);
            return 0;
        }
        if (m_5ac) {
            ReportNetError("This game is closed.", 0);
            return 0;
        }
        if (m_56c) {
            ReportNetError("This game is already full.", 0);
            return 0;
        }
        if (m_570) {
            ReportNetError(
                "This version is not the same as the host computer's version of the game.",
                0
            );
            return 0;
        }
    } while (m_58c == 0);
    return 1;
}

// ---------------------------------------------------------------------------
// CNetMgr::ResetPlayerCommands  (__thiscall).
// Flushes the resend buffers for one player's command slot. No-op unless
// connected (m_580). Looks the player's slot up in the session (m_520); if found
// and not already reset (slot->m_4 == 0), latches it, then for each command
// sequence number in the slot's window ([(seq0+1)..(seq0+1)+3] scaled by the
// per-command delay m_5a4) re-dispatches the command through m_4's queue and
// drops it from the slot. Finally clears the slot's two command ranges.
RVA(0x000bcf20, 0xaf)
int CNetMgr::ResetPlayerCommands(int id) {
    if (m_580 == 0) {
        return 0;
    }

    CNetCmdSlot* slot = m_520->FindCmdSlot(id);
    if (slot == 0) {
        return 0;
    }
    if (slot->m_4 != 0) {
        return 0;
    }

    slot->Touch();
    int seq = (slot->m_14 + 1) * (int)m_5a4;
    int end = seq + (int)m_5a4 * 3;
    for (; seq < end; seq++) {
        ((CNetSubObject*)m_4)->m_6c->Dispatch(*slot->m_c, seq);
        slot->RemoveCmd(seq / (int)m_5a4);
    }
    slot->ResetTriple(slot->m_4c);
    slot->ResetTriple(slot->m_58);
    return 1;
}

// ---------------------------------------------------------------------------
// CNetMgr::ReportAckLatency  (__thiscall).
// Thin wrapper: samples the current worst ack latency and ships it to the
// engine command dispatcher as stat 0x421.
RVA(0x000bd000, 0x19)
void CNetMgr::ReportAckLatency() {
    unsigned latency = GetMaxAckLatency();
    SendNetStat(0x421, latency, 0);
}

// ---------------------------------------------------------------------------
// CNetMgr::GetMaxAckLatency  (pure leaf; __thiscall).
// Returns the largest latency value across the four network slots. When the
// branch selector m_528 is set the values come from the inline m_5f0[4] channel
// array (every entry counted); otherwise they come from the four per-player
// slots hanging off m_4 (stride 0x238), each counted only when BOTH of its
// "slot active" gate flags (m_164, m_170) are nonzero.
RVA(0x000bd030, 0x5d)
unsigned CNetMgr::GetMaxAckLatency() {
    unsigned max = 0;

    if (m_528 != 0) {
        for (int i = 0; i < 4; i++) {
            if (m_5f0[i] > max) {
                max = m_5f0[i];
            }
        }
    } else {
        CNetPlayerSlot* slot = (CNetPlayerSlot*)m_4;
        for (int i = 0; i < 4; i++) {
            if (slot->m_164 && slot->m_170) {
                if (slot->m_37c > max) {
                    max = slot->m_37c;
                }
            }
            slot = (CNetPlayerSlot*)((char*)slot + 0x238);
        }
    }
    return max;
}

// ---------------------------------------------------------------------------
// CNetMgr::HandleVersionCheck  (__thiscall).
// Inspects a host packet's version pair (+0x18/+0x1c) against the two engine
// version locals. On any mismatch it latches m_570, and - if a connection was
// already up (m_580) - reports the canned "version mismatch" diagnostic and
// posts WM_COMMAND(0x8023) to the engine window; then fires stat 0x418 and
// sleeps 250ms before returning.
RVA(0x000bd0b0, 0x9a)
void CNetMgr::HandleVersionCheck(CNetVersionMsg* msg) {
    if (msg == 0) {
        return;
    }

    int mismatch = 0;
    if (g_localVersion != msg->m_1c) {
        mismatch = 1;
    }
    if (g_remoteVersion != msg->m_18) {
        mismatch = 1;
    }

    if (mismatch) {
        int wasConnected = m_580;
        m_570 = 1;
        if (wasConnected) {
            ReportVersionMsg(
                "This version is not the same as the host computer's version of the game.",
                0
            );
            void* hwnd = ((CNetHwndHolder*)((CNetHwndHolder*)m_4)->m_4)->m_4;
            PostMessageA((HWND)hwnd, 0x111, 0x8023, 0);
        }
    }
    if (mismatch) {
        SendStatFlag(0x418, 1);
        Sleep(0xfa);
    }
}

// ---------------------------------------------------------------------------
// CNetMgr::AnnounceVersion  (__thiscall).
// Builds a 0x20-byte version-announce packet on the stack (flag byte, the
// CButeMgr config word, g_cfgWord, stat id 0x417, and the local/remote version
// pair) and ships it through the engine stat dispatcher as stat 0x417.
RVA(0x000bd180, 0x66)
void CNetMgr::AnnounceVersion(int param) {
    CNetVersionPacket packet;
    memset(&packet, 0, sizeof(packet));

    packet.m_0 |= 0x80;
    packet.m_18 = g_remoteVersion;
    packet.m_c = g_cfgWord;
    packet.m_8 = g_buteMgrField4;
    packet.m_1c = g_localVersion;
    packet.m_10 = 0x417;

    SendStatPacket(param, &packet, 0x20, 1);
}

// @confidence: high
// @source: import:DPLAYX.dll!#1
// @stub
RVA(0x001780b0, 0xbb)
void CNetMgr::Stub_1780b0() {}

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

    INetReleasable*& iface14 = *(INetReleasable**)((char*)this + 0x14);
    if (iface14 != 0) {
        iface14->vtbl->Release(iface14);
        iface14 = 0;
    }
    INetReleasable*& iface18 = *(INetReleasable**)((char*)this + 0x18);
    if (iface18 != 0) {
        iface18->vtbl->Slot10(iface18);
        INetReleasable* again = iface18;
        again->vtbl->Release(again);
        iface18 = 0;
    }
}

// @confidence: high
// @source: import:DPLAYX.dll!#2
// @stub
RVA(0x00178280, 0x43)
void CNetMgr::Stub_178280() {}

// @confidence: high
// @source: import:DPLAYX.dll!#1
// @stub
RVA(0x001782d0, 0x86)
void CNetMgr::Stub_1782d0() {}

// AddGroupNode: operator-new'd 0x10-byte node (2-phase vtbl @0x5f0748 / base
// dtor-vtbl @0x5e8cb4 + CString member @+0x8 + /GX EH frame) AddTail'd onto the
// +0x1c list. Reconstruct in the final sweep: needs the custom node class modeled.
// @confidence: high
// @source: decomp-xref
// @stub
RVA(0x00178360, 0xc8)
void CNetMgr::Stub_178360() {}

// ---------------------------------------------------------------------------
// CNetMgr::ClearGroupList  (__thiscall).
// Tears down the managed CObList at +0x1c: walks its nodes (head at +0x20),
// self-destructs each node's payload sub-object (vtable slot 1, flag 1), then
// RemoveAll's the list and zeroes the count/id pair (+0x7c, +0x70).
RVA(0x00178430, 0x3a)
void CNetMgr::ClearGroupList() {
    CNetListNode* node = *(CNetListNode**)((char*)this + 0x20);
    while (node != 0) {
        CNetListNode* cur = node;
        node = node->m_next;
        if (cur->m_data != 0) {
            cur->m_data->SelfDestruct(1);
        }
    }
    ((CObList*)((char*)this + 0x1c))->RemoveAll();
    *(int*)((char*)this + 0x7c) = 0;
    *(int*)((char*)this + 0x70) = 0;
}

// ---------------------------------------------------------------------------
// CNetMgr::ReadGroupSel  (__thiscall).
// Reads the current selection of the supplied list box (Win32) and, if its item
// data is a valid in-range index (< the +0x28 count), latches it into +0x70.
// Returns the latched value, or 0 on any failure / out-of-range / no selection.
RVA(0x00178590, 0x78)
int CNetMgr::ReadGroupSel(void* hList) {
    if (hList == 0) {
        return 0;
    }
    int sel = (int)SendMessageA((HWND)hList, 0x188, 0, 0);
    if (sel == -1) {
        return 0;
    }
    if (sel < 0) {
        return 0;
    }
    if (sel >= *(int*)((char*)this + 0x28)) {
        return 0;
    }
    int data = (int)SendMessageA((HWND)hList, 0x199, sel, 0);
    if (data == -1) {
        return 0;
    }
    if (data == 0) {
        return 0;
    }
    *(int*)((char*)this + 0x70) = data;
    return data;
}

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
long CNetMgr::EnumPlayersInto(void* a, void* b) {
    ClearPlayerList();

    char desc[0x50];
    memset(desc, 0, 0x50);
    int* guid = (int*)((char*)this + 4);
    *(int*)(desc + 0x00) = 0x50;
    *(int*)(desc + 0x18) = guid[0];
    *(int*)(desc + 0x1c) = guid[1];
    *(int*)(desc + 0x20) = guid[2];
    *(int*)(desc + 0x24) = guid[3];

    IDirectPlay4Z* com = *(IDirectPlay4Z**)((char*)this + 0x18);
    long hr = com->vtbl->EnumPlayers(com, desc, a, (void*)&NetEnumPlayerCb, this, b);
    if (hr) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x1c9, hr, 0);
        return hr;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// CNetMgr::ClearPlayerList  (__thiscall).
// Tears down the managed CObList at +0x38 (head at +0x3c): self-destructs each
// node's payload, RemoveAll's the list, zeroes the count/id pair (+0x80, +0x74).
RVA(0x00178750, 0x3d)
void CNetMgr::ClearPlayerList() {
    CNetListNode* node = *(CNetListNode**)((char*)this + 0x3c);
    while (node != 0) {
        CNetListNode* cur = node;
        node = node->m_next;
        if (cur->m_data != 0) {
            cur->m_data->SelfDestruct(1);
        }
    }
    ((CObList*)((char*)this + 0x38))->RemoveAll();
    *(int*)((char*)this + 0x80) = 0;
    *(int*)((char*)this + 0x74) = 0;
}

// ---------------------------------------------------------------------------
// CNetMgr::ReadPlayerSel  (__thiscall).
// As ReadGroupSel but for the +0x44 count / +0x74 latch list box.
RVA(0x00178820, 0x78)
int CNetMgr::ReadPlayerSel(void* hList) {
    if (hList == 0) {
        return 0;
    }
    int sel = (int)SendMessageA((HWND)hList, 0x188, 0, 0);
    if (sel == -1) {
        return 0;
    }
    if (sel < 0) {
        return 0;
    }
    if (sel >= *(int*)((char*)this + 0x44)) {
        return 0;
    }
    int data = (int)SendMessageA((HWND)hList, 0x199, sel, 0);
    if (data == -1) {
        return 0;
    }
    if (data == 0) {
        return 0;
    }
    *(int*)((char*)this + 0x74) = data;
    return data;
}

// EnumGroupsInto: IDirectPlay4 EnumGroups (+0x60) wrapper + GetPlayerData2 (+0x58)
// probes with operator-new/RezFree blob juggling; calls the sibling group-node
// factory at 0x1786d0 (itself unmatched). Reconstruct in the final sweep once
// 0x1786d0 and the node class are modeled.
// @confidence: high
// @source: decomp-xref
// @stub
RVA(0x001788a0, 0x13c)
void CNetMgr::Stub_1788a0() {}

// ---------------------------------------------------------------------------
// CNetMgr::ClearSessionList  (__thiscall).
// Tears down the managed CObList at +0x54 (head at +0x58): self-destructs each
// node's payload, RemoveAll's the list, zeroes the count/id pair (+0x84, +0x78).
RVA(0x00178c70, 0x3d)
void CNetMgr::ClearSessionList() {
    CNetListNode* node = *(CNetListNode**)((char*)this + 0x58);
    while (node != 0) {
        CNetListNode* cur = node;
        node = node->m_next;
        if (cur->m_data != 0) {
            cur->m_data->SelfDestruct(1);
        }
    }
    ((CObList*)((char*)this + 0x54))->RemoveAll();
    *(int*)((char*)this + 0x84) = 0;
    *(int*)((char*)this + 0x78) = 0;
}

// ---------------------------------------------------------------------------
// CNetMgr::RemovePlayerObj  (__thiscall).
// Tears down one managed player object: no-op if null; otherwise self-destructs
// it (vtable slot 1, flag 1) and - if it has a cached list position (+0x20) -
// unlinks it from the embedded m_54 CObList. Returns 1 when an object was given.
RVA(0x00178e20, 0x33)
int CNetMgr::RemovePlayerObj(CNetPlayerObj* obj) {
    if (obj == 0) {
        return 0;
    }

    __POSITION* pos = obj->m_20;
    obj->SelfDestruct(1);
    if (pos != 0) {
        ((CObList*)((char*)this + 0x54))->RemoveAt(pos);
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CNetMgr::FindPlayerById  (pure leaf; __thiscall).
// Walks the m_58 player list and returns the first entry whose id (+0x4) equals
// the requested id, or null if the list is empty / no entry matches.
RVA(0x00178e90, 0x20)
CNetPlayerEntry* CNetMgr::FindPlayerById(int id) {
    CNetPlayerNode* node = m_58;
    while (node != 0) {
        CNetPlayerEntry* entry = node->m_8;
        node = node->m_next;
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
void* CNetMgr::GetPlayerData(int id) {
    unsigned long size;
    void* data;
    data = 0;
    size = 4;
    long hr = m_18->vtbl->GetData2(m_18, id, &data, &size, 1);
    return hr ? 0 : data;
}

// ---------------------------------------------------------------------------
// CNetMgr::SetGroupData2  (__thiscall).
// IDirectPlay4 set-data wrapper taking two CNetPlayerEntry handles whose +0x4
// ids are forwarded (0 if null), trailed by three raw dwords; on a nonzero
// HRESULT it routes the error through the static diagnostic reporter
// (NetMgr.cpp:1133).
RVA(0x00178ef0, 0x5c)
long CNetMgr::SetGroupData2(CNetPlayerEntry* a, CNetPlayerEntry* b, int c, int d, int e) {
    int ida = a ? a->m_4 : 0;
    int idb = b ? b->m_4 : 0;
    long hr = m_18->vtbl->SetData5(m_18, ida, idb, c, d, e);
    if (hr) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x46d, hr, 0);
    }
    return hr;
}

// ---------------------------------------------------------------------------
// CNetMgr::SetData  (__thiscall).
// Bare IDirectPlay4 set-data wrapper: forwards five args straight through; on a
// nonzero HRESULT routes the error through the diagnostic reporter
// (NetMgr.cpp:1170).
RVA(0x00178fc0, 0x44)
long CNetMgr::SetData(int a, int b, int c, int d, int e) {
    long hr = m_18->vtbl->SetData5(m_18, a, b, c, d, e);
    if (hr) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x492, hr, 0);
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
long CNetMgr::SetGroupDataFrom(CNetPlayerEntry* a, int c, int d, int e) {
    int ida = a ? a->m_4 : 0;
    long hr = m_18->vtbl->SetData5(m_18, ida, 0, c, d, e);
    if (hr) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x4da, hr, 0);
    }
    return hr;
}

// ---------------------------------------------------------------------------
// CNetMgr::EnumSessions  (__thiscall).
// IDirectPlay4 enumeration wrapper: zero-inits the 0x28-byte descriptor `desc`,
// stamps its dwSize, then calls the enum slot with the caller context. On a
// nonzero HRESULT routes the error through the diagnostic reporter
// (NetMgr.cpp:1322) and returns 0; otherwise returns 1.
RVA(0x00179130, 0x5d)
int CNetMgr::EnumSessions(void* desc, void* ctx) {
    if (desc == 0) {
        return 0;
    }

    memset(desc, 0, 0x28);
    *(int*)desc = 0x28;
    long hr = m_18->vtbl->Enum2(m_18, desc, ctx);
    if (hr) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x52a, hr, 0);
        return 0;
    }
    return 1;
}
