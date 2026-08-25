#include <rva.h>

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

#include <dplay.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

DATA(0x00249858)
char g_lobbyRecvBuf[0x800];
DATA(0x0024a058)
NetCmdSendMsg g_netCmdSendMsg;
DATA(0x0024a8a8)
NetGruntRecMsg g_netGruntRecMsg;

RVA_DYNINIT(0x000beef0, 0xa, s_freeList)
RVA_DYNINIT(0x000bef10, 0xd, s_freeList)
RVA_DYNINIT(0x000bef30, 0xe, s_freeList)
RVA_DYNINIT(0x000bef50, 0x1f, s_freeList)
template<> DATA(0x0024aca8)
CPtrList CPtrListPool<GruntRec>::s_freeList(0xa);

DATA(0x0024b6a0)
char g_sequenceScratch[0x10];

DATA(0x0024b6b0)
char g_sequenceListBuffer[0x40];

RVA(0x000c0b10, 0x72)
i32 CNetCmdSlot::Initialize(CMulti* owner, GruntzPlayer* player, NetSlotState state) {
    if (player == NULL) {
        return 0;
    }
    if (owner == NULL) {
        return 0;
    }
    m_state = state;
    m_isDraining = 0;
    m_drainSequence = 0;
    m_player = player;
    m_latency = 0;
    m_contiguousSequence = 0;
    m_peerWindowBase = 0;
    m_owner = owner;
    ResetNetCmdSlotCommandWindow(this);
    return 1;
}

RVA(0x000c0bb0, 0x47)
void CNetCmdSlot::ResetSlot() {
    m_state = NETSLOT_EMPTY;
    m_isDraining = 0;
    m_drainSequence = 0;
    m_player = NULL;
    m_latency = 0;
    m_contiguousSequence = 0;
    m_peerWindowBase = 0;
    m_owner = NULL;
    ResetNetCmdSlotCommandWindow(this);
}

RVA(0x000c0c20, 0x3f)
void CNetCmdSlot::ClearSyncState() {
    m_isDraining = 0;
    m_drainSequence = 0;
    m_latency = 0;
    m_contiguousSequence = 0;
    m_peerWindowBase = 0;
    ResetNetCmdSlotCommandWindow(this);
}

RVA(0x000c0c70, 0x20f)
i32 CNetCmdSlot::ProcessPacket(i32 playerId, char* packet, i32 packetSize) {
    if (packet == NULL) {
        return 0;
    }
    u8 opcode = static_cast<u8>(*packet);
    i32 isDrainPacket = opcode & 1;
    char* packetStart = packet;
    packet++;
    if (m_state != NETSLOT_ACTIVE) {
        return 1;
    }
    if (opcode & 0x80) {
        return m_owner->DispatchRecvMsg(m_player->m_networkPlayerId, packetStart, packetSize);
    }
    if (isDrainPacket == 0) {
        if (m_isDraining != 0) {
            return 1;
        }
    }
    if (isDrainPacket) {
        if (m_isDraining == 0) {
            return 1;
        }
    }

    i32 remaining = packetSize - 1;
    if (isDrainPacket) {
        packet++;
        remaining--;
    }

    CNetWireMsg wire;
    wire.m_bytes = packet;
    CNetCmdHdr* header = wire.m_cmdHdr;
    i32 sequence = header->m_sequence;
    i32 windowBase = header->m_windowBase;
    i32 checksum = header->m_checksum;
    u8 entryCount = header->m_entryCount;
    char* cursor = packet + 13;
    remaining -= 13;

    if (m_isDraining != 0 && isDrainPacket) {
        CNetCmdSlot* slot = m_owner->m_session->FindSlotByPlayerId(playerId);
        if (slot == NULL) {
            return 0;
        }
        if (opcode & 2) {
            i32 ackPlayerIndex = slot->m_player->m_playerIndex & 0xff;
            m_drainAckFlags[ackPlayerIndex] = 1;
            if (sequence > m_drainSequence) {
                m_drainSequence = sequence;
            }
        }
    }

    RecordPeerWindowBase(windowBase);
    if (opcode & 0x10) {
        AddSequence(m_peerReceivedAhead, windowBase + 2);
    } else if (opcode & 0x20) {
        AddSequence(m_peerReceivedAhead, windowBase + 3);
    }
    RemoveSequence(m_peerReceivedAhead, windowBase + 1);

    if (m_contiguousSequence >= sequence) {
        return 1;
    }
    if (ContainsSequence(m_receivedAhead, sequence)) {
        return 1;
    }
    RecordReceivedSequence(sequence);

    GruntRec* record = AllocateGruntRecord(0);
    record->m_entryCount = entryCount;
    record->m_checksum = checksum;
    record->m_sequence = sequence;
    record->m_payloadLength = remaining;
    memcpy(record->m_payload, cursor, remaining);
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
        m_owner->m_mgr->m_commandMgr->EnqueueCommand(0, command);
    }
    return 1;
}

// @early-stop
RVA(0x000c0f10, 0x6e)
void CNetCmdSlot::RecordReceivedSequence(i32 sequence) {
    if (m_contiguousSequence + 1 == sequence) {
        RemoveSequence(m_receivedAhead, m_contiguousSequence);
        m_contiguousSequence++;
        while (ContainsSequence(m_receivedAhead, m_contiguousSequence + 1)) {
            m_contiguousSequence++;
            RemoveSequence(m_receivedAhead, m_contiguousSequence);
        }
    } else {
        AddSequence(m_receivedAhead, sequence);
    }
}

RVA(0x000c0fa0, 0x11)
void CNetCmdSlot::RecordPeerWindowBase(i32 sequence) {
    if (sequence > m_peerWindowBase) {
        m_peerWindowBase = sequence;
    }
}

RVA(0x000c0fd0, 0x24)
i32 CNetCmdSlot::ContainsSequence(i32* sequences, i32 sequence) {
    for (i32 i = 0; i < 3; i++) {
        if (sequence == sequences[i]) {
            return 1;
        }
    }
    return 0;
}

RVA(0x000c1010, 0x32)
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

RVA(0x000c1060, 0x29)
void CNetCmdSlot::RemoveSequence(i32* sequences, i32 sequence) {
    for (i32 i = 0; i < 3; i++) {
        if (sequence == sequences[i]) {
            sequences[i] = -1;
            return;
        }
    }
}

RVA(0x000c10a0, 0x12)
void CNetCmdSlot::ClearSequenceSet(i32* sequences) {

    for (i32 i = 0; i < 3; i++) {
        sequences[i] = -1;
    }
}

// @early-stop
// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000c10d0, 0x7c)
char* __stdcall SequenceSetToString(i32* sequences) {
    g_sequenceListBuffer[0] = 0;
    i32 remaining = 3;
    do {
        if (*sequences != -1) {
            wsprintfA(g_sequenceScratch, "%d,", *sequences);
            strcat(g_sequenceListBuffer, g_sequenceScratch);
        }
        sequences++;
    } while (--remaining != 0);
    return g_sequenceListBuffer;
}

RVA(0x000c1170, 0x26)
void CNetCmdSlot::AddRecord(GruntRec* record) {
    if (record != NULL && FindRecord(record->m_sequence) == NULL) {
        m_records.AddTail(record);
    }
}

RVA(0x000c11b0, 0x55)
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

RVA(0x000c1230, 0x55)
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

RVA(0x000c12b0, 0x1f)
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

RVA(0x000c12e0, 0x2c)
void CNetCmdSlot::ClearRecords() {
    while (m_records.GetCount() != 0) {
        GruntRec* record = static_cast<GruntRec*>(m_records.RemoveHead());
        if (record != NULL) {
            RecycleGruntRecord(record);
        }
    }
}

RVA(0x000c1320, 0x4a)
i32 CNetCmdSlot::DrainAcknowledged() {
    CMulti* owner = m_owner;
    if (owner == NULL) {
        return 0;
    }
    CNetSession* session = owner->m_session;
    for (i32 i = 0; i < 4; i++) {
        CNetCmdSlot* slot = &session->m_slots[i];
        if (slot != NULL && slot->m_state == NETSLOT_ACTIVE && slot->m_isDraining == 0
            && m_drainAckFlags[i] == 0) {
            return 0;
        }
    }
    return 1;
}

RVA(0x000c1390, 0x15)
void CNetCmdSlot::BeginDrain() {
    if (m_isDraining == 0) {
        m_isDraining = 1;
        m_drainSequence = m_contiguousSequence;
    }
}
