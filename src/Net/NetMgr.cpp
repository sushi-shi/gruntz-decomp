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
#include <stdio.h>  // sprintf (the chat-line formatter)

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
    i32 hr = m_peer->SetGroupData2((CNetPlayerEntry*)m_localPlayer, recipient, c, (i32)pkt, 0x10);
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
    i32 hr = m_peer->SetGroupData2((CNetPlayerEntry*)m_localPlayer, recipient, c, (i32)pkt, size);
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
    i32 hr = m_peer->SetData(((CNetPlayerEntry*)m_localPlayer)->m_4, id, flag, (i32)&pkt, 0x10);
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
        i32 hr = dp->vtbl->GetMessageCount(dp, ((CNetPlayerEntry*)m_localPlayer)->m_4, &count);
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
// CNetMgr::GetConfigNameA / GetConfigNameB  (__thiscall).
// Return the +0x5b4 / +0x5b8 config-name CString members by value (same NRV
// copy-construct shape as GetName).
RVA(0x000b6090, 0x23)
CString CNetMgr::GetConfigNameA() {
    return m_5b4;
}

RVA(0x000b60d0, 0x23)
CString CNetMgr::GetConfigNameB() {
    return m_5b8;
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

// ---------------------------------------------------------------------------
// CNetMgr::AckDropPlayer  (__thiscall).
// Finalizes a dropped player. When the host-mode flag (m_534) is clear it records
// the drop (RecordDropPlayer), then looks the player's command slot up
// (m_session->FindCmdSlot) and, if found, latches+resets it (Touch + FullReset)
// and arms both the slot and its command-list head (slot->m_0 = 1,
// slot->m_c[+0x2c] = 1). In host mode it instead tears the player down directly
// (OnPlayerLeft) and flushes their resend buffers (ResetPlayerCommands).
RVA(0x000ba590, 0x63)
void CNetMgr::AckDropPlayer(i32 id) {
    if (m_534 == 0) {
        RecordDropPlayer(0, id);
        CNetCmdSlot* slot = m_session->FindCmdSlot(id);
        if (slot != 0) {
            slot->Touch();
            slot->FullReset();
            slot->m_0 = 1;
            slot->m_c[0xb] = 1;
        }
        return;
    }

    OnPlayerLeft(id);
    ResetPlayerCommands(id);
}

// ---------------------------------------------------------------------------
// CNetMgr::ResolveLocalPlayer  (__thiscall).
// Resolves the local player descriptor: bails (0) with no peer; otherwise looks
// the local player id (m_5c0) up in the peer's player list and latches the
// result into m_5bc, returning whether one was found.
RVA(0x000ba7d0, 0x2e)
i32 CNetMgr::ResolveLocalPlayer() {
    if (m_peer == 0) {
        return 0;
    }
    m_localPlayer = (i32)m_peer->FindPlayerById(m_5c0);
    return m_localPlayer != 0;
}

// ---------------------------------------------------------------------------
// CNetMgr::BroadcastChannelTable  (__thiscall).
// Serializes the whole four-channel ack/state table (m_4 + 0x150, stride 0x238)
// into a 0x88-byte stat packet (stat id 0x3f8) - one 0x20-byte record per channel
// (each carries the channel's gate/id bytes plus its name copied in via the
// engine GetName CString + inline strcpy) - then ships it: to one recipient via
// SendStatPairRaw when given, else to the local player's group via SendStatFrom.
// @early-stop
// regalloc + load-width wall (~86%): the logic, the 0x88 packet memset, the
// per-channel record fill, the GetName CString + inline strcpy and the
// recipient/group send branch are all reproduced - but retail anchors the record
// ptr (ebx) one byte lower, dword-loads each channel field before the byte store
// (movl;movb vs my movb;movb), and orders the slot-address lea operands
// (eax,ebp vs ebp,eax) differently; none move under source restructuring (the
// inverse parse ParseChannelTable is 99.9%). Deferred to the final sweep.
RVA(0x000ba810, 0x11c)
i32 CNetMgr::BroadcastChannelTable(CNetPlayerEntry* recipient) {
    char packet[0x88];
    memset(packet, 0, 0x88);
    packet[0] |= 0x80;
    *(i32*)(packet + 4) = 0x3f8;

    i32 off = 0;
    char* rec = packet + 9;
    for (; off < 0x8e0; off += 0x238) {
        CNetChannel* ch = (CNetChannel*)((char*)m_4 + 0x150 + off);
        if (ch != 0) {
            rec[-1] = (char)ch->m_20;
            rec[0] = (char)ch->m_8;
            rec[1] = (char)ch->m_14;
            rec[2] = (char)ch->m_10;
            rec[5] = (char)ch->m_1c;
            rec[4] = (char)ch->m_228;
            *(i32*)(rec + 7) = ch->m_18;
            CString name = ch->GetName();
            strcpy(rec + 0xb, (const char*)name);
        }
        rec += 0x20;
    }

    if (recipient != 0) {
        return SendStatPairRaw(recipient, packet, 0x88, 1);
    }
    return SendStatFrom((CNetStatPacket*)packet, 0x88, 1);
}

// ---------------------------------------------------------------------------
// CNetMgr::ParseChannelTable  (__thiscall).
// The inverse of BroadcastChannelTable: parses a received 0x88 packet back into
// the four-channel table. Bails (0) on a null packet; (re)initializes the global
// net-slot table when not channel-latency mode, then for each channel copies the
// record bytes back, restores its name CString, and - in non-channel mode for a
// newly active channel - frees its net slot (SetNetSlot(id, 0)).
// @early-stop
// regalloc SIB-base wall (~99.9%): the whole body is byte-exact, the single
// residual is the slot-address `lea 0x150(%eax,%ebp)` vs my `lea 0x150(%ebp,%eax)`
// (SIB base/index swap of m_4 vs the loop counter); not steerable from source
// (m_4 is reloaded each iteration so a running pointer diverges). Final sweep.
RVA(0x000ba980, 0xca)
i32 CNetMgr::ParseChannelTable(void* packet) {
    if (packet == 0) {
        return 0;
    }
    if (m_useChannelLatency == 0) {
        ResetNetSlots();
    }

    i32 off = 0;
    char* rec = (char*)packet + 9;
    for (; off < 0x8e0; off += 0x238) {
        CNetChannel* ch = (CNetChannel*)((char*)m_4 + 0x150 + off);
        if (ch != 0) {
            ch->m_20 = (u8)rec[-1];
            ch->m_8 = (u8)rec[0];
            ch->m_14 = (u8)rec[1];
            ch->m_10 = (u8)rec[2];
            if (rec[5] != 0) {
                ch->m_1c = 1;
            } else {
                ch->m_1c = 0;
            }
            ch->m_228 = (u8)rec[4];
            ch->m_4 = rec + 0xb;
            ch->m_18 = *(i32*)(rec + 7);
            if (m_useChannelLatency == 0 && ch->m_20 != 0) {
                SetNetSlot(ch->m_8, 0);
            }
        }
        rec += 0x20;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CNetMgr::RegisterChannelFrom  (__thiscall).
// Thin forwarder: fixes the c=1 / idx=0 register arguments and tail-calls
// RegisterChannel with the caller's four fields (name, id, e, f).
RVA(0x000baa90, 0x20)
i32 CNetMgr::RegisterChannelFrom(const char* name, i32 b, i32 e, i32 f) {
    return RegisterChannel(name, b, 1, 0, e, f);
}

// ---------------------------------------------------------------------------
// CNetMgr::RegisterChannel  (__thiscall; /GX EH frame).
// Creates or refreshes one channel slot. Bails (0) if the table is full
// (m_4->CountActiveChannels >= 4). Tries the requested index (idx in [0,4]) when
// its slot is free, else linear-scans for the first inactive slot; bails if none.
// Stores the supplied fields into the slot (name CString into +0x4, id/flags) and
// marks it active. The CString temp's dtor runs under the /GX frame.
// @early-stop
// /GX EH-state cookie wall (~93%): the whole body is byte-exact (the full-table
// guard, the requested/scan slot selection, the SetNetSlot + scoped CString temp,
// every field store), but retail's __except prologue pushes the scope cookie 0x8
// where our cl pushes 0x0 and references its own funclet, the residual being the
// TU-wide EH-state numbering. See docs/patterns/gx-scoped-local-eh-frame-size.md +
// eh-state-numbering-base.md. Deferred to the final sweep.
RVA(0x000baac0, 0x12e)
i32 CNetMgr::RegisterChannel(const char* name, i32 id, i32 c, i32 d, i32 idx, i32 e) {
    if (((CNetGameMgr*)m_4)->CountActiveChannels(1) >= 4) {
        return 0;
    }

    CNetChannel* ch = 0;
    if (idx >= 0 && idx <= 4) {
        ch = (CNetChannel*)((char*)m_4 + idx * 0x238 + 0x150);
        if (ch != 0 && ch->m_20 != 0) {
            ch = 0;
        }
    }
    if (ch == 0) {
        CNetChannel* p = (CNetChannel*)((char*)m_4 + 0x150);
        for (i32 i = 0; i < 4; i++) {
            ch = p;
            if (p != 0 && p->m_20 == 0) {
                break;
            }
            ch = 0;
            p = (CNetChannel*)((char*)p + 0x238);
        }
        if (ch == 0) {
            return 0;
        }
    }

    SetNetSlot(id, 0);
    {
        CString temp(name);
        ch->m_4 = temp;
    }
    ch->m_8 = id;
    ch->m_14 = c;
    ch->m_10 = d;
    ch->m_1c = 0;
    ch->m_18 = e;
    ch->m_20 = 1;
    ch->m_22c = 0;
    ch->m_230 = 0;
    return 1;
}

// ---------------------------------------------------------------------------
// CNetMgr::RegisterChannelRec  (__thiscall).
// Unpacks a received register record (a CNetCtrlMsg-shaped blob): no-op (returns
// 1) unless its +0x8 active byte is set; otherwise pulls the name pointer (+0x14)
// and the four header bytes (+0x9..+0xc) plus the id dword (+0x10) and registers
// the channel.
RVA(0x000bac40, 0x38)
i32 CNetMgr::RegisterChannelRec(void* rec) {
    u8* r = (u8*)rec;
    if (r[8] != 0) {
        RegisterChannel((const char*)(r + 0x14), r[9], r[0xa], r[0xb], r[0xc], *(i32*)(r + 0x10));
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CNetMgr::RemoveChannel  (__thiscall).
// Tears down the channel slot at the given index: no-op on a null slot; returns 0
// if it was already inactive; otherwise clears its active gate and frees its net
// slot (SetNetSlot(id, 1)). Returns 1 when a slot was removed.
RVA(0x000bac90, 0x46)
i32 CNetMgr::RemoveChannel(i32 idx) {
    CNetChannel* ch = (CNetChannel*)((char*)m_4 + idx * 0x238 + 0x150);
    if (ch == 0) {
        return 0;
    }
    if (ch->m_20 == 0) {
        return 0;
    }
    ch->m_20 = 0;
    SetNetSlot(ch->m_8, 1);
    return 1;
}

// ---------------------------------------------------------------------------
// CNetMgr::OnPauseChannel  (__thiscall).
// No-op (returns 0) unless connected (m_580); otherwise announces the pause
// (SendStatFlag(0x407, 1)) and runs the multiplayer pause handler (OnMultiPause).
RVA(0x000bad00, 0x2d)
i32 CNetMgr::OnPauseChannel() {
    if (m_connected == 0) {
        return 0;
    }
    SendStatFlag(0x407, 1);
    OnMultiPause();
    return 1;
}

// ---------------------------------------------------------------------------
// CNetMgr::BroadcastOneChannel  (__thiscall).
// Serializes one channel descriptor into a 0x2c-byte stat packet (stat id 0x3fa,
// the same record byte layout as a single BroadcastChannelTable slot plus the
// channel name strcpy'd in) and ships it to the local player's group via
// SendStatFrom.
// @early-stop
// load-width wall (~87%): the whole shape matches retail - the 0x2c packet build,
// the frameless CString name temp (scoped to elide the /GX frame), the inline
// strcpy and the SendStatFrom send are byte-aligned - but retail dword-loads each
// i32 channel field before the byte store (movl;movb) where our cl byte-loads it
// (movb;movb), shuffling the field-store order. Same wall as
// BroadcastChannelTable; not steerable from source. Deferred to the final sweep.
RVA(0x000baf00, 0xb2)
i32 CNetMgr::BroadcastOneChannel(CNetChannel* ch) {
    char packet[0x2c];
    memset(packet, 0, 0x2c);
    packet[0] |= 0x80;
    *(i32*)(packet + 4) = 0x3fa;
    *(i32*)(packet + 8) = ch->m_0;

    packet[0xd] = ch->m_8;
    packet[0xe] = ch->m_14;
    packet[0xf] = ch->m_10;
    packet[0x12] = ch->m_1c;
    packet[0xc] = 1;
    packet[0x11] = ch->m_228;
    {
        i32 id = ch->m_18;
        CString name = ch->GetName();
        *(i32*)(packet + 0x18) = id;
        strcpy(packet + 0x18, (const char*)name);
    }

    return SendStatFrom((CNetStatPacket*)packet, 0x2c, 1);
}

// ---------------------------------------------------------------------------
// CNetMgr::ParseOneChannel  (__thiscall).
// The inverse of BroadcastOneChannel: parses a single-channel record into the
// channel slot named by the record's index (rec+0x8, must be in [0,4]). Bails (0)
// on a null record / out-of-range index / null slot. Restores the channel name
// CString (+0x4) and the header bytes, then marks the slot active.
RVA(0x000baff0, 0x88)
i32 CNetMgr::ParseOneChannel(void* rec) {
    if (rec == 0) {
        return 0;
    }
    u8* r = (u8*)rec;
    i32 idx = *(i32*)(r + 8);
    if (idx < 0 || idx >= 4) {
        return 0;
    }
    CNetChannel* ch = (CNetChannel*)((char*)m_4 + idx * 0x238 + 0x150);
    if (ch == 0) {
        return 0;
    }

    ch->m_4 = (char*)(r + 0x18);
    ch->m_8 = r[0xd];
    ch->m_10 = r[0xf];
    if (r[0x12] != 0) {
        ch->m_1c = 1;
    } else {
        ch->m_1c = 0;
    }
    ch->m_228 = r[0x11];
    ch->m_14 = r[0xe];
    ch->m_18 = *(i32*)(r + 0x14);
    ch->m_20 = 1;
    return 1;
}

// ---------------------------------------------------------------------------
// CNetMgr::SendChannelStat422  (__thiscall).
// Stamps the static stat-0x422 packet (flag bit7, value 0) and ships it to the
// local player's group via SetGroupDataFrom.
RVA(0x000bb0b0, 0x44)
i32 CNetMgr::SendChannelStat422() {
    g_chanStat422_id = 0x422;
    g_chanStat422_flag |= 0x80;
    g_chanStat422_val = 0;
    m_peer->SetGroupDataFrom((CNetPlayerEntry*)m_localPlayer, 1, (i32)&g_chanStat422_flag, 0xc);
    return 1;
}

// ---------------------------------------------------------------------------
// CNetMgr::SendChannelStat423  (__thiscall).
// As SendChannelStat422 but for the static stat-0x423 packet.
RVA(0x000bb120, 0x44)
i32 CNetMgr::SendChannelStat423() {
    g_chanStat423_id = 0x423;
    g_chanStat423_flag |= 0x80;
    g_chanStat423_val = 0;
    m_peer->SetGroupDataFrom((CNetPlayerEntry*)m_localPlayer, 1, (i32)&g_chanStat423_flag, 0xc);
    return 1;
}

// ---------------------------------------------------------------------------
// CNetMgr::BroadcastChatLine  (__thiscall).
// Assembles a chat line and broadcasts it (stat 0x3f0). Bails (0) on a null/empty
// text. Caps the text at 0x80 chars and trims up to two trailing control chars.
// When toChat is set it prefixes the local player's name ("<name>: <text>") via
// sprintf; otherwise the raw text is used. When showWnd is set the line is either
// posted to a Win32 chat control (ShowChatLine) or appended to the in-game chat
// log (m_4->m_5c->AddItem). Finally the line is stamped into the static chat
// packet and shipped through SetGroupDataFrom.
// @early-stop
// scheduling wall (~75%): the full logic is reproduced - the null/empty guard,
// the 0x80 cap, the 2-iteration trailing-control-char trim, the toChat "name: msg"
// sprintf (frameless GetName temp), the showWnd ShowChatLine/AddItem branch, and
// the static-packet (per-field globals) build + SetGroupDataFrom send - and the
// /GX frame is correctly elided (line modeled as a stack char buffer, not a
// CString). The residual is instruction-selection that desyncs the tail: retail
// hoists the 0x20 trim constant + `cmpb %al,mem` (vs my `movb mem,%dl;cmpb`), and
// folds the send-size strlen differently (`dec;add 0xd` vs `add 0xc`), reordering
// the static stores. Big function, scheduling-class; deferred to the final sweep.
RVA(0x000bb190, 0x1c5)
i32 CNetMgr::BroadcastChatLine(char* text, i32 toChat, i32 showWnd, void* hWnd) {
    if (text == 0) {
        return 0;
    }
    if (text[0] == 0) {
        return 0;
    }

    i32 len = strlen(text);
    if (len > 0x80) {
        text[0x80] = 0;
        len = 0x80;
    }
    if (len > 0 && text[len - 1] < 0x20) {
        text[len - 1] = 0;
        len--;
        if (len > 0 && text[len - 1] < 0x20) {
            text[len - 1] = 0;
        }
    }

    char line[0x12c];
    if (toChat != 0) {
        CNetPlayerName* player = (CNetPlayerName*)((CNetGameMgr*)m_4)
                                     ->FindPlayer(((CNetPlayerEntry*)m_localPlayer)->m_4);
        CString name = player->GetName();
        sprintf(line, "%s: %s", (const char*)name, text);
    } else {
        strcpy(line, text);
    }

    if (showWnd != 0) {
        if (hWnd != 0) {
            ShowChatLine(line, hWnd);
        } else {
            CNetPlayerName* player = (CNetPlayerName*)((CNetGameMgr*)m_4)->FindPlayer(m_5c0);
            if (player != 0) {
                ((CNetGameMgr*)m_4)->m_5c->AddItem(line, 0x30, player->m_8);
            }
        }
    }

    g_chatPacket_id = 0x3f0;
    g_chatPacket_val = 0;
    strcpy(&g_chatPacket_buf, line);
    g_chatPacket_flag |= 0x80;
    m_peer->SetGroupDataFrom(
        (CNetPlayerEntry*)m_localPlayer,
        1,
        (i32)&g_chatPacket_flag,
        strlen(line) + 0xd
    );
    return 1;
}

// ---------------------------------------------------------------------------
// CNetMgr::DropChannelPlayer  (__thiscall).
// Drops the player owning channel[idx]. Bails (0) on an out-of-range index
// ([0,4)), no peer (m_528), or a null channel. Looks the channel player's data
// up in the peer (GetPlayerData by the channel's +0x18 id); when the channel's
// +0x14 "active" gate is set it reports the player-left to the rest (stat 0x3fb
// via SendStatPair when the data is present, else just clears below). Removes the
// channel (RemoveChannel(idx)) and, on success, fires the rejoin finalizer
// (RejoinIfNeeded(0)) and latches the player-left flag. Returns 1 when a channel
// was dropped, else 0.
// @early-stop
// regalloc wall (~98%): the whole body is byte-aligned (index guard, m_528 gate,
// channel-record lea, GetPlayerData probe, m_14-gated SendStatTo, RemoveChannel +
// RejoinIfNeeded + g_playerLeftFlag tail) but retail pins the "active" flag
// (ch->m_14) in edi (callee-saved across the calls) where cl keeps it in ecx, and
// shares the failure epilogue one instruction tighter. Final sweep.
RVA(0x000bb510, 0x9d)
i32 CNetMgr::DropChannelPlayer(i32 idx) {
    if (idx < 0 || idx >= 4) {
        return 0;
    }
    if (m_useChannelLatency == 0) {
        return 0;
    }

    CNetChannel* ch = (CNetChannel*)((char*)m_4 + idx * 0x238 + 0x150);
    if (ch == 0) {
        return 0;
    }

    void* data = m_peer->GetPlayerData(ch->m_18);
    i32 active = ch->m_14;
    if (data == 0) {
        if (active != 0) {
            return 0;
        }
    } else if (active != 0) {
        SendStatTo((CNetPlayerEntry*)data, 0x3fb, 1);
    }

    if (RemoveChannel(idx) == 0) {
        return 0;
    }
    RejoinIfNeeded(0);
    g_playerLeftFlag = 1;
    return 1;
}

// ---------------------------------------------------------------------------
// CNetMgr::RecordDropPlayer2  (__thiscall).
// Records a pending player-drop into the m_608 id array. No-op once the host
// latch m_534 is set, or for the local player (m_5c0). Skips a player already
// recorded; otherwise fills the first empty m_608 slot, bailing if the array is
// full. Then, once the number of recorded drops reaches the number of
// command slots in state 3 (the m_520 sub-object, stride 0x64), it announces the
// drop twice (stat 0x3e8) and latches m_534.
// @early-stop
// regalloc wall (~93%): every instruction matches in the multiset (the m_534/m_5c0
// guards, the three m_608 scans, the state-3 slot count, the double SendStatFlag and
// the m_534 latch) but retail pins this->esi / id->edi where cl assigns this->edi /
// id->esi; the register choice is not steerable from source. Final sweep.
RVA(0x000bb5e0, 0xd9)
void CNetMgr::RecordDropPlayer2(i32 a, i32 id) {
    if (m_534 != 0) {
        return;
    }
    if (id == m_5c0) {
        return;
    }

    i32 count = m_60c;
    i32 i;
    for (i = 0; i < count; i++) {
        if (m_608[i] == id) {
            return;
        }
    }

    i32 slot = 0;
    while (slot < count) {
        if (m_608[slot] == 0) {
            break;
        }
        slot++;
    }
    if (slot >= count) {
        return;
    }
    m_608[slot] = id;

    i32 stateThree = 0;
    i32* p = (i32*)((char*)m_session + 0x20);
    for (i = 0; i < 4; i++) {
        if (p != 0 && *p == 3) {
            stateThree++;
        }
        p = (i32*)((char*)p + 0x64);
    }

    i32 recorded = 0;
    for (i = 0; i < count; i++) {
        if (m_608[i] != 0) {
            recorded++;
        }
    }
    if (recorded < stateThree) {
        return;
    }

    SendStatFlag(0x3e8, 1);
    SendStatFlag(0x3e8, 1);
    m_534 = 1;
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
// CNetMgr::AutoTuneCmdDelay  (__thiscall).
// Derives the command-timing config (m_cmdDelay/m_resend) from the measured peer
// ping. No-op (returns) once the config-loaded gate m_530 is set. Samples the
// ping (MeasurePing), divides it by 9 (the 0x88888889 reciprocal-multiply) and
// adds 2 -> a base command delay clamped to a minimum of 3; bumps it by 1 plus a
// further 1 when a secondary probe (ProbeLatency(0)) exceeds 2, stores it as
// m_cmdDelay, and picks a resend window (10 for <=5, else 30 or 20 for >8) before
// persisting the pair (ApplyCmdDelayDefaults via the 0-arg overload).
// @early-stop
// regalloc + integer-division-idiom wall (~68%): the logic is reproduced (the m_530
// gate, the /9 + 2 clamp(min 3), the ProbeLatency>2 bump, the m_5a4 store, the
// <=5/>8 resend selection, the two WriteCmdDelay persists) but retail keeps this in
// edi, folds the divide with a different /9 magic (0x38e38e39;shr1 vs cl's
// 0x88888889;shr4) and folds the (>2?1:0)+1 bump into a single lea - none steerable
// from C source. Final sweep (an optimizer-math idiom).
RVA(0x000bcc10, 0x8e)
void CNetMgr::AutoTuneCmdDelay() {
    if (m_530 != 0) {
        return;
    }

    u32 ping = (u32)MeasurePing();
    i32 base = (i32)(ping / 9) + 2;
    if (base < 3) {
        base = 3;
    }

    i32 probe = ProbeLatency(0);
    base += (probe > 2 ? 1 : 0) + 1;
    m_cmdDelay = base;
    if (base <= 5) {
        m_resend = 0xa;
        WriteCmdDelay(0);
        return;
    }
    m_resend = (base > 8 ? 0x14 : 0x1e);
    WriteCmdDelay(0);
}

// ---------------------------------------------------------------------------
// CNetMgr::LoadConfig  (__thiscall).
// Copies the command-timing config out of a caller config blob into the manager:
// a config word (cfg+0x8 -> m_5b0), two name CStrings (cfg+0xc -> m_5b4,
// cfg+0x8c -> m_5b8) and four dwords (cfg+0x10c -> m_cmdDelay, +0x110 ->
// m_resend, +0x114 -> m_600, +0x118 -> m_2d8). No-op (0) on a null blob; else 1.
RVA(0x000bce80, 0x77)
i32 CNetMgr::LoadConfig(void* cfg) {
    if (cfg == 0) {
        return 0;
    }

    char* c = (char*)cfg;
    m_5b0 = *(i32*)(c + 8);
    m_5b4 = (const char*)(c + 0xc);
    m_5b8 = (const char*)(c + 0x8c);
    m_cmdDelay = *(i32*)(c + 0x10c);
    m_resend = *(i32*)(c + 0x110);
    m_600 = *(i32*)(c + 0x114);
    m_2d8 = *(i32*)(c + 0x118);
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
    i32 hr = iface->vtbl->Open(iface, 0, &out, 0);
    if (hr != 0) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x78, hr, 0);
        Destroy();
        return 0;
    }
    hr = iface->vtbl->QueryInterface(iface, g_netDirectPlayRiid, &m_directPlay);
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
// CNetMgr::AddPlayerNode  (__thiscall).
// Adds one enumerated player to the +0x38 player list. No-op (0) on a null
// descriptor. RezAlloc's a 0x58-byte node (vptr 0x5f0760, body zeroed),
// inits it from the descriptor (copies the 0x50-byte player desc in and trims
// its name); if the init fails it self-destructs the node and returns 0,
// otherwise AddTail's it onto the +0x38 CObList and caches the position at +0x54.
// @early-stop
// regalloc wall (~92%): the RezAlloc + vptr-stamp + zero-loop, the Init call with
// the self-destruct-on-fail, and the AddTail-into-+0x38 are all byte-aligned, but
// retail keeps playerDesc->ebx / this->ebp where cl swaps them (ebp/ebx), and the
// vptr store / lea schedule one pair differently. Not steerable. Final sweep.
RVA(0x001786d0, 0x77)
i32 CNetMgr::AddPlayerNode(void* playerDesc) {
    if (playerDesc == 0) {
        return 0;
    }

    CNetPlayerListNode* node = (CNetPlayerListNode*)RezAlloc(0x58);
    if (node != 0) {
        *(void**)node = &g_netPlayerNodeVtbl;
        i32* body = (i32*)((char*)node + 4);
        for (i32 i = 0; i < 0x14; i++) {
            body[i] = 0;
        }
        node->m_54 = 0;
    } else {
        node = 0;
    }

    if (node->Init(playerDesc) == 0) {
        if (node != 0) {
            node->SelfDestruct(1);
        }
        return 0;
    }

    node->m_54 = (__POSITION*)((CObList*)((char*)this + 0x38))->AddTail((CObject*)node);
    return (i32)node;
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

    CNetListNode* node = m_3c;
    m_playerSelId = (i32)node;
    CNetPlayerName* payload;
    if (node != 0) {
        m_playerSelId = (i32)node->m_next;
        payload = (CNetPlayerName*)node->m_data;
    } else {
        payload = 0;
    }

    while (payload != 0) {
        i32 r = (i32)
            SendMessageA((HWND)hList, LB_ADDSTRING, 0, (LPARAM) * (i32*)((char*)payload + 0x34));
        if (r != -1) {
            SendMessageA((HWND)hList, LB_SETITEMDATA, r, (LPARAM)payload);
        }
        CNetListNode* cur = (CNetListNode*)m_playerSelId;
        if (cur != 0) {
            payload = (CNetPlayerName*)cur->m_data;
            m_playerSelId = (i32)cur->m_next;
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
    i32 hr = iface->vtbl->EnumGroups(iface, (char*)a + 4, 1);
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
    i32 hr = iface->vtbl->EnumGroupsCb(iface, 0, (void*)&NetEnumCb, this, 0);
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
    i32 hr = iface->vtbl->EnumGroupsCb(iface, desc, (void*)&NetEnumCb, this, flags);
    if (hr != 0) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x327, hr, 0);
        return hr;
    }
    return 0;
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
// /GX EH-temp + scheduling wall (~56%): the RezAlloc'd node, the two-vptr stamp
// around the scoped CString temps, the InitSession call, the GetData5(slot 0x74)
// probe + ReportError, and the AddTail/self-destruct tail are all reconstructed,
// but retail interleaves the two CString stack-temp ctors/dtors with the node-member
// stores and the EH-state cookie sequence in a way a clean C++ scoped temp won't
// express. Largest residual of the cluster; final-sweep redo with the node class
// fully modeled. See docs/patterns/gx-scoped-local-eh-frame-size.md.
RVA(0x00178b30, 0x140)
i32 CNetMgr::AddSessionNode(void* a, void* b) {
    CNetSessionNode* node = (CNetSessionNode*)RezAlloc(0x24);
    if (node != 0) {
        *(void**)node = &g_netSessionNodeDtorVtbl;
        {
            CString temp1;
            CString temp2;
            *(void**)node = &g_netSessionNodeVtbl;
            node->m_4 = 0;
            node->m_20 = 0;
            node->m_18 = 0;
            node->m_14 = 0;
        }
    } else {
        node = 0;
    }

    if (node->InitSession((i32)a, (const char*)b, (const char*)b, (i32)b) != 0) {
        IDirectPlay4Z* iface = m_directPlay;
        i32 hr = iface->vtbl->GetData5(iface, ((CNetSessionNode*)a)->m_4, node, 4, 1);
        if (hr != 0) {
            ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x36c, hr, 0);
        }
    }

    if (node != 0) {
        __POSITION* pos = (__POSITION*)((CObList*)((char*)this + 0x54))->AddTail((CObject*)node);
        if (pos == 0) {
            if (node != 0) {
                node->SelfDestruct(1);
            }
        } else {
            node->m_20 = (i32)pos;
        }
    }
    return (i32)node;
}

// ---------------------------------------------------------------------------
// CNetMgr::CreatePlayer  (__thiscall; ret 0xc, 3 args).
// Reads a session's player-data blob via the DirectPlay interface (slot 6) into a
// 0x10-byte descriptor (size 0x10, the two trailing args at +0x8/+0xc) plus a
// scalar output, passing the third arg through. On a nonzero HRESULT reports it
// (NetMgr.cpp line 0x3bb) and returns 0; on success hands the output plus the two
// trailing args to AddSessionNode.
// @early-stop
// stack-buffer-size-drives-frame wall (~71%): the GetSessionDesc(slot 6) probe into
// the 0x10 descriptor + scalar out-var, the ReportError failure path and the
// AddSessionNode tail are reproduced, but retail's frame is 0x14 (cl 0x10) and it
// packs the desc fields / out-var into different esp offsets, re-permuting the
// field stores + arg regs. See docs/patterns/stack-buffer-size-drives-frame.md.
// Final sweep.
RVA(0x00178cb0, 0x8b)
i32 CNetMgr::CreatePlayer(void* a, i32 b, i32 c) {
    i32 out = 0;
    i32 desc[4];
    desc[0] = 0x10;
    desc[1] = 0;
    desc[2] = b;
    desc[3] = c;

    IDirectPlay4Z* iface = m_directPlay;
    i32 hr = iface->vtbl->GetSessionDesc(iface, &desc[0], &out, (i32)a, 0, 0);
    if (hr != 0) {
        ReportError("C:\\Proj\\NetMgr\\NetMgr.cpp", 0x3bb, hr, 0);
        return 0;
    }
    return AddSessionNode((void*)out, (void*)b);
}

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

    CNetListNode* node = (CNetListNode*)m_58;
    m_sessionSelId = (i32)node;
    CNetSessionNode* payload;
    if (node != 0) {
        m_sessionSelId = (i32)node->m_next;
        payload = (CNetSessionNode*)node->m_data;
    } else {
        payload = 0;
    }

    while (payload != 0) {
        CString name = payload->GetName();
        i32 r = (i32)SendMessageA((HWND)hList, LB_ADDSTRING, 0, (LPARAM)(const char*)name);
        if (r != -1) {
            SendMessageA((HWND)hList, LB_SETITEMDATA, r, (LPARAM)payload);
        }
        CNetListNode* cur = (CNetListNode*)m_sessionSelId;
        if (cur != 0) {
            payload = (CNetSessionNode*)cur->m_data;
            m_sessionSelId = (i32)cur->m_next;
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

// ---------------------------------------------------------------------------
// CNetMgr::Find (0x179270, __thiscall, ret 4) - the group-list lookup.
// The +0x1c group CObList holds InterfaceObject payload nodes (CNode shape:
// next@+0, data@+8). For each payload, the service-provider class `kind` selects
// one of the GUID predicates (Font.cpp InterfaceObject::IsInterfaceX, reloc-
// masked): kind 1 -> IsInterface2, kind 2 -> IsInterface1, kind 5 -> IsInterface5;
// return the first payload that matches. The running POSITION is cached at +0x7c
// (the m_groupSelId slot, reused as the GetNext cursor). Raw +0x20/+0x7c offset
// access keeps the codegen independent of the dual-purpose field names.
// ---------------------------------------------------------------------------
struct InterfaceObject {
    i32 IsInterface1(); // 0x1794b0
    i32 IsInterface2(); // 0x1794e0
    i32 IsInterface5(); // 0x179570
};
struct CGroupNode {
    CGroupNode* m_next;      // +0x00
    void* m_4;               // +0x04
    InterfaceObject* m_data; // +0x08
};

// @early-stop
// linked-list advance regalloc wall (~94.9%): the head-load + the kind switch +
// the three predicate calls are byte-exact; only the GetNext advance differs -
// retail conservatively reloads the +0x7c cursor into a 2nd register (edx) for
// the ->next read while routing ->data through eax, the recompile derefs both
// from one register. Aliasing-conservatism choice, not source-steerable. See
// docs/patterns/linked-list-walk-node-eax-rotation.md. Logic complete.
RVA(0x00179270, 0x89)
InterfaceObject* CNetMgr::Find(i32 kind) {
    CGroupNode* node = *(CGroupNode**)((char*)this + 0x20);
    *(CGroupNode**)((char*)this + 0x7c) = node;
    InterfaceObject* item;
    if (node) {
        *(CGroupNode**)((char*)this + 0x7c) = node->m_next;
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
        CGroupNode* cur = *(CGroupNode**)((char*)this + 0x7c);
        if (cur) {
            item = cur->m_data;
            *(CGroupNode**)((char*)this + 0x7c) = cur->m_next;
        } else {
            item = 0;
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------
// CNetSessionNode::InitSession (0x1796c0, __thiscall) - the 4-arg body the
// session-node ctor (AddSessionNode) runs on the fresh 0x24-byte node: store the
// dword id (+0x4) and the second dword (+0x10), assign the two name CStrings
// (+0x8/+0xc), zero the +0x14/+0x18/+0x1c scratch, return TRUE.
// ---------------------------------------------------------------------------
RVA(0x001796c0, 0x3f)
i32 CNetSessionNode::InitSession(i32 id, const char* a, const char* b, i32 d) {
    m_4 = id;
    m_8 = a;
    m_c = b;
    m_10 = d;
    m_18 = 0;
    m_1c = 0;
    m_14 = 0;
    return 1;
}

// The game-settings singleton (_g_mgrSettings @0x64556c) - only the level-name
// rez-path builder and the modal reporter are reached here. External; the
// `call rel32` reloc-masks.
struct CGameSettings {
    void* BuildRezPath(i32 a, void* name, i32 c, i32 d, CString cap); // 0x93d40
    void ShowModal(const char* msg);                                  // 0x8ef10
};
extern "C" CGameSettings* g_mgrSettings; // 0x64556c

// The active net session the verify path polls (DAT_00648cf8, a CNetMgr*).
extern "C" CNetMgr* g_648cf8; // 0x648cf8

// The shared empty-string literal CreateLocalPlayer hands to the peer's player
// factory (0x6293f4; DIR32 reloc-masked).
extern "C" char g_emptyString[]; // 0x6293f4

// The 0x28-byte "player joined" announce packet CreateLocalPlayer builds and ships
// as stat 0x3f9: a flag byte, the stat id, a small fixed config block, the local
// player id, then the 0x14-byte name buffer.
struct CNetJoinPacket {
    u8 m_0; // +0x00  flag byte (bit7)
    char m_pad1[3];
    i32 m_4;        // +0x04  stat id (0x3f9)
    u8 m_8;         // +0x08
    u8 m_9;         // +0x09
    u8 m_a;         // +0x0a
    u8 m_b;         // +0x0b
    u8 m_c;         // +0x0c
    u8 m_d;         // +0x0d
    u8 m_e;         // +0x0e
    char m_padf;    // +0x0f
    i32 m_10;       // +0x10  local player id (m_5c0)
    char m_14[0x14]; // +0x14  player name (strcpy)
};

// The 0x11c-byte command-timing config blob SaveConfig builds and ships as stat
// 0x416 (the inverse of LoadConfig): a flag byte, the stat id, the config word,
// the two config-name strings (wsprintf'd in), then the four timing dwords.
struct CNetConfigBlob {
    u8 m_0; // +0x000  flag byte (bit7)
    char m_pad1[3];
    i32 m_4;            // +0x004  stat id (0x416)
    i32 m_8;            // +0x008  m_5b0
    char m_nameA[0x80]; // +0x00c  config name A
    char m_nameB[0x80]; // +0x08c  config name B
    i32 m_10c;          // +0x10c  m_cmdDelay
    i32 m_110;          // +0x110  m_resend
    i32 m_114;          // +0x114  m_600
    i32 m_118;          // +0x118  m_2d8
};

// ---------------------------------------------------------------------------
// CNetMgr::SaveConfig  (__thiscall; ret 4; /GX EH frame).
// Serializes the command-timing config into a 0x11c-byte stat-0x416 blob and
// ships it: the config word (m_5b0), the two config names formatted in with
// wsprintfA, and the four timing dwords (m_cmdDelay/m_resend/m_600/m_2d8). When a
// recipient is given it goes point-to-point (SendStatPairRaw), else it broadcasts
// (SendStatFrom). The two config-name CString temps run under the /GX frame.
// @early-stop
// reloc-masked plateau (96.4%): the instruction stream is byte-faithful (the
// memset, the |0x80 flag, every blob field store, both GetConfigName + cached
// wsprintfA-through-IAT formats, the four timing dwords, the recipient-vs-broadcast
// send). The residual is non-steerable: the /GX unwind cookie immediate (push 0xb
// vs 0x0), the wsprintfA IAT pointer symbol (__imp vs raw 0x6c44c0; reloc-masked),
// the CString-buffer read kept in the return reg vs re-read from the temp slot, and
// a tail `mov eax,1` materialization. Final sweep.
RVA(0x000bccd0, 0x141)
i32 CNetMgr::SaveConfig(CNetPlayerEntry* recipient) {
    CNetConfigBlob blob;
    memset(&blob, 0, sizeof(blob));
    blob.m_0 |= 0x80;
    blob.m_4 = 0x416;
    blob.m_8 = m_5b0;
    {
        CString a = GetConfigNameA();
        wsprintfA(blob.m_nameA, (const char*)a);
    }
    {
        CString b = GetConfigNameB();
        wsprintfA(blob.m_nameB, (const char*)b);
    }
    blob.m_10c = m_cmdDelay;
    blob.m_110 = m_resend;
    blob.m_114 = m_600;
    blob.m_118 = m_2d8;

    if (recipient != 0) {
        return SendStatPairRaw(recipient, &blob, 0x11c, 1);
    }
    return SendStatFrom((CNetStatPacket*)&blob, 0x11c, 1);
}

// ---------------------------------------------------------------------------
// CNetMgr::CreateLocalPlayer  (__thiscall; /GX EH frame).
// Registers the local player with the peer manager under the local name, latches
// its DirectPlay id (m_5c0), blocks until the host admits it, and announces the
// join. Bails (reports + 0) when the peer rejects the player or the connect wait
// times out. The two name CString temps run under the /GX frame; the join packet's
// name field is filled with an inline strcpy.
// @early-stop
// reloc-masked + scheduling plateau (94.5%): the instruction stream is byte-faithful
// (GetString5a0 + CreatePlayer, the id latch, WaitForConnect, the full join-packet
// build, the inline strlen/rep-movs strcpy, SendStatFrom). The residual is non-
// steerable: the /GX unwind-cookie immediate (push 0x8 vs 0x0), a CString-buffer
// read kept in the return reg vs re-read from the temp slot, and the order MSVC
// schedules the adjacent packet byte-stores (0x63/0xf and the m_5c0 load). Final sweep.
RVA(0x000bc750, 0x151)
i32 CNetMgr::CreateLocalPlayer() {
    {
        CString name = GetString5a0();
        m_localPlayer = m_peer->CreatePlayer((void*)(const char*)name, (i32)g_emptyString, 0);
    }
    if (m_localPlayer == 0) {
        ReportConnectFailed(0);
        return 0;
    }

    m_5c0 = ((CNetPlayerEntry*)m_localPlayer)->m_4;
    if (WaitForConnect() == 0) {
        return 0;
    }

    CNetJoinPacket pkt;
    memset(&pkt, 0, 0x28);
    pkt.m_0 = 0x80;
    pkt.m_4 = 0x3f9;
    pkt.m_8 = 1;
    pkt.m_9 = 0;
    pkt.m_a = 1;
    pkt.m_b = 0;
    pkt.m_c = 0x63;
    pkt.m_d = 0xf;
    pkt.m_e = 0;
    pkt.m_10 = m_5c0;
    {
        CString name = GetString5a0();
        strcpy(pkt.m_14, (const char*)name);
    }
    SendStatFrom((CNetStatPacket*)&pkt, 0x28, 1);
    return 1;
}

// ---------------------------------------------------------------------------
// CNetMgr::VerifyCustomLevel  (__thiscall; /GX EH frame).
// Confirms every player is on the same custom level before the match starts.
// No-op (0) on a null token pair or when the config isn't loaded (m_530 == 0; that
// path just pumps the receive queue). Builds the level rez path from the active
// config name (GetConfigNameB when a custom id m_5b0 is set, else GetConfigNameA),
// hands it to the active session's Poll, and reports: Poll failure -> re-disable +
// "unable to verify"; verified-but-mismatch (m_53c still 0) -> "not all players
// have the same level"; agreement -> 1. The by-value CString rez-path arg + the
// name temp run under the /GX frame.
// @early-stop
// /GX CString-by-value EH-frame-layout wall (7%): the instruction SEQUENCE is
// faithful (the arg1/arg2/m_530 guards, the GetConfigNameA/B selection, the
// BuildRezPath by-value CString copy-ctor, the multi-temp destruct bitmask, the
// g_648cf8 Poll dispatch and both ShowModal reports), but retail reserves two
// dedicated EH-state dwords (`sub esp,8`) and overlaps the CString temps onto the
// now-dead arg slots, while cl folds the EH state into the arg-overlap area and
// omits the sub - an 8-byte frame-size delta that cascades through every
// stack-relative offset. Same CString-EH residue family as the dialog sibling
// CNetGameDlg::VerifyCustomLevel (0xc4c00, parked ~55%); not source-steerable.
// See docs/patterns/gx-scoped-local-eh-frame-size.md. Final sweep.
RVA(0x000b8fc0, 0x151)
i32 CNetMgr::VerifyCustomLevel(i32 a1, i32 a2) {
    if (a1 == 0) {
        return 0;
    }
    if (a2 == 0) {
        return 0;
    }
    if (m_530 == 0) {
        PollSession();
        return 0;
    }

    void* token;
    if (m_5b0 != 0) {
        CString b = GetConfigNameB();
        token = g_mgrSettings->BuildRezPath(0, (void*)m_5b0, 0, 0, b);
    } else {
        CString a = GetConfigNameA();
        token = g_mgrSettings->BuildRezPath(0, (void*)m_5b0, 0, 0, a);
    }

    g_648cf8->m_53c = 0;
    if (g_648cf8->Poll((i32)token) == 0) {
        m_530 = 0;
        g_mgrSettings->ShowModal("Unable to verify custom level with other players");
        return 0;
    }
    if (g_648cf8->m_53c == 0) {
        g_mgrSettings->ShowModal("Not all players have the (same) custom level.");
        m_530 = 0;
        return 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CNetMgr::CreateSession  (__thiscall; /GX EH frame).
// Stands up the DirectPlay command session: enumerate the host's group (via the
// peer's EnumGroupsRange over the create-context record), resolve the local
// player, then allocate + construct the 0x20bb0-byte CNetSession (operator new +
// the 4-slot vector-ctor + ResetAll), wire it (Init with the game sub-object,
// this manager and the peer), latch the local player and derive the resync tick
// byte (m_5cc), and finally seed one command slot per channel with a per-channel
// owner code (1 inactive, 2 local, 3 remote). The new'd session is the /GX-tracked
// object. Returns 1 once every slot is created, 0 on any failure.
// @early-stop
// reloc-masked + EH-cookie plateau (95.4%): the instruction stream is byte-identical
// to retail (RezAlloc alloc, the 4-slot vector-ctor + ResetAll, Init, the tick-byte
// derivation, the per-channel CreateSlot loop). The residual is non-steerable: the
// /GX unwind-table cookie immediate (push 0x16 vs 0x0; gx-scoped-local-eh-frame-
// size.md), the MSVC-internal vector-ctor/ctor/dtor helper symbols (delinker named
// them Boundary_/CGruntWingzTimeSprite - unalignable reloc names), and a 4-insn
// register shuffle around the m_session->m_c store. Final sweep.
RVA(0x000bbc90, 0x1b8)
i32 CNetMgr::CreateSession() {
    void* rec = g_648cf4->m_74;
    if (rec == 0) {
        return 0;
    }
    m_peer->EnumGroupsRange(rec, 0);
    if (ResolveLocalPlayer() == 0) {
        return 0;
    }

    CNetSession* session = new CNetSession();
    m_session = session;
    if (session == 0) {
        return 0;
    }
    if (session->Init(m_4, this, m_peer) == 0) {
        return 0;
    }

    m_session->m_c = m_localPlayer;
    i32 raw10 = m_session->m_10;
    u8 b = (u8)raw10;
    if (b == 0) {
        b = 0x7f;
    } else {
        b = b - 1;
    }
    m_5cc = b;

    for (i32 i = 0; i < 4; i++) {
        CNetChannel* ch = (CNetChannel*)((char*)m_4 + 0x150 + i * 0x238);
        i32 code = 1;
        if (ch->m_20 != 0 && ch->m_14 != 0) {
            code = (ch->m_18 == m_5c0) ? 2 : 3;
        }
        if (m_session->CreateSlot(i, code) == 0) {
            return 0;
        }
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CNetCmdSlot::CNetCmdSlot  (__thiscall; /GX EH frame).
// Constructs the queued-command list (CObList m_cmds, default nBlockSize 10),
// then resets the slot to its empty state: zero the scalar header, drain the
// queue (ClearCmds), zero the command fields and splat both command ranges.
// The CObList member's dtor pulls in the /GX EH frame.
// @early-stop
// zero-register-pinning wall (78.8%): code bytes byte-faithful (EH frame, CObList
// ctor, every field store, ClearCmds + both ResetTriple calls all match retail).
// Retail re-materializes the splat 0 in eax (caller-saved, `xor eax,eax` after the
// CObList ctor AND after ClearCmds) and pushes only esi; cl pins 0 in callee-saved
// edi (one xor, an extra push/pop edi). Identical coin-flip to the sibling
// CNetCmdSlot::ResetAll/Init in netcmdslot; not source-steerable. See
// docs/patterns/zero-register-pinning.md. Final sweep.
RVA(0x000bbec0, 0x81)
CNetCmdSlot::CNetCmdSlot() {
    m_0 = 0;
    m_4 = 0;
    m_8 = 0;
    m_c = 0;
    m_10 = 0;
    m_14 = 0;
    m_18 = 0;
    m_1c = 0;
    ClearCmds();
    m_3c = 0;
    m_40 = 0;
    m_44 = 0;
    m_48 = 0;
    ResetTriple(m_4c);
    ResetTriple(m_58);
}
