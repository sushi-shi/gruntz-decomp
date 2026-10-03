#include <StdAfx.h>

#include <Ints.h>

#include <Net/NetCmdSlot.h>

#include <Gruntz/Grunt.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzCmdMgr.h>
#include <Gruntz/GruntzCommand.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/Multi.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/TriggerMgr.h>
#include <Ints.h>
#include <Net/CmdPool.h>
#include <Net/NetCmdSlotInline.h>
#include <Net/NetMgr.h>
#include <Net/NetSlotState.h>
#include <Pix16.h>
#include <Rez/RezMgr.h>
#include <Utils/PackedReadWrite.h>

#include <dplay.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

char g_lobbyRecvBuf[NET_RECEIVE_BUFFER_BYTES];

NetCmdSendMsg g_netCmdSendMsg;

NetGruntRecMsg g_netGruntRecMsg;

template<>
CPtrList CPtrListPool<GruntRec>::s_freeList(0xa);

char g_sequenceScratch[0x10];

char g_sequenceListBuffer[0x40];

i32 CNetCmdSlot::Initialize(CMulti* owner, GruntzPlayer* player, NetSlotState state) {
    if (player == NULL) {
        return 0;
    }
    if (owner == NULL) {
        return 0;
    }
    m_state = state;
    m_isDraining = false;
    m_drainSequence = 0;
    m_player = player;
    m_latency = 0;
    m_contiguousSequence = 0;
    m_peerWindowBase = 0;
    m_owner = owner;
    ResetNetCmdSlotCommandWindow(this);
    return 1;
}

void CNetCmdSlot::ResetSlot() {
    m_state = NETSLOT_EMPTY;
    m_isDraining = false;
    m_drainSequence = 0;
    m_player = NULL;
    m_latency = 0;
    m_contiguousSequence = 0;
    m_peerWindowBase = 0;
    m_owner = NULL;
    ResetNetCmdSlotCommandWindow(this);
}

void CNetCmdSlot::ClearSyncState() {
    m_isDraining = false;
    m_drainSequence = 0;
    m_latency = 0;
    m_contiguousSequence = 0;
    m_peerWindowBase = 0;
    ResetNetCmdSlotCommandWindow(this);
}

inline void CNetCmdSlot::QueueRecord(GruntRec* record, u8 entryCount, char* cursor, i32 remaining) {
    AddRecord(record);

    for (i32 i = entryCount & 0xff; i > 0; i--) {
        u8 commandFlags = static_cast<u8>(*cursor);
        CGruntzCommand* command;
        if (commandFlags & 1) {
            command = CGruntzSingleCommand::Allocate();
        } else if (commandFlags & 2) {
            command = CGruntzMultiCommand::Allocate();
        } else {
            continue;
        }
        i32 consumed = command->DecodePacket(cursor, remaining);
        command->m_submitFlags = COMMAND_SUBMIT_SCHEDULED;
        remaining -= consumed;
        cursor += consumed;
        m_owner->Mgr()->m_commandMgr->EnqueueCommand(false, command);
    }
}

i32 CNetCmdSlot::ProcessPacket(i32 playerId, char* packet, i32 packetSize) {
    if (packet == NULL) {
        return 0;
    }
    u8 opcode = static_cast<u8>(*packet);
    b32 isDrainPacket = (opcode & 1) != 0;
    char* cursor = packet + 1;
    if (m_state != NETSLOT_ACTIVE) {
        return 1;
    }
    if (opcode & 0x80) {
        return m_owner->DispatchRecvMsg(m_player->m_networkPlayerId, packet, packetSize);
    }
    if (isDrainPacket == false) {
        if (m_isDraining != false) {
            return 1;
        }
    }
    if (isDrainPacket) {
        if (m_isDraining == false) {
            return 1;
        }
    }

    i32 remaining = packetSize - 1;
    if (isDrainPacket) {
        cursor++;
        remaining--;
    }

    i32 sequence = PeekI32(cursor);
    cursor += 4;
    remaining -= 4;
    i32 windowBase = PeekI32(cursor);
    cursor += 4;
    remaining -= 4;
    i32 checksum = PeekI32(cursor);
    cursor += 4;
    remaining -= 4;
    u8 entryCount = *cursor;
    cursor++;
    remaining--;

    if (m_isDraining != false && isDrainPacket) {
        CNetCmdSlot* slot = m_owner->Session()->FindSlotByPlayerId(playerId);
        if (slot == NULL) {
            return 0;
        }
        i32 ackPlayerIndex = slot->m_player->m_playerIndex;
        if (opcode & 2) {
            m_drainAckFlags[ackPlayerIndex & 0xff] = 1;
            if (sequence > m_drainSequence) {
                m_drainSequence = sequence;
            }
        }
    }

    RecordPeerWindowBase(windowBase);
    if (opcode & 0x10) {
        AddSequence(PeerReceivedAhead(), windowBase + 2);
    } else if (opcode & 0x20) {
        AddSequence(PeerReceivedAhead(), windowBase + 3);
    }
    RemoveSequence(PeerReceivedAhead(), windowBase + 1);

    if (HasReceivedThrough(sequence)) {
        return 1;
    }
    if (ContainsSequence(ReceivedAhead(), sequence)) {
        return 1;
    }
    RecordReceivedSequence(sequence);

    GruntRec* record = AllocateGruntRecord(0);
    record->m_entryCount = entryCount;
    record->m_checksum = checksum;
    record->m_sequence = sequence;
    record->m_payloadLength = remaining;
    memcpy(record->m_payload, cursor, remaining);
    QueueRecord(record, entryCount, cursor, remaining);
    return 1;
}

void CNetCmdSlot::RecordReceivedSequence(i32 sequence) {
    if (m_contiguousSequence + 1 == sequence) {
        RemoveSequence(ReceivedAhead(), m_contiguousSequence);
        m_contiguousSequence++;
        while (ContainsSequence(ReceivedAhead(), m_contiguousSequence + 1)) {
            m_contiguousSequence++;
            RemoveSequence(ReceivedAhead(), m_contiguousSequence);
        }
    } else {
        AddSequence(ReceivedAhead(), sequence);
    }
}

void CNetCmdSlot::RecordPeerWindowBase(i32 sequence) {
    if (sequence > m_peerWindowBase) {
        m_peerWindowBase = sequence;
    }
}

i32 CNetCmdSlot::ContainsSequence(i32* sequences, i32 sequence) {
    for (i32 i = 0; i < 3; i++) {
        if (sequence == sequences[i]) {
            return 1;
        }
    }
    return 0;
}

void CNetCmdSlot::AddSequence(i32* sequences, i32 sequence) {
    if (ContainsSequence(sequences, sequence)) {
        return;
    }
    for (i32 i = 0; i < 3; i++) {
        if (sequences[i] == -1) {
            sequences[i] = sequence;
            return;
        }
    }
}

void CNetCmdSlot::RemoveSequence(i32* sequences, i32 sequence) {
    for (i32 i = 0; i < 3; i++) {
        if (sequence == sequences[i]) {
            sequences[i] = -1;
            return;
        }
    }
}

void CNetCmdSlot::ClearSequenceSet(i32* sequences) {

    for (i32 i = 0; i < 3; i++) {
        sequences[i] = -1;
    }
}

char* __stdcall SequenceSetToString(i32* sequences) {
    g_sequenceListBuffer[0] = 0;
    for (i32 i = 0; i < 3; i++) {
        if (sequences[i] != -1) {
            wsprintfA(g_sequenceScratch, "%d,", sequences[i]);
            strcat(g_sequenceListBuffer, g_sequenceScratch);
        }
    }
    return g_sequenceListBuffer;
}

void CNetCmdSlot::AddRecord(GruntRec* record) {
    if (record != NULL && FindRecord(record->m_sequence) == NULL) {
        m_records.AddTail(record);
    }
}

void CNetCmdSlot::RemoveRecord(i32 sequence) {
    POSITION pos = m_records.GetHeadPosition();
    while (pos != NULL) {
        GruntRec* record = static_cast<GruntRec*>(m_records.GetNext(pos));
        if (sequence == record->m_sequence) {
            if (pos != NULL) {

                m_records.GetPrev(pos);
                m_records.RemoveAt(pos);
            } else {
                m_records.RemoveTail();
            }
            RecycleGruntRecord(record);
            return;
        }
    }
}

void CNetCmdSlot::GetRecordRange(i32* minimum, i32* maximum) {
    if (minimum == NULL) {
        return;
    }
    if (maximum == NULL) {
        return;
    }
    *maximum = 0x80000001;
    *minimum = INT_MAX;
    POSITION pos = m_records.GetHeadPosition();
    if (pos == NULL) {
        *maximum = 0;
        *minimum = 0;
        return;
    }
    do {
        GruntRec* record = static_cast<GruntRec*>(m_records.GetNext(pos));
        if (record->m_sequence > *maximum) {
            *maximum = record->m_sequence;
        }
        if (record->m_sequence < *minimum) {
            *minimum = record->m_sequence;
        }
    } while (pos != NULL);
}

GruntRec* CNetCmdSlot::FindRecord(i32 sequence) {
    POSITION pos = m_records.GetHeadPosition();
    while (pos != NULL) {
        GruntRec* record = static_cast<GruntRec*>(m_records.GetNext(pos));
        if (sequence == record->m_sequence) {
            return record;
        }
    }
    return NULL;
}

void CNetCmdSlot::ClearRecords() {
    while (!m_records.IsEmpty()) {
        GruntRec* record = static_cast<GruntRec*>(m_records.RemoveHead());
        if (record != NULL) {
            RecycleGruntRecord(record);
        }
    }
}

i32 CNetCmdSlot::DrainAcknowledged() {
    if (m_owner == NULL) {
        return 0;
    }
    for (i32 i = 0; i < 4; i++) {
        CNetCmdSlot* slot = &m_owner->Session()->m_slots[i];
        if (slot != NULL && slot->m_state == NETSLOT_ACTIVE && slot->m_isDraining == false
            && m_drainAckFlags[i] == 0) {
            return 0;
        }
    }
    return 1;
}

void CNetCmdSlot::BeginDrain() {
    if (m_isDraining == false) {
        m_isDraining = true;
        m_drainSequence = m_contiguousSequence;
    }
}
