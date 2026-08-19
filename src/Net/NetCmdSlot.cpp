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
char g_idScratch[0x10];

DATA(0x0024b6b0)
char g_idListBuf[0x40];

// @early-stop
RVA(0x000c0b10, 0x72)
i32 CNetCmdSlot::Init(CMulti* owner, GruntzPlayer* desc, NetSlotState state) {
    if (desc == NULL) {
        return 0;
    }
    if (owner == NULL) {
        return 0;
    }
    m_owner = owner;
    m_state = state;
    m_isRemote = 0;
    m_latchedSeq = 0;
    m_desc = desc;
    m_latency = 0;
    m_baseSeq = 0;
    m_maxSeq = 0;
    ClearCmds();

    for (i32 i = 0; i < NET_SLOT_COUNT; i++) {
        m_ackFlags[i] = 0;
    }
    ResetTriple(m_rangeA);
    ResetTriple(m_rangeB);
    return 1;
}

RVA(0x000c0bb0, 0x47)
void CNetCmdSlot::ResetAll() {
    m_state = NETSLOT_EMPTY;
    m_isRemote = 0;
    m_latchedSeq = 0;
    m_desc = NULL;
    m_latency = 0;
    m_baseSeq = 0;
    m_maxSeq = 0;
    m_owner = NULL;
    ClearCmds();

    for (i32 i = 0; i < NET_SLOT_COUNT; i++) {
        m_ackFlags[i] = 0;
    }
    ResetTriple(m_rangeA);
    ResetTriple(m_rangeB);
}

RVA(0x000c0c20, 0x3f)
void CNetCmdSlot::FullReset() {
    m_isRemote = 0;
    m_latchedSeq = 0;
    m_latency = 0;
    m_baseSeq = 0;
    m_maxSeq = 0;
    ClearCmds();

    for (i32 i = 0; i < NET_SLOT_COUNT; i++) {
        m_ackFlags[i] = 0;
    }
    ResetTriple(m_rangeA);
    ResetTriple(m_rangeB);
}

// @early-stop
RVA(0x000c0c70, 0x20f)
i32 CNetCmdSlot::ProcessCmd(i32 playerId, char* rec, i32 size) {
    if (rec == NULL) {
        return 0;
    }
    u8 opcode = static_cast<u8>(*rec);
    i32 odd = opcode & 1;
    char* p = rec + 1;
    if (m_state != NETSLOT_ACTIVE) {
        return 1;
    }
    if (opcode & 0x80) {
        return m_owner->DispatchRecvMsg(m_desc->m_slotKey, rec, size);
    }
    if (odd == 0) {
        if (m_isRemote != 0) {
            return 1;
        }
    }
    if (odd) {
        if (m_isRemote == 0) {
            return 1;
        }
    }

    i32 rem = size - 1;
    if (odd) {
        p++;
        rem--;
    }

    CNetWireMsg wire;
    wire.m_bytes = p;
    CNetCmdHdr* h = wire.m_cmdHdr;
    i32 seq = h->m_sequence;
    i32 base = h->m_windowBase;
    i32 checksum = h->m_checksum;
    u8 count = h->m_entryCount;
    char* cursor = p + 13;
    rem -= 13;

    if (m_isRemote != 0 && odd) {
        CNetCmdSlot* slot = m_owner->m_session->FindCmdSlot(playerId);
        if (slot == NULL) {
            return 0;
        }
        if (opcode & 2) {
            i32 pid = slot->m_desc->m_playerIndex & 0xff;
            m_ackFlags[pid] = 1;
            if (seq > m_latchedSeq) {
                m_latchedSeq = seq;
            }
        }
    }

    RaiseMax(base);
    if (opcode & 0x10) {
        NetCmdIdAdd(m_rangeB, base + 2);
    } else if (opcode & 0x20) {
        NetCmdIdAdd(m_rangeB, base + 3);
    }
    NetCmdIdClear(m_rangeB, base + 1);

    if (m_baseSeq >= seq) {
        return 1;
    }
    if (NetCmdIdFind(m_rangeA, seq)) {
        return 1;
    }
    AdvanceSeq(seq);

    GruntRec* pkt = AllocateGruntRecord(0);
    pkt->m_count = count;
    pkt->m_checksum = checksum;
    pkt->m_seq = seq;
    pkt->m_payloadLen = rem;
    memcpy(pkt->m_payload, cursor, rem);
    AddCmd(pkt);

    for (i32 i = count & 0xff; i > 0; i--) {
        u8 b = static_cast<u8>(*cursor);
        CGruntzCommand* obj;
        if (b & 1) {
            obj = CGruntzSingleCommand::Allocate();
        } else if (b & 2) {
            obj = CGruntzMultiCommand::Allocate();
        } else {
            continue;
        }
        i32 consumed = obj->Parse(cursor, rem);
        obj->m_submitted = COMMAND_SUBMIT_SCHEDULED;

        m_owner->m_mgr->m_cmdSubMgr->EnqueueCommand(0, obj);
        rem -= consumed;
        cursor += consumed;
    }
    return 1;
}

// @early-stop
RVA(0x000c0f10, 0x6e)
void CNetCmdSlot::AdvanceSeq(i32 id) {
    if (m_baseSeq + 1 == id) {
        NetCmdIdClear(m_rangeA, m_baseSeq);
        m_baseSeq++;
        while (NetCmdIdFind(m_rangeA, m_baseSeq + 1)) {
            m_baseSeq++;
            NetCmdIdClear(m_rangeA, m_baseSeq);
        }
    } else {
        NetCmdIdAdd(m_rangeA, id);
    }
}

RVA(0x000c0fa0, 0x11)
void CNetCmdSlot::RaiseMax(i32 v) {
    if (v > m_maxSeq) {
        m_maxSeq = v;
    }
}

RVA(0x000c0fd0, 0x24)
i32 CNetCmdSlot::NetCmdIdFind(i32* arr, i32 v) {
    for (i32 i = 0; i < 3; i++) {
        if (v == arr[i]) {
            return 1;
        }
    }
    return 0;
}

RVA(0x000c1010, 0x32)
void CNetCmdSlot::NetCmdIdAdd(i32* arr, i32 v) {
    if (NetCmdIdFind(arr, v)) {
        return;
    }
    for (i32 i = 0; i < 3; i++) {
        if (arr[i] == -1) {
            arr[i] = v;
            return;
        }
    }
}

RVA(0x000c1060, 0x29)
void CNetCmdSlot::NetCmdIdClear(i32* arr, i32 v) {
    for (i32 i = 0; i < 3; i++) {
        if (v == arr[i]) {
            arr[i] = -1;
            return;
        }
    }
}

RVA(0x000c10a0, 0x12)
void CNetCmdSlot::ResetTriple(i32* p) {

    for (i32 i = 0; i < 3; i++) {
        p[i] = -1;
    }
}

// @early-stop
// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000c10d0, 0x7c)
char* __stdcall NetCmdIdToString(i32* arr) {
    g_idListBuf[0] = 0;
    i32 n = 3;
    do {
        if (*arr != -1) {
            wsprintfA(g_idScratch, "%d,", *arr);
            strcat(g_idListBuf, g_idScratch);
        }
        arr++;
    } while (--n != 0);
    return g_idListBuf;
}

RVA(0x000c1170, 0x26)
void CNetCmdSlot::AddCmd(GruntRec* cmd) {
    if (cmd != NULL && FindCmd(cmd->m_seq) == NULL) {
        m_cmds.AddTail(cmd);
    }
}

RVA(0x000c11b0, 0x55)
void CNetCmdSlot::RemoveCmd(i32 seq) {
    POSITION pos = m_cmds.GetHeadPosition();
    while (pos != NULL) {
        GruntRec* cmd = static_cast<GruntRec*>(m_cmds.GetNext(pos));
        if (seq == cmd->m_seq) {
            if (pos != NULL) {

                m_cmds.GetPrev(pos);
                m_cmds.RemoveAt(pos);
            } else {
                m_cmds.RemoveTail();
            }
            RecycleCmd(cmd);
            return;
        }
    }
}

RVA(0x000c1230, 0x55)
void CNetCmdSlot::GetRange(i32* pMin, i32* pMax) {
    if (pMin == NULL) {
        return;
    }
    if (pMax == NULL) {
        return;
    }
    *pMax = 0x80000001;
    *pMin = INT_MAX;
    POSITION pos = m_cmds.GetHeadPosition();
    if (pos == NULL) {
        *pMax = 0;
        *pMin = 0;
        return;
    }
    do {
        GruntRec* cmd = static_cast<GruntRec*>(m_cmds.GetNext(pos));
        if (cmd->m_seq > *pMax) {
            *pMax = cmd->m_seq;
        }
        if (cmd->m_seq < *pMin) {
            *pMin = cmd->m_seq;
        }
    } while (pos != NULL);
}

RVA(0x000c12b0, 0x1f)
GruntRec* CNetCmdSlot::FindCmd(i32 seq) {
    POSITION pos = m_cmds.GetHeadPosition();
    while (pos != NULL) {
        GruntRec* cmd = static_cast<GruntRec*>(m_cmds.GetNext(pos));
        if (seq == cmd->m_seq) {
            return cmd;
        }
    }
    return 0;
}

RVA(0x000c12e0, 0x2c)
void CNetCmdSlot::ClearCmds() {
    while (m_cmds.GetCount() != 0) {
        GruntRec* cmd = static_cast<GruntRec*>(m_cmds.RemoveHead());
        if (cmd != NULL) {
            RecycleCmd(cmd);
        }
    }
}

// @early-stop
RVA(0x000c1320, 0x4a)
i32 CNetCmdSlot::Ready() {
    CMulti* mgr = m_owner;
    if (mgr == NULL) {
        return 0;
    }
    CNetSession* sess = mgr->m_session;
    for (i32 i = 0; i < 4; i++) {
        CNetCmdSlot* slot = &sess->m_slots[i];
        if (slot != NULL && slot->m_state == NETSLOT_ACTIVE && slot->m_isRemote == 0
            && m_ackFlags[i] == 0) {
            return 0;
        }
    }
    return 1;
}

RVA(0x000c1390, 0x15)
void CNetCmdSlot::Touch() {
    if (m_isRemote == 0) {
        m_isRemote = 1;
        m_latchedSeq = m_baseSeq;
    }
}
