// NetMgr.cpp - CNetMgr (DirectPlay networking manager) handlers + config writer.
// Matched methods:
//   CNetMgr::OnMultiOptions
//   CNetMgr::OnMultiPause
//   CNetMgr::OnOutOfSync
//   CNetMgr::ApplyCmdDelayDefaults
//   CNetMgr::GetMaxAckLatency   - max ack-latency over the four network slots
//   CNetMgr::ReportAckLatency   - ships that latency to the engine as stat 0x421
//   CNetMgr::FindPlayerById     - id lookup over the m_58 player list
//   CNetMgr::SendStatTo / SendStat3 / SendStatPairRaw / SendStatValue - the rest
//       of the stat-send family (build/forward a 0x10 packet through SetData /
//       SetGroupData2)
//   CNetMgr::GetName            - returns the +0x8 CString member by value
//   CNetMgr::HandleControlMsg   - the network control-message switch dispatcher
//                                 (code byte-exact; jump-table scoring artifact)
//   CNetMgr::OnPlayerLeft       - report + tear down a leaving player (/GX EH)
//   CNetMgr::PollSession        - pump the DirectPlay receive queue (@early-stop)
//
// (CNetMgr::ReportError, the DirectPlay HRESULT->string formatter, lives in its
// own TU - src/Net/NetMgrReportError.cpp / the netmgrerror unit.)
//
// The three message handlers fire a multiplayer command through the engine
// dispatcher (MultiDispatch, external, via incremental-link thunk), guarded by
// a reentrancy flag, and (Pause/OutOfSync) forward a WM_COMMAND to the engine
// window via PostMessageA when the dispatch result matches. ApplyCmdDelayDefaults
// persists the command-timing config (m_cmdDelay/m_resend) to the game's RegistryHelper.
#include <Net/NetMgr.h>
#include <rva.h>
#include <string.h> // memset (inlined rep stosl for the version packet)

#include <Gruntz/GruntzPlayer.h> // OnPlayerLeft derefs the leaving player's slot

CGameMgr* g_pGameMgr;

// File-scope reentrancy guards.
static i32 g_optionzGuard;
static i32 g_pauseGuard;
static i32 g_sharedFlag;
static i32 g_dropGuard;  // OnDropPlayer reentrancy guard (DAT_00648d10)
static i32 g_syncToggle; // FrameSyncWait alternating low-bit flag (DAT_00648d0c)

// MultiDispatch outcome codes the message handlers switch on (engine command
// dispatcher result space; names reconstructed from the branches, values
// load-bearing). Pause/OutOfSync forward the resync WM_COMMAND on DISPATCH_RESYNC;
// DropPlayer resets buffers on DISPATCH_RESET, resets+aborts on DISPATCH_ABORT,
// and reports/broadcasts the leaving player on DISPATCH_PLAYERLEFT.
enum {
    DISPATCH_RESYNC = 0x4cc,     // post the resync WM_COMMAND
    DISPATCH_RESET = 0x4cd,      // reset command buffers only
    DISPATCH_ABORT = 0x4ce,      // reset buffers + post the abort WM_COMMAND
    DISPATCH_PLAYERLEFT = 0x4ea, // report + broadcast the leaving player
};

// Engine stat-dispatcher ids the Net cluster ships (names reconstructed from
// use; values load-bearing).
enum {
    STAT_PLAYERLEFT = 0x410,       // broadcast: a player has left
    STAT_PLAYERLEFT_LOCAL = 0x411, // report: the local view of the leaving player
    STAT_CONNECTING = 0x415,       // announce: connection attempt in progress
    STAT_VERSIONPACKET = 0x417,    // the version-announce packet stat id
    STAT_VERSIONMISMATCH = 0x418,  // announce: host/client version mismatch
    STAT_ACKLATENCY = 0x421,       // report: current worst ack latency
};

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
    i32 r = MultiDispatch("MULTI_PAUSE", MultiPauseCallback, 0);
    g_pauseGuard = 0;
    g_sharedFlag = 0;

    if (r == DISPATCH_RESYNC) {
        void* hwnd = ((CNetHwndHolder*)((CNetHwndHolder*)m_4)->m_4)->m_4;
        PostMessageA((HWND)hwnd, WM_COMMAND, 0x80d7, m_1c);
    }
}

// ---------------------------------------------------------------------------
// CNetMgr::OnOutOfSync
// Per-instance reentrancy-guarded fire of MULTI_OUTOFSYNC. Switches on the
// dispatch result: 0x4cc -> the same WM_COMMAND(0x80d7, m_1c) as Pause;
// 0x4cd -> nothing; otherwise -> WM_COMMAND(0x8023, 0).
RVA(0x000bae40, 0x84)
void CNetMgr::OnOutOfSync() {
    if (m_outOfSyncGuard) {
        return;
    }

    m_outOfSyncGuard = 1;
    m_584 = 0;
    i32 r = MultiDispatch("MULTI_OUTOFSYNC", MultiOutOfSyncCallback, 0);
    g_sharedFlag = 0;

    switch (r) {
        case DISPATCH_RESYNC: {
            void* hwnd = ((CNetHwndHolder*)((CNetHwndHolder*)m_4)->m_4)->m_4;
            PostMessageA((HWND)hwnd, WM_COMMAND, 0x80d7, m_1c);
            break;
        }
        case DISPATCH_RESET:
            break;
        default: {
            void* hwnd = ((CNetHwndHolder*)((CNetHwndHolder*)m_4)->m_4)->m_4;
            PostMessageA((HWND)hwnd, WM_COMMAND, 0x8023, 0);
            break;
        }
    }
}

// ---------------------------------------------------------------------------
// CNetMgr::ApplyCmdDelayDefaults  (C++ EH frame).
// Persists the command-timing config to the game RegistryHelper (the singleton's
// +0x38 member). Builds three value-name strings "m_configSection + _Suffix" via operator+
// (each a stack CString temp), writes m_cmdDelay under "_CmdDelay" and m_resend under
// "_Resend"; the "_DynCmdDelay" temp is built but its write is elided here. The
// three temporaries' dtors run under the C++ EH frame (=> /GX).
RVA(0x000b85a0, 0xd2)
void CNetMgr::ApplyCmdDelayDefaults() {
    Utils::RegistryHelper* reg = g_pGameMgr->m_38;

    CString cmdDelayName = m_configSection + "_CmdDelay";
    CString resendName = m_configSection + "_Resend";
    CString dynCmdName = m_configSection + "_DynCmdDelay";

    reg->SetValueDword((char*)(const char*)cmdDelayName, m_cmdDelay);
    reg->SetValueDword((char*)(const char*)resendName, m_resend);
}

// ---------------------------------------------------------------------------
// CNetMgr::SendStatBuf  (__thiscall).
// Core stat sender: sets the packet's bit7 flag, then ships the 0x10-byte
// packet to the local player's peer group via the DirectPlay set-data wrapper
// (m_peer->SetGroupDataFrom(localPlayer, flag, pkt, 0x10)). Returns the
// success bool (hr == 0).
RVA(0x000b91f0, 0x31)
i32 CNetMgr::SendStatBuf(CNetStatPacket* pkt, i32 flag) {
    pkt->m_0 |= 0x80;
    i32 hr = m_peer->SetGroupDataFrom((CNetPlayerEntry*)m_localPlayer, flag, (i32)pkt, 0x10);
    return hr == 0;
}

// ---------------------------------------------------------------------------
// CNetMgr::SendStatFlag  (__thiscall).
// Builds the 0x10-byte stat header {id, localPlayer.id} on the stack and ships
// it through SendStatBuf with the caller's flag.
RVA(0x000b9240, 0x38)
void CNetMgr::SendStatFlag(i32 id, i32 flag) {
    CNetStatPacket pkt;
    pkt.m_0 |= 0x80;
    pkt.m_4 = id;
    pkt.m_8 = ((CNetPlayerEntry*)m_localPlayer)->m_4;
    SendStatBuf(&pkt, flag);
}

// ---------------------------------------------------------------------------
// CNetMgr::SendNetStat  (__thiscall).
// Builds the 0x10-byte stat header {id, value} on the stack and ships it
// through SendStatBuf with the caller's flag.
RVA(0x000b9290, 0x32)
void CNetMgr::SendNetStat(i32 id, u32 value, i32 flag) {
    CNetStatPacket pkt;
    pkt.m_0 |= 0x80;
    pkt.m_4 = id;
    pkt.m_8 = value;
    SendStatBuf(&pkt, flag);
}

// ---------------------------------------------------------------------------
// CNetMgr::SendStatFrom  (__thiscall).
// No-op on a null packet; otherwise ships the caller's packet to the local
// player's peer group via SetGroupDataFrom(localPlayer, c, pkt, b).
RVA(0x000b92e0, 0x34)
i32 CNetMgr::SendStatFrom(CNetStatPacket* pkt, i32 b, i32 c) {
    if (pkt == 0) {
        return 0;
    }
    i32 hr = m_peer->SetGroupDataFrom((CNetPlayerEntry*)m_localPlayer, c, (i32)pkt, b);
    return hr == 0;
}

// ---------------------------------------------------------------------------
// CNetMgr::SendStatPair  (__thiscall).
// Null-recipient -> 0; otherwise sets the packet's bit7 flag and ships both
// through SetGroupData2(localPlayer, recipient, c, packet, 0x10).
RVA(0x000b9330, 0x41)
i32 CNetMgr::SendStatPair(CNetPlayerEntry* recipient, CNetStatPacket* pkt, i32 c) {
    if (recipient == 0) {
        return 0;
    }
    pkt->m_0 |= 0x80;
    i32 hr = m_peer->SetGroupData2(
        (CNetPlayerEntry*)m_localPlayer, recipient, c, (i32)pkt, 0x10
    );
    return hr == 0;
}

// ---------------------------------------------------------------------------
// CNetMgr::SendStatTo  (__thiscall).
// No-op on a null recipient; otherwise builds the 0x10-byte stat header
// {flag, id, localPlayer.id} on the stack and ships it to that one player via
// SendStatPair.
RVA(0x000b93a0, 0x47)
i32 CNetMgr::SendStatTo(CNetPlayerEntry* recipient, i32 id, i32 c) {
    if (recipient == 0) {
        return 0;
    }
    CNetStatPacket pkt;
    pkt.m_0 |= 0x80;
    pkt.m_4 = id;
    pkt.m_8 = ((CNetPlayerEntry*)m_localPlayer)->m_4;
    return SendStatPair(recipient, &pkt, c);
}

// ---------------------------------------------------------------------------
// CNetMgr::SendStat3  (__thiscall).
// The 3-arg stat sender OnDropPlayer fires: builds the 0x10-byte stat header
// {flag, value, localPlayer.id} on the stack and ships it through SetData
// (a=localPlayer.id, b=id, c=flag). Returns the success bool.
RVA(0x000b9410, 0x51)
i32 CNetMgr::SendStat3(i32 id, u32 value, i32 flag) {
    CNetStatPacket pkt;
    pkt.m_0 |= 0x80;
    pkt.m_4 = value;
    pkt.m_8 = ((CNetPlayerEntry*)m_localPlayer)->m_4;
    i32 hr = m_peer->SetData(((CNetPlayerEntry*)m_localPlayer)->m_4, id, flag, (i32)&pkt, 0x10);
    return hr == 0;
}

// ---------------------------------------------------------------------------
// CNetMgr::SendStatPairRaw  (__thiscall).
// Forwards a caller packet to one recipient via SetGroupData2 (no bit7 stamp,
// caller-supplied size): null-recipient or null-packet -> 0, else ships
// SetGroupData2(localPlayer, recipient, c, pkt, size). Returns the success bool.
RVA(0x000b9500, 0x46)
i32 CNetMgr::SendStatPairRaw(CNetPlayerEntry* recipient, void* pkt, i32 size, i32 c) {
    if (recipient == 0) {
        return 0;
    }
    if (pkt == 0) {
        return 0;
    }
    i32 hr = m_peer->SetGroupData2(
        (CNetPlayerEntry*)m_localPlayer, recipient, c, (i32)pkt, size
    );
    return hr == 0;
}

// ---------------------------------------------------------------------------
// CNetMgr::SendStatValue  (__thiscall).
// Builds the 0x10-byte stat header {flag, statId, value} on the stack and ships
// it through SetData (a=localPlayer.id, b=id, c=flag). Returns the success bool.
RVA(0x000b9570, 0x53)
i32 CNetMgr::SendStatValue(i32 id, i32 statId, i32 value, i32 flag) {
    CNetStatPacket pkt;
    pkt.m_0 |= 0x80;
    pkt.m_4 = statId;
    pkt.m_8 = value;
    i32 hr = m_peer->SetData(
        ((CNetPlayerEntry*)m_localPlayer)->m_4, id, flag, (i32)&pkt, 0x10
    );
    return hr == 0;
}

// ---------------------------------------------------------------------------
// CNetMgr::PollSession  (__thiscall).
// Pumps the DirectPlay receive queue. Bails (returns 0) if there is no local
// player. Asks the peer's interface how many messages are pending for the local
// player (GetMessageCount, slot 0x44), then receives + dispatches up to that
// many (each Receive into the shared g_recvBuffer, slot 0x64): a nonzero HRESULT
// is reported (NetMgr.h:0x141) and breaks; a message not addressed from the
// local player is handed to the engine dispatcher (Stub_0b9750) and counted.
// Stops early if the abort latch (m_pollAbort) is set. Returns the dispatched
// count.
// @early-stop
// frame-size + regalloc + COM-slot-aliasing wall (~60%): logic, the null guard,
// the inlined GetMessageCount (slot 0x44) probe with the neg/sbb/not/and HRESULT
// mask, the receive loop (Receive slot 0x64), the ReportError on failure, and the
// per-message DispatchRecvMsg are all reproduced - but retail's frame is 0x10
// (mine 0xc), it pins this=esi / 0=edi / count=ebx across the function, and it
// OVERLAPS the receive {size,idFrom} stack slot (one local serves lpidFrom AND
// lpdwDataSize) which a clean C++ shape won't express. See
// docs/patterns/stack-buffer-size-drives-frame.md. Deferred to the final sweep.
RVA(0x000b95f0, 0x10f)
i32 CNetMgr::PollSession() {
    if (m_localPlayer == 0) {
        return 0;
    }

    i32 count;
    if (m_localPlayer == 0) {
        count = 0;
    } else {
        IDirectPlay4Z* dp = m_peer->m_directPlay;
        count = 0;
        i32 hr = dp->vtbl->GetMessageCount(
            dp, ((CNetPlayerEntry*)m_localPlayer)->m_4, &count
        );
        if (hr) {
            count = 0;
        }
    }
    if (count <= 0) {
        return 0;
    }

    i32 dispatched = 0;
    i32 sender = 0;
    while (count > 0) {
        if (m_pollAbort) {
            break;
        }

        i32 size = 0x800;
        i32 idTo = ((CNetPlayerEntry*)m_localPlayer)->m_4;
        IDirectPlay4Z* dp = m_peer->m_directPlay;
        i32 hr = dp->vtbl->Receive(dp, &size, &idTo, 1, (void*)g_recvBuffer, &size);
        if (hr) {
            ReportError("c:\\proj\\incs\\netmgr.h", 0x141, hr, 0);
            if (hr) {
                break;
            }
        }
        count--;
        if (sender != ((CNetPlayerEntry*)m_localPlayer)->m_4) {
            DispatchRecvMsg(sender, g_recvBuffer, size);
            dispatched++;
        }
        if (hr) {
            break;
        }
    }
    return dispatched;
}

// ---------------------------------------------------------------------------
// CNetMgr::GetName  (__thiscall).
// Returns the +0x8 CString member by value (NRV-construct the return slot as a
// copy of m_8).
RVA(0x000ba170, 0x20)
CString CNetMgr::GetName() {
    return m_8;
}

// ---------------------------------------------------------------------------
// CNetMgr::HandleControlMsg  (__thiscall).
// Dispatches a network control message on its +0x0 code: 3 -> the sprite/menu
// handler; 5 -> the player-left path (when sub-code 1, report+teardown then set
// the shared player-left flag); 0x31 -> latch m_sessionTerminated; 0x101 ->
// latch m_useChannelLatency. Anything else (or a null/out-of-range code) -> 0;
// the matched cases return 1.
// @early-stop
// jump-table-data-overlap scoring artifact (objdiff 0%, CODE byte-exact: all 37
// dispatch+case bytes match retail, incl. the two-level byte-index + jump-ptr
// table). objdiff mis-scores the inline .rdata table region against the
// differently-named switchdataD_004ba2xx symbols. See
// docs/patterns/jumptable-data-overlap.md (topic:scoring-artifact). Logic correct.
RVA(0x000ba1a0, 0x83)
i32 CNetMgr::HandleControlMsg(CNetCtrlMsg* msg, i32 arg2) {
    if (msg == 0) {
        return 0;
    }

    switch (msg->m_0) {
        case 3:
            HandleSpriteMsg(msg);
            return 1;
        case 5:
            if (msg->m_4 != 1) {
                return 1;
            }
            OnPlayerLeft(msg->m_8);
            g_playerLeftFlag = 1;
            return 1;
        case 0x31:
            m_useChannelLatency = 1;
            return 1;
        case 0x101:
            m_sessionTerminated = 1;
            return 1;
        default:
            return 0;
    }
}

// ---------------------------------------------------------------------------
// CNetMgr::OnPlayerLeft  (__thiscall; /GX EH frame).
// Tears down a leaving player and announces it. Looks the player's data blob up
// in the peer (GetPlayerData); ignores the local player. Resolves the player's
// slot (m_4->FindPlayer); requires its +0x20 / +0x14 gates set. Releases the
// slot's global flag (g_netSlotTable[slot->m_008] via SetNetSlot, decrement
// g_activePlayers if armed), clears its list link, builds "<name> has left the
// game." and appends it to the chat log (m_4->m_5c->AddItem, type 0x20 data
// 0x11), and unlinks the blob (RemovePlayerObj). If the channel selector is set
// and not yet connected, fires the rejoin finalizer and sets g_playerLeftFlag.
// The two CString temps' dtors run under the /GX frame.
RVA(0x000ba3b0, 0x17f)
i32 CNetMgr::OnPlayerLeft(i32 playerId) {
    CNetPlayerObj* blob = (CNetPlayerObj*)m_peer->GetPlayerData(playerId);
    if ((i32)blob == m_localPlayer) {
        return 0;
    }

    CNetGameMgr* gm = (CNetGameMgr*)m_4;
    GruntzPlayer* slot = gm->FindPlayer(playerId);
    if (slot == 0) {
        return 0;
    }
    if (slot->m_020 == 0) {
        return 0;
    }
    if (slot->m_014 == 0) {
        return 0;
    }

    if (slot->m_030 != 0) {
        slot->m_030 = 0;
        g_activePlayers--;
    }
    slot->m_020 = 0;
    SetNetSlot(slot->m_008, 1);

    CString line = slot->GetName() + " has left the game.";
    ((CNetGameMgr*)m_4)->m_5c->AddItem((char*)(const char*)line, 0x20, 0x11);

    if (blob != 0) {
        m_peer->RemovePlayerObj(blob);
    }
    if (m_useChannelLatency != 0 && m_connected == 0) {
        RejoinIfNeeded(0);
        g_playerLeftFlag = 1;
    }
    return 1;
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
// last call (m_lastFrameDelta) and the new stamp (m_lastFrameTime). If the frame came in under 0x1f
// ms it busy-waits the remainder (ActiveWait) and re-stamps; otherwise, if the
// frame ran long (> 0x28 ms) and the sync gate m_syncGate is set, it flips the
// global low-bit sync toggle and returns it.
RVA(0x000bc070, 0x73)
u32 CNetMgr::FrameSyncWait() {
    u32 now = timeGetTime();
    u32 delta = now - m_lastFrameTime;
    u32 ret = 0;
    m_lastFrameDelta = delta;
    m_lastFrameTime = now;

    if (delta <= 0x1e) {
        Utils::WinAPI::ActiveWait(0x1f - delta);
        m_lastFrameTime = (now - m_lastFrameDelta) + 0x1f;
        return 0;
    }
    if (delta > 0x28 && m_syncGate) {
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
    i32 r = MultiDispatch("MULTI_DROPPLAYER", MultiDropPlayerCallback, 0);
    g_dropGuard = 0;
    g_sharedFlag = 0;

    switch (r) {
        case DISPATCH_RESET:
            m_session->ResetCmdBuffers();
            break;
        case DISPATCH_ABORT: {
            m_session->ResetCmdBuffers();
            void* hwnd = ((CNetHwndHolder*)((CNetHwndHolder*)m_4)->m_4)->m_4;
            PostMessageA((HWND)hwnd, WM_COMMAND, 0x8023, 0);
            break;
        }
        case DISPATCH_PLAYERLEFT:
            if (g_dropPlayerId != -999) {
                if (m_peer->FindPlayerById(g_dropPlayerId)) {
                    SendStat3(g_dropPlayerId, STAT_PLAYERLEFT_LOCAL, 1);
                }
            }
            SendNetStat(STAT_PLAYERLEFT, g_dropPlayerId, 1);
            AckDropPlayer(g_dropPlayerId);
            m_session->ResetCmdBuffers();
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
// interface (m_peer) or local player descriptor (m_localPlayer). Announces "connecting"
// (stat 0x415), clears the admit flag m_admitted, then loops: each pass times out at
// 60s or on Esc (-> status 0x8022, fail), pumps the receive queue, and reports +
// fails on any of the session-state flags (terminated / removed / closed / full
// / version-mismatch). Returns 1 once m_admitted latches (admitted), 0 on any failure.
RVA(0x000bca50, 0x155)
i32 CNetMgr::WaitForConnect() {
    if (m_peer == 0) {
        return 0;
    }
    if (m_localPlayer == 0) {
        return 0;
    }

    SendStatFlag(STAT_CONNECTING, 1);
    m_admitted = 0;
    u32 start = timeGetTime();
    if (m_admitted != 0) {
        return 1;
    }

    do {
        u32 now = timeGetTime();
        if (now > start + 60000 || ((i32)GetAsyncKeyState(VK_ESCAPE) & 0x80000000)) {
            ReportStatusId(0x8022, 0);
            return 0;
        }
        PollSession();
        if (m_sessionTerminated) {
            ReportNetError("The game session has been terminated.", 0);
            return 0;
        }
        if (m_removedFromGame) {
            ReportNetError("You have been removed from the game by the host.", 0);
            return 0;
        }
        if (m_gameClosed) {
            ReportNetError("This game is closed.", 0);
            return 0;
        }
        if (m_gameFull) {
            ReportNetError("This game is already full.", 0);
            return 0;
        }
        if (m_versionMismatch) {
            ReportNetError(
                "This version is not the same as the host computer's version of the game.",
                0
            );
            return 0;
        }
    } while (m_admitted == 0);
    return 1;
}

// ---------------------------------------------------------------------------
// CNetMgr::ResetPlayerCommands  (__thiscall).
// Flushes the resend buffers for one player's command slot. No-op unless
// connected (m_connected). Looks the player's slot up in the session (m_session); if found
// and not already reset (slot->m_4 == 0), latches it, then for each command
// sequence number in the slot's window ([(seq0+1)..(seq0+1)+3] scaled by the
// per-command delay m_cmdDelay) re-dispatches the command through m_4's queue and
// drops it from the slot. Finally clears the slot's two command ranges.
RVA(0x000bcf20, 0xaf)
i32 CNetMgr::ResetPlayerCommands(i32 id) {
    if (m_connected == 0) {
        return 0;
    }

    CNetCmdSlot* slot = m_session->FindCmdSlot(id);
    if (slot == 0) {
        return 0;
    }
    if (slot->m_4 != 0) {
        return 0;
    }

    slot->Touch();
    i32 seq = (slot->m_14 + 1) * (i32)m_cmdDelay;
    i32 end = seq + (i32)m_cmdDelay * 3;
    for (; seq < end; seq++) {
        ((CNetSubObject*)m_4)->m_6c->Dispatch(*slot->m_c, seq);
        slot->RemoveCmd(seq / (i32)m_cmdDelay);
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
    u32 latency = GetMaxAckLatency();
    SendNetStat(STAT_ACKLATENCY, latency, 0);
}

// ---------------------------------------------------------------------------
// CNetMgr::GetMaxAckLatency  (pure leaf; __thiscall).
// Returns the largest latency value across the four network slots. When the
// branch selector m_useChannelLatency is set the values come from the inline m_channelLatency[4] channel
// array (every entry counted); otherwise they come from the four per-player
// slots hanging off m_4 (stride 0x238), each counted only when BOTH of its
// "slot active" gate flags (m_164, m_170) are nonzero.
RVA(0x000bd030, 0x5d)
u32 CNetMgr::GetMaxAckLatency() {
    u32 max = 0;

    if (m_useChannelLatency != 0) {
        for (i32 i = 0; i < 4; i++) {
            if (m_channelLatency[i] > max) {
                max = m_channelLatency[i];
            }
        }
    } else {
        CNetPlayerSlot* slot = (CNetPlayerSlot*)m_4;
        for (i32 i = 0; i < 4; i++) {
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
// version locals. On any mismatch it latches m_versionMismatch, and - if a connection was
// already up (m_connected) - reports the canned "version mismatch" diagnostic and
// posts WM_COMMAND(0x8023) to the engine window; then fires stat 0x418 and
// sleeps 250ms before returning.
RVA(0x000bd0b0, 0x9a)
void CNetMgr::HandleVersionCheck(CNetVersionMsg* msg) {
    if (msg == 0) {
        return;
    }

    i32 mismatch = 0;
    if (g_localVersion != msg->m_1c) {
        mismatch = 1;
    }
    if (g_remoteVersion != msg->m_18) {
        mismatch = 1;
    }

    if (mismatch) {
        i32 wasConnected = m_connected;
        m_versionMismatch = 1;
        if (wasConnected) {
            ReportVersionMsg(
                "This version is not the same as the host computer's version of the game.",
                0
            );
            void* hwnd = ((CNetHwndHolder*)((CNetHwndHolder*)m_4)->m_4)->m_4;
            PostMessageA((HWND)hwnd, WM_COMMAND, 0x8023, 0);
        }
    }
    if (mismatch) {
        SendStatFlag(STAT_VERSIONMISMATCH, 1);
        Sleep(0xfa);
    }
}

// ---------------------------------------------------------------------------
// CNetMgr::AnnounceVersion  (__thiscall).
// Builds a 0x20-byte version-announce packet on the stack (flag byte, the
// CButeMgr config word, g_cfgWord, stat id 0x417, and the local/remote version
// pair) and ships it through the engine stat dispatcher as stat 0x417.
RVA(0x000bd180, 0x66)
void CNetMgr::AnnounceVersion(i32 param) {
    CNetVersionPacket packet;
    memset(&packet, 0, sizeof(packet));

    packet.m_0 |= 0x80;
    packet.m_18 = g_remoteVersion;
    packet.m_c = g_cfgWord;
    packet.m_8 = g_buteMgrField4;
    packet.m_1c = g_localVersion;
    packet.m_10 = STAT_VERSIONPACKET;

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

    if (m_releaseIface != 0) {
        m_releaseIface->vtbl->Release(m_releaseIface);
        m_releaseIface = 0;
    }
    // The DirectPlay interface releases through the same IUnknown-shaped vtable
    // (Slot10 then a re-read + Release) - the same COM object, viewed as
    // INetReleasable. The reference re-reads m_directPlay before each call,
    // matching retail's reload of [this+0x18].
    INetReleasable*& dp = *(INetReleasable**)&m_directPlay;
    if (dp != 0) {
        dp->vtbl->Slot10(dp);
        INetReleasable* again = dp;
        again->vtbl->Release(again);
        dp = 0;
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
    m_groupSelId = 0;
    m_groupSel = 0;
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
    i32 sel = (i32)SendMessageA((HWND)hList, LB_GETCURSEL, 0, 0);
    if (sel == -1) {
        return 0;
    }
    if (sel < 0) {
        return 0;
    }
    if (sel >= *(i32*)((char*)this + 0x28)) {
        return 0;
    }
    i32 data = (i32)SendMessageA((HWND)hList, LB_GETITEMDATA, sel, 0);
    if (data == -1) {
        return 0;
    }
    if (data == 0) {
        return 0;
    }
    m_groupSel = data;
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
    i32 hr = com->vtbl->EnumPlayers(com, desc, a, (void*)&NetEnumPlayerCb, this, b);
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
    m_playerSelId = 0;
    m_playerSel = 0;
}

// ---------------------------------------------------------------------------
// CNetMgr::ReadPlayerSel  (__thiscall).
// As ReadGroupSel but for the +0x44 count / +0x74 latch list box.
RVA(0x00178820, 0x78)
i32 CNetMgr::ReadPlayerSel(void* hList) {
    if (hList == 0) {
        return 0;
    }
    i32 sel = (i32)SendMessageA((HWND)hList, LB_GETCURSEL, 0, 0);
    if (sel == -1) {
        return 0;
    }
    if (sel < 0) {
        return 0;
    }
    if (sel >= *(i32*)((char*)this + 0x44)) {
        return 0;
    }
    i32 data = (i32)SendMessageA((HWND)hList, LB_GETITEMDATA, sel, 0);
    if (data == -1) {
        return 0;
    }
    if (data == 0) {
        return 0;
    }
    m_playerSel = data;
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
    m_sessionSelId = 0;
    m_sessionSel = 0;
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
        ((CObList*)((char*)this + 0x54))->RemoveAt(pos);
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CNetMgr::FindPlayerById  (pure leaf; __thiscall).
// Walks the m_58 player list and returns the first entry whose id (+0x4) equals
// the requested id, or null if the list is empty / no entry matches.
RVA(0x00178e90, 0x20)
CNetPlayerEntry* CNetMgr::FindPlayerById(i32 id) {
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
void* CNetMgr::GetPlayerData(i32 id) {
    u32 size;
    void* data;
    data = 0;
    size = 4;
    i32 hr = m_directPlay->vtbl->GetData2(m_directPlay, id, &data, &size, 1);
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
    i32 hr = m_directPlay->vtbl->SetData5(m_directPlay, ida, idb, c, d, e);
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
i32 CNetMgr::SetData(i32 a, i32 b, i32 c, i32 d, i32 e) {
    i32 hr = m_directPlay->vtbl->SetData5(m_directPlay, a, b, c, d, e);
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
i32 CNetMgr::SetGroupDataFrom(CNetPlayerEntry* a, i32 c, i32 d, i32 e) {
    i32 ida = a ? a->m_4 : 0;
    i32 hr = m_directPlay->vtbl->SetData5(m_directPlay, ida, 0, c, d, e);
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
i32 CNetMgr::EnumSessions(void* desc, void* ctx) {
    if (desc == 0) {
        return 0;
    }

    memset(desc, 0, 0x28);
    *(i32*)desc = 0x28;
    i32 hr = m_directPlay->vtbl->Enum2(m_directPlay, desc, ctx);
    if (hr) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x52a, hr, 0);
        return 0;
    }
    return 1;
}
