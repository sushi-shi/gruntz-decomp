#include <rva.h>

#include <Gruntz/Grunt.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntPickupInline.h>
#include <Gruntz/GruntzCmdMgr.h>
#include <Gruntz/GruntzCommand.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/Multi.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/ScanGridMacros.h>
#include <Gruntz/TriggerMgr.h>
#include <Ints.h>
#include <Net/CmdPool.h>
#include <Net/NetCmdSlot.h>
#include <Net/NetMgr.h>
#include <Net/NetSlotState.h>
#include <Pix16.h>
#include <Rez/RezMgr.h>

#include <dplay.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

RVA(0x000bef80, 0x51)
i32 CNetSession::Initialize(CGruntzMgr* mgr, CMulti* owner, CNetMgr* netMgr) {
    if (mgr == NULL) {
        return 0;
    }
    if (owner == NULL) {
        return 0;
    }
    if (netMgr == NULL) {
        return 0;
    }
    m_mgr = mgr;
    m_owner = owner;
    m_netMgr = netMgr;
    ResetRound();
    m_commandPeriod = owner->m_commandDelay;
    return 1;
}

// @early-stop
RVA(0x000bf000, 0xd5)
void CNetSession::Shutdown() {
    m_mgr = NULL;
    m_owner = NULL;
    m_netMgr = NULL;
    m_localPlayer = NULL;
    m_commandTick = 0;
    m_batchBuilt = false;
    m_sequence = 0;
    m_commandPeriod = 1;
    for (i32 i = 0; i < 4; i++) {
        m_slots[i].m_isDraining = false;
        m_slots[i].m_drainSequence = 0;
        m_slots[i].m_state = NETSLOT_EMPTY;
        m_slots[i].m_player = NULL;
        m_slots[i].m_latency = 0;
        m_slots[i].m_contiguousSequence = 0;
        m_slots[i].m_peerWindowBase = 0;
        m_slots[i].m_owner = NULL;
        m_slots[i].ClearRecords();
        m_slots[i].ClearDrainAcks();
        m_slots[i].ClearSequenceSet(m_slots[i].m_receivedAhead);
        m_slots[i].ClearSequenceSet(m_slots[i].m_peerReceivedAhead);
    }
    for (i32 j = 0; j < 0x80; j++) {
        m_commandByTick[j] = NULL;
    }
    for (i32 k = 0; k < 0x80; k++) {
        m_commandRecords[k].m_sequence = 0;
        m_commandRecords[k].m_entryCount = 0;
        m_commandRecords[k].m_payloadLength = 0;
        m_commandRecords[k].m_checksum = 0;
    }
    CPtrList& freeList = CPtrListPool<GruntRec>::s_freeList;
    while (freeList.GetCount() != 0) {
        GruntRec* p = static_cast<GruntRec*>(freeList.RemoveTail());
        if (p) {
            delete p;
        }
    }
}

RVA(0x000bf120, 0x11)
void CNetCmdSlot::ClearDrainAcks() {
    for (i32 i = 0; i < NET_SLOT_COUNT; i++) {
        m_drainAckFlags[i] = 0;
    }
}

RVA(0x000bf150, 0x58)
void CNetSession::ResetRound() {
    m_commandTick = 0;
    m_batchBuilt = false;
    m_sequence = 0;
    i32 i;
    for (i = 0; i < 4; i++) {
        m_slots[i].ClearSyncState();
    }

    for (i = 0; i < 0x80; i++) {
        m_commandByTick[i] = NULL;
    }
    for (i = 0; i < 0x80; i++) {
        m_commandRecords[i].m_sequence = 0;
        m_commandRecords[i].m_entryCount = 0;
        m_commandRecords[i].m_payloadLength = 0;
        m_commandRecords[i].m_checksum = 0;
    }
}

// @early-stop
RVA(0x000bf1d0, 0x2a4)
void CNetSession::BuildGruntzCrcInfo() {
    char szLine[0x100] = "";

    CString info("crc info for all gruntz:\n------------------------\n");

    for (i32 player = 0; player < 4; player++) {
        for (i32 g = 0; g < 0xf; g++) {

            CGrunt* grunt = m_owner->Mgr()->m_triggerMgr->m_units[player * 0xf + g];
            if (grunt == NULL) {
                continue;
            }
            i32 rnd = rand();
            PickupType type = grunt->m_entranceReason;
            i32 wp;
            PRIO(wp, type);
            b32 da = grunt->m_daFlag;
            PickupType toy = grunt->m_vehiclePickupType;
            PickupType tool = ArrivalPickupOf(grunt, type);
            wsprintfA(
                szLine,
                "[p=%d][g=%d][health=%d][x=%d][y=%d][dir=%d][stm=%d][ttl=%d][tool=%d]"
                "[toy=%d][da=%d][wp=%d][iic=%d][qat=%d][qax=%d][ia=%d][iad=%d][rnd=%d]\n",
                player,
                g,
                grunt->m_health,
                grunt->m_object->m_screenX,
                grunt->m_object->m_screenY,
                grunt->m_entranceCell.direction,
                grunt->m_stamina,
                grunt->m_toyTime,
                tool,
                toy,
                da,
                wp,
                grunt->m_poweredUp,
                grunt->m_neighborValid,
                grunt->m_arrivalPhase,
                grunt->m_combatActive,
                grunt->m_neighborScanEnabled,
                rnd
            );
            info += DATA_COMPGEN(0x002122ac, "\n");
            info += szLine;
        }
    }
    m_owner->ReportVersionMsg(const_cast<char*>(static_cast<const char*>(info)), 0);
}

RVA(0x000bf530, 0x3b)
GruntRec* AllocateGruntRecord(i32 clearRecord) {
    CPtrList& freeList = CPtrListPool<GruntRec>::s_freeList;
    if (freeList.GetCount()) {
        GruntRec* record = static_cast<GruntRec*>(freeList.RemoveTail());
        if (clearRecord) {
            memset(record, 0, sizeof(GruntRec));
        }
        return record;
    }
    return new GruntRec;
}

RVA(0x000bf580, 0x10)
void RecycleGruntRecord(GruntRec* record) {
    CPtrListPool<GruntRec>::s_freeList.AddTail(record);
}

RVA(0x000bf5a0, 0x110)
i32 CNetSession::Poll(i32 elapsedMs) {
    for (i32 i = 0; i < 4; i++) {
        if (m_slots[i].m_state == NETSLOT_ACTIVE) {
            m_slots[i].m_latency += elapsedMs;
        }
    }

    i32 status = 0;
    i32 availableCount;
    CNetPlayerNode* localPlayer = m_localPlayer;
    CNetMgr* netMgr = m_netMgr;
    if (localPlayer == NULL) {
        availableCount = 0;
    } else {
        DWORD messageCount;

        IDirectPlay4A* directPlay = netMgr->m_directPlay;
        i32 result = directPlay->GetMessageCount(localPlayer->m_playerId, &messageCount);
        availableCount = (result == 0) ? messageCount : 0;
    }

    DPID senderId = 0;
    DWORD messageSize = sizeof(g_lobbyRecvBuf);
    i32 received = 0;
    while (status == 0 && availableCount > 0 && m_owner->m_pollAbort == false) {
        messageSize = sizeof(g_lobbyRecvBuf);
        IDirectPlay4A* directPlay = m_netMgr->m_directPlay;
        DPID recipientId = m_localPlayer->m_playerId;
        status =
            directPlay
                ->Receive(&senderId, &recipientId, DPRECEIVE_ALL, g_lobbyRecvBuf, &messageSize);
        if (status != 0) {
            CNetMgr::ReportError(
                const_cast<char*>("c:\\proj\\incs\\netmgr.h"),
                0x141,
                status,
                NULL
            );
        }
        if (status == 0) {
            availableCount--;
            received++;
            if (senderId != m_localPlayer->m_playerId) {

                CNetWireMsg wire;
                wire.m_bytes = g_lobbyRecvBuf;
                Dispatch(senderId, wire.m_prefix, messageSize);
            }
        }
    }
    return received;
}

RVA(0x000bf700, 0x82)
i32 CNetSession::Dispatch(i32 senderId, CNetPacketPrefix* message, i32 messageSize) {
    if (!message) {
        return 0;
    }
    if (senderId == 0) {
        CNetWireMsg wire;
        wire.m_prefix = message;
        return DispatchSystemMessage(wire.m_system, messageSize);
    }
    CNetCmdSlot* slot = FindSlotByPlayerId(senderId);
    if (!slot) {
        return 0;
    }
    slot->m_latency = 0;

    if (!(message->m_routeFlags & 0x80) && (message->m_routeFlags & 1)) {
        slot = &m_slots[message->m_routeSlot];
        if (!slot) {
            return 0;
        }
    }
    CNetWireMsg wire;
    wire.m_prefix = message;
    return slot->ProcessPacket(senderId, wire.m_bytes, messageSize);
}

RVA(0x000bf7c0, 0x1b0)
i32 CNetSession::DispatchSystemMessage(LPDPMSG_GENERIC message, i32 messageSize) {
    if (!message) {
        return 0;
    }
    switch (message->dwType) {
        case DPSYS_CREATEPLAYERORGROUP: {
            CNetWireMsg wire;
            wire.m_system = message;
            m_owner->HandlePlayerCreated(wire.m_playerCreated);
            return 1;
        }
        case DPSYS_DESTROYPLAYERORGROUP: {
            CNetWireMsg wire;
            wire.m_system = message;
            if (wire.m_playerDestroyed->dwPlayerType == DPPLAYERTYPE_PLAYER) {
                i32 playerId = wire.m_playerDestroyed->dpId;
                m_owner->OnPlayerLeft(playerId);
                m_owner->ResetPlayerCommands(playerId);
                return 1;
            }
            return 1;
        }
        case DPSYS_SESSIONLOST:
            return m_owner->HandleSystemMessage(message, messageSize);
        case DPSYS_HOST:
            return m_owner->HandleSystemMessage(message, messageSize);
        default:
            return 1;
    }
}

// @early-stop
RVA(0x000bf9e0, 0xfe)
i32 CNetSession::SendTick() {
    if (m_batchBuilt == false && (m_commandTick + 1) % m_commandPeriod == 0) {
        i32 batchSequence = m_sequence + 2;
        GruntRec* record = &m_commandRecords[batchSequence % 0x80];
        record->m_sequence = batchSequence;
        record->m_payloadLength = 0;
        record->m_entryCount = 0;
        record->m_checksum = ComputeChecksum();
        char* payload = record->m_payload;
        i32 commandTick = batchSequence * m_commandPeriod;
        batchSequence = batchSequence + 1;
        for (; commandTick < batchSequence * m_commandPeriod; commandTick++) {
            CGruntzCommand* command = GetCommandAtTick(commandTick);
            if (command) {
                NoopSync(command);
                record->m_entryCount++;

                RecordBytes<GruntRec> recordBytes;
                recordBytes.m_rec = record;
                payload +=
                    command->EncodePacket(payload, recordBytes.m_chars - payload + sizeof(*record));
            }
        }
        m_owner->WriteTag("[end]\n");
        RecordBytes<GruntRec> recordBytes;
        recordBytes.m_rec = record;
        record->m_payloadLength =
            static_cast<i32>(payload - recordBytes.m_chars - offsetof(GruntRec, m_payload));
        m_batchBuilt = true;
    }
    i32 sentCount = SendPendingRecords();
    sentCount += RelayDrainingRecords();
    return sentCount;
}

RVA(0x000bfb20, 0x1)
void NoopSync(CGruntzCommand*) {}

RVA(0x000bfb40, 0xe2)
i32 CNetSession::RelayDrainingRecords() {
    i32 count = 0;
    CNetCmdSlot* source = m_slots;
    for (i32 sourceIndex = 0; sourceIndex < 4; sourceIndex++, source++) {
        if (source && source->m_state == NETSLOT_ACTIVE && source->m_isDraining != false) {
            i32 firstSequence, lastSequence;
            source->GetRecordRange(&firstSequence, &lastSequence);
            CNetCmdSlot* recipient = m_slots;
            i32 recipientsRemaining = 4;
            do {
                if (recipient && recipient->m_state == NETSLOT_ACTIVE
                    && recipient->m_isDraining == false) {
                    for (i32 sequence = firstSequence; sequence <= lastSequence; sequence++) {
                        GruntRec* record = source->FindRecord(sequence);
                        if (record) {

                            u8 flags = 1;
                            if (sequence == lastSequence) {
                                flags = 3;
                            }
                            if (SendGruntRecord(
                                    sequence,
                                    record,
                                    flags,
                                    sourceIndex,
                                    recipient->m_player->m_networkPlayerId
                                )) {
                                count++;
                            }
                        }
                    }
                }
                recipient++;
            } while (--recipientsRemaining);
        }
    }
    return count;
}

// @early-stop
RVA(0x000bfc70, 0x9c)
i32 CNetSession::SendGruntRecord(
    i32 sequence,
    GruntRec* record,
    u8 flags,
    i32 sourceSlot,
    i32 recipientId
) {
    if (!record) {
        return 0;
    }
    if (sequence < 0) {
        return 1;
    }
    g_netGruntRecMsg.m_sequence = sequence;
    g_netGruntRecMsg.m_flags = flags;
    g_netGruntRecMsg.m_slot = static_cast<unsigned char>(sourceSlot);
    g_netGruntRecMsg.m_checksum = record->m_checksum;
    g_netGruntRecMsg.m_entryCount = record->m_entryCount;
    memcpy(g_netGruntRecMsg.m_payload, record->m_payload, record->m_payloadLength);

    i32 result = m_netMgr->SendById(
        m_localPlayer->m_playerId,
        recipientId,
        0,
        &g_netGruntRecMsg,
        record->m_payloadLength + offsetof(NetGruntRecMsg, m_payload)
    );
    return result == 0;
}

RVA(0x000bfd40, 0x116)
i32 CNetSession::SendPendingRecords() {
    i32 count = 0;
    CNetCmdSlot* slot = m_slots;
    i32 slotsRemaining = 4;
    do {
        if (slot && slot->m_state == NETSLOT_ACTIVE && slot->m_isDraining == false) {
            i32 candidateSequence = m_sequence + 2;
            if (m_batchBuilt == false && (m_commandTick + 1) % m_commandPeriod == 0) {
                if (SendRecord(slot, candidateSequence)) {
                    count++;
                }
            }
            candidateSequence = m_sequence + 1;
            if (slot->m_peerWindowBase < candidateSequence
                && slot->ContainsSequence(slot->m_peerReceivedAhead, candidateSequence) == 0) {
                if (SendRecord(slot, candidateSequence)) {
                    count++;
                }
            }
            candidateSequence = m_sequence;
            if (slot->m_peerWindowBase < candidateSequence
                && slot->ContainsSequence(slot->m_peerReceivedAhead, candidateSequence) == 0) {
                if (SendRecord(slot, candidateSequence)) {
                    count++;
                }
            }
            candidateSequence = m_sequence - 1;
            if (slot->m_peerWindowBase < candidateSequence
                && slot->ContainsSequence(slot->m_peerReceivedAhead, candidateSequence) == 0) {
                if (SendRecord(slot, candidateSequence)) {
                    count++;
                }
            }
            candidateSequence = m_sequence - 2;
            if (slot->m_peerWindowBase < candidateSequence
                && slot->ContainsSequence(slot->m_peerReceivedAhead, candidateSequence) == 0) {
                if (SendRecord(slot, candidateSequence)) {
                    count++;
                }
            }
        }
        slot++;
    } while (--slotsRemaining);
    return count;
}

// @early-stop
RVA(0x000bfeb0, 0xfa)
i32 CNetSession::SendRecord(CNetCmdSlot* slot, i32 sequence) {
    if (!slot) {
        return 0;
    }
    if (sequence < 0) {
        return 1;
    }
    unsigned char flags = 0;
    i32 baseSeq = slot->m_contiguousSequence;
    if (slot->ContainsSequence(slot->m_receivedAhead, baseSeq + 2)) {
        flags = 0x10;
    }
    if (slot->ContainsSequence(slot->m_receivedAhead, baseSeq + 3)) {
        flags |= 0x20;
    }
    g_netCmdSendMsg.m_flags = flags;
    g_netCmdSendMsg.m_sequence = sequence;
    i32 recordIndex = sequence % 0x80;
    GruntRec* entry = &m_commandRecords[recordIndex];
    g_netCmdSendMsg.m_windowBase = slot->m_contiguousSequence;
    g_netCmdSendMsg.m_checksum = entry->m_checksum;
    g_netCmdSendMsg.m_entryCount = entry->m_entryCount;
    memcpy(g_netCmdSendMsg.m_payload, entry->m_payload, entry->m_payloadLength);

    i32 status = m_netMgr->SendById(
        m_localPlayer->m_playerId,
        slot->m_player->m_networkPlayerId,
        0,
        &g_netCmdSendMsg,
        entry->m_payloadLength + offsetof(NetCmdSendMsg, m_payload)
    );
    return status == 0;
}

RVA(0x000bfff0, 0x5d)
CNetCmdSlot* CNetSession::CreateSlot(i32 index, NetSlotState state) {
    if (index < 0 || index >= NET_SLOT_COUNT) {
        return NULL;
    }
    CNetCmdSlot* slot = &m_slots[index];
    if (slot == NULL) {
        return NULL;
    }
    (static_cast<CNetCmdSlot*>(slot))->ResetSlot();
    return slot->Initialize(m_owner, &m_mgr->m_players[index], state) ? slot : NULL;
}

RVA(0x000c0070, 0x15)
void CNetSession::ResetLatencies() {
    for (i32 i = 0; i < 4; i++) {
        m_slots[i].m_latency = 0;
    }
}

RVA(0x000c00a0, 0x31)
CNetCmdSlot* CNetSession::FindSlotByPlayerId(i32 playerId) {
    for (i32 i = 0; i < 4; i++) {
        if (m_slots[i].m_player->m_networkPlayerId == playerId) {
            return &m_slots[i];
        }
    }
    return NULL;
}

RVA(0x000c00f0, 0xaf)
void CNetSession::ReconcileDrainingSlots() {
    i32 withFlag = 0;
    i32 withoutFlag = 0;
    CNetCmdSlot* base = m_slots;
    {
        CNetCmdSlot* slot = base;
        i32 slotsRemaining = 4;
        do {
            if (slot) {
                NetSlotState state = slot->m_state;
                if (state == NETSLOT_ACTIVE && slot->m_isDraining != false) {
                    withFlag++;
                }
                if (state == NETSLOT_ACTIVE && slot->m_isDraining == false) {
                    withoutFlag++;
                }
            }
            slot++;
        } while (--slotsRemaining);
    }
    if (withoutFlag == 0) {
        CNetCmdSlot* slot = base;
        i32 slotsRemaining = 4;
        do {
            if (slot && slot->m_state == NETSLOT_ACTIVE) {
                slot->ClearSyncState();
                GruntzPlayer* player = slot->m_player;
                slot->m_state = NETSLOT_DONE;
                player->m_doneFlag = true;
            }
            slot++;
        } while (--slotsRemaining);
    } else if (withFlag != 0) {
        CNetCmdSlot* slot = base;
        i32 slotsRemaining = 4;
        do {
            if (slot && slot->m_state == NETSLOT_ACTIVE && slot->m_isDraining != false
                && m_sequence > slot->m_drainSequence + 2) {
                slot->ClearSyncState();
                GruntzPlayer* player = slot->m_player;
                slot->m_state = NETSLOT_DONE;
                player->m_doneFlag = true;
            }
            slot++;
        } while (--slotsRemaining);
    }
}

RVA(0x000c01d0, 0x8c)
i32 CNetSession::AdvanceTick() {
    i32 nextTick = m_commandTick + 1;
    i32 nextSeq = m_sequence + 1;
    if (nextTick % m_commandPeriod != 0) {
        m_commandTick = nextTick;
        return 1;
    }
    ReconcileDrainingSlots();
    if (!ReadyForSequence(nextSeq)) {
        return 0;
    }
    CNetCmdSlot* slot = m_slots;
    i32 slotsRemaining = 4;
    do {
        if (slot && slot->m_state == NETSLOT_ACTIVE && slot->m_isDraining == false) {
            slot->RemoveRecord(m_sequence - 4);
        }
        slot++;
    } while (--slotsRemaining);
    m_commandTick = nextTick;
    m_sequence = nextSeq;
    m_batchBuilt = false;
    return 1;
}

RVA(0x000c0290, 0x63)
i32 CNetSession::ReadyForSequence(i32 sequence) {
    for (i32 i = 0; i < 4; i++) {
        CNetCmdSlot* slot = &m_slots[i];
        if (slot != NULL) {
            if (slot->m_state == NETSLOT_ACTIVE && slot->m_isDraining == false) {
                if (slot->m_contiguousSequence < sequence) {
                    return 0;
                }
            } else if (slot->m_state == NETSLOT_ACTIVE && slot->m_isDraining != false) {
                if (slot->DrainAcknowledged() == 0) {
                    return 0;
                }
                if (slot->m_drainSequence != slot->m_contiguousSequence) {
                    return 0;
                }
            }
        }
    }
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000c0320, 0x37)
i32 CNetSession::AllPeerWindowsReached(i32 sequence) {
    for (i32 i = 0; i < 4; i++) {
        CNetCmdSlot* slot = &m_slots[i];
        if (slot != NULL && slot->m_state == NETSLOT_ACTIVE && slot->m_isDraining == false
            && slot->m_peerWindowBase < sequence) {
            return 0;
        }
    }
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000c0370, 0x28)
void CNetSession::RecordSequenceForAllSlots(i32 sequence) {
    CNetCmdSlot* slot = m_slots;
    for (i32 i = 4; i != 0; i--) {
        if (slot->m_state == NETSLOT_ACTIVE) {
            slot->RecordReceivedSequence(sequence);
        }
        slot++;
    }
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000c03b0, 0x28)
void CNetSession::RecordPeerWindowForAllSlots(i32 sequence) {
    CNetCmdSlot* slot = m_slots;
    for (i32 i = 4; i != 0; i--) {
        if (slot->m_state == NETSLOT_ACTIVE) {
            slot->RecordPeerWindowBase(sequence);
        }
        slot++;
    }
}

RVA(0x000c03f0, 0x29)
void CNetSession::ScheduleCommand(CGruntzCommand* command, u8 tickOffset) {
    m_commandByTick[(m_commandTick + tickOffset) % 128] = command;
}

RVA(0x000c0430, 0x1f)
CGruntzCommand* CNetSession::GetCommandAtTick(i32 commandTick) {
    return m_commandByTick[(commandTick & 0xff) % 128];
}

RVA(0x000c0460, 0x2e)
CNetCmdSlot* CNetSession::FindLaggingSlot(u32 latencyThreshold) {

    for (i32 i = 0; i < 4; i++) {
        CNetCmdSlot* slot = &m_slots[i];
        if (slot && slot->m_state == NETSLOT_ACTIVE && slot->m_isDraining == false
            && static_cast<u32>(slot->m_latency) > latencyThreshold) {
            return slot;
        }
    }
    return NULL;
}

RVA(0x000c04a0, 0x37)
i32 CNetSession::AllActiveLatenciesWithin(i32 latencyLimit) {
    for (i32 i = 0; i < 4; i++) {
        CNetCmdSlot* slot = &m_slots[i];
        if (slot != NULL && slot->m_state == NETSLOT_ACTIVE && slot->m_isDraining == false
            && static_cast<u32>(slot->m_latency) > static_cast<u32>(latencyLimit)) {
            return 0;
        }
    }
    return 1;
}

RVA(0x000c04f0, 0x7c)
i32 CNetSession::VerifyChecksums() {
    i32 sequence = m_sequence - 2;
    GruntRec* expected = &m_commandRecords[sequence % 128];
    if (expected != NULL) {
        for (i32 i = 0; i < 4; i++) {
            CNetCmdSlot* slot = &m_slots[i];
            if (slot != NULL && slot->m_state == NETSLOT_ACTIVE && slot->m_isDraining == false) {
                GruntRec* received = slot->FindRecord(sequence);
                if (received != NULL && received->m_checksum != expected->m_checksum) {
                    return 0;
                }
            }
        }
    }
    return 1;
}

RVA(0x000c0590, 0x21c)
i32 CNetSession::ComputeChecksum() {
    i32 sum = 0;
    for (i32 player = 0; player < TM_PLAYER_COUNT; player++) {
        for (i32 g = 0; g < TM_UNITS_PER_PLAYER; g++) {
            CGrunt* grunt = m_owner->m_mgr->m_triggerMgr->m_units[player * TM_UNITS_PER_PLAYER + g];
            if (grunt != NULL) {
                sum += IDX(grunt->m_entranceCell.direction) + grunt->m_stamina + grunt->m_toyTime
                       + grunt->m_health + grunt->m_object->m_screenY + grunt->m_object->m_sortKey
                       + grunt->m_object->m_screenX + grunt->LastTilePx().m_x
                       + grunt->LastTilePx().m_y;

                PickupType carried = grunt->m_entranceReason;
                PickupType effective = ArrivalPickupOf(grunt, carried);
                sum += IDX(grunt->m_vehiclePickupType) + grunt->m_entranceCommitted
                       + grunt->m_entranceActive + grunt->m_daFlag + IDX(effective);

                PickupType next;
                switch (carried) {
                    case PICKUP_BOMB:
                        next = PICKUP_BOOMERANG;
                        break;
                    case PICKUP_BOOMERANG:
                        next = PICKUP_BRICK;
                        break;
                    case PICKUP_BRICK:
                        next = PICKUP_CLUB;
                        break;
                    case PICKUP_CLUB:
                        next = PICKUP_GAUNTLETZ;
                        break;
                    case PICKUP_GAUNTLETZ:
                        next = PICKUP_GLOVEZ;
                        break;
                    case PICKUP_GLOVEZ:
                        next = PICKUP_GOOBER;
                        break;
                    case PICKUP_GOOBER:
                        next = PICKUP_GRAVITYBOOTZ;
                        break;
                    case PICKUP_GRAVITYBOOTZ:
                        next = PICKUP_GUNHAT;
                        break;
                    case PICKUP_GUNHAT:
                        next = PICKUP_NERFGUN;
                        break;
                    case PICKUP_NERFGUN:
                        next = PICKUP_ROCK;
                        break;
                    case PICKUP_ROCK:
                        next = PICKUP_SHIELD;
                        break;
                    case PICKUP_SHIELD:
                        next = PICKUP_SHOVEL;
                        break;
                    case PICKUP_SHOVEL:
                        next = PICKUP_SPRING;
                        break;
                    case PICKUP_SPRING:
                        next = PICKUP_SPY;
                        break;
                    case PICKUP_SPY:
                        next = PICKUP_SWORD;
                        break;
                    case PICKUP_SWORD:
                        next = PICKUP_TIMEBOMB;
                        break;
                    case PICKUP_TIMEBOMB:
                        next = PICKUP_TOOB;
                        break;
                    case PICKUP_TOOB:
                        next = PICKUP_WAND;
                        break;
                    case PICKUP_WAND:
                        next = PICKUP_WARPSTONE;
                        break;
                    case PICKUP_WARPSTONE:
                        next = PICKUP_WELDER;
                        break;
                    case PICKUP_WELDER:
                        next = PICKUP_WINGZ;
                        break;
                    case PICKUP_WINGZ:
                        next = PICKUP_BABYWALKER;
                        break;
                    default:
                        next = PICKUP_BABYWALKER;
                        break;
                }

                sum += grunt->m_arrivalPhase + grunt->m_neighborScanEnabled + grunt->m_combatActive
                       + grunt->m_neighborValid + grunt->m_poweredUp + static_cast<i32>(g_frameTime)
                       + IDX(next);
                sum += rand();
            }
        }
    }
    return sum;
}
