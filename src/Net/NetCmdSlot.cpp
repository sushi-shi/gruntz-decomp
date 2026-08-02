#include <Net/CmdPool.h>
#include <Net/NetMgr.h>
#include <Gruntz/Multi.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/GruntzCmdMgr.h>
#include <Gruntz/GruntzCommand.h>
#include <Ints.h>
#include <Rez/RezMgr.h>
#include <dplay.h>
#include <rva.h>
#include <Pix16.h>
#include <string.h>
#include <Net/NetCmdSlot.h>

#pragma intrinsic(memcpy)
#pragma intrinsic(strcat)

char g_lobbyRecvBuf[0x800];
DATA(0x0024a058)
NetCmdSendMsg g_netCmdSendMsg;
DATA(0x0024a8a8)
NetGruntRecMsg g_netGruntRecMsg;

template<> DATA(0x0024aca8)
CPtrList CPtrListPool<GruntRec>::s_freeList(0xa);

DATA(0x0024b6a0)
char g_idScratch[0x10];

DATA(0x0024b6b0)
char g_idListBuf[0x40];

RVA(0x000bef80, 0x51)
i32 CNetSession::Init(CGruntzMgr* mgr, CMulti* owner, CNetMgr* netMgr) {
    if (mgr == 0) {
        return 0;
    }
    if (owner == 0) {
        return 0;
    }
    if (netMgr == 0) {
        return 0;
    }
    m_mgr = mgr;
    m_session = owner;
    m_netMgr = netMgr;
    Reset();
    m_period = owner->m_commandDelay;
    return 1;
}

// @early-stop
RVA(0x000bf000, 0xd5)
void CNetSession::ResetSync() {
    m_mgr = 0;
    m_session = 0;
    m_netMgr = 0;
    m_localDesc = 0;
    m_tick = 0;
    m_snapshotDone = 0;
    m_seq = 0;
    m_period = 1;
    CNetCmdSlot* s = m_slots;
    i32 n = 4;
    do {
        s->m_isRemote = 0;
        s->m_latchedSeq = 0;
        s->m_state = 0;
        s->m_desc = 0;
        s->m_latency = 0;
        s->m_baseSeq = 0;
        s->m_maxSeq = 0;
        s->m_owner = 0;
        s->ClearCmds();
        s->ClearAckFlags();
        s->ResetTriple(s->m_rangeA);
        s->ResetTriple(s->m_rangeB);
        s++;
    } while (--n);
    for (i32 j = 0; j < 0x80; j++) {
        m_idMap[j] = 0;
    }
    GruntRec* r = m_records;
    i32 k = 0x80;
    do {
        r->m_seq = 0;
        r->m_count = 0;
        r->m_payloadLen = 0;
        r->m_checksum = 0;
        r++;
    } while (--k);
    CPtrList& freeList = CPtrListPool<GruntRec>::s_freeList;
    while (freeList.GetCount() != 0) {
        void* p = freeList.RemoveTail();
        if (p) {
            ::operator delete(p);
        }
    }
}

RVA(0x000bf120, 0x11)
void CNetCmdSlot::ClearAckFlags() {
    for (i32 i = 0; i < 4; i++) {
        m_ackFlags[i] = 0;
    }
}

RVA(0x000bf150, 0x58)
void CNetSession::Reset() {
    m_tick = 0;
    m_snapshotDone = 0;
    m_seq = 0;
    i32 i;
    for (i = 0; i < 4; i++) {
        m_slots[i].FullReset();
    }

    for (i = 0; i < 0x80; i++) {
        m_idMap[i] = 0;
    }
    for (i = 0; i < 0x80; i++) {
        m_records[i].m_seq = 0;
        m_records[i].m_count = 0;
        m_records[i].m_payloadLen = 0;
        m_records[i].m_checksum = 0;
    }
}

RVA(0x000bf580, 0x10)
void RecycleCmd(void* cmd) {
    CPtrListPool<GruntRec>::s_freeList.AddTail(cmd);
}

// @early-stop
RVA(0x000bf5a0, 0x110)
i32 CNetSession::Poll(i32 delta) {
    CNetCmdSlot* s = m_slots;
    i32 n = 4;
    do {
        if (s->m_state == 3) {
            s->m_latency += delta;
        }
        s++;
    } while (--n);

    i32 avail;
    if (m_localDesc == 0) {
        avail = 0;
    } else {
        i32 got;

        IDirectPlay4Z* ep = m_netMgr->m_directPlay;
        i32 r = ep->GetMessageCount(m_localDesc->m_id, &got);
        avail = (r == 0) ? got : 0;
    }

    i32 a = 0;
    i32 received = 0;
    while (avail > 0 && m_session->m_pollAbort == 0) {
        i32 len = 0x800;
        i32 chan = m_localDesc->m_id;
        IDirectPlay4Z* ep = m_netMgr->m_directPlay;
        i32 st = ep->Receive(&a, &chan, 1, g_lobbyRecvBuf, &len);
        if (st != 0) {
            CNetMgr::ReportError(const_cast<char*>("c:\\proj\\incs\\netmgr.h"), 0x141, st, 0);
            if (st != 0) {
                break;
            }
        }
        received++;
        avail--;
        if (a != m_localDesc->m_id) {

            CNetWireMsg wire;
            wire.m_bytes = g_lobbyRecvBuf;
            Dispatch(a, wire.m_ctrl, len);
        }
    }
    return received;
}

RVA(0x000bf700, 0x82)
i32 CNetSession::Dispatch(i32 a, CNetCtrlMsg* b, i32 c) {
    if (!b) {
        return 0;
    }
    if (a == 0) {
        return DispatchMsg(b, c);
    }
    CNetCmdSlot* obj = FindCmdSlot(a);
    if (!obj) {
        return 0;
    }
    obj->m_latency = 0;

    if (!(b->m_route.m_routeFlags & 0x80) && (b->m_route.m_routeFlags & 1)) {
        obj = &m_slots[b->m_route.m_routeSlot];
        if (!obj) {
            return 0;
        }
    }
    return obj->ProcessCmd(a, b, c);
}

RVA(0x000bf7c0, 0x1b0)
i32 CNetSession::DispatchMsg(CNetCtrlMsg* m, i32 ctrlArg) {
    if (!m) {
        return 0;
    }
    switch (m->m_code) {
        case 3:
            m_session->LoadMenuSelectSprite(static_cast<void*>(m));
            return 1;
        case 5:
            if (m->m_subCode == 1) {
                i32 playerId = m->m_playerId;
                m_session->OnPlayerLeft(playerId);
                m_session->ResetPlayerCommands(playerId);
                return 1;
            }
            return 1;
        case 49:
            return m_session->HandleControlMsg(m, ctrlArg);
        case 257:
            return m_session->HandleControlMsg(m, ctrlArg);
        default:
            return 1;
    }
}

// @early-stop
RVA(0x000bf9e0, 0xfe)
i32 CNetSession::Tick() {
    if (m_snapshotDone == 0 && (m_tick + 1) % m_period == 0) {
        i32 seq = m_seq + 2;
        GruntRec* rec = &m_records[seq % 0x80];
        rec->m_seq = seq;
        rec->m_payloadLen = 0;
        rec->m_count = 0;
        rec->m_checksum = Checksum();
        char* payload = rec->m_payload;
        i32 next = seq + 1;
        for (i32 t = seq * m_period; t < next * m_period; t++) {
            CGruntzCommand* obj = GetSlotPtr(t);
            if (obj) {
                NoopSync(obj);
                rec->m_count++;

                RecordBytes<GruntRec> rb;
                rb.m_rec = rec;
                payload += obj->Pack(payload, rb.m_chars - payload + 0x410);
            }
        }
        m_session->WriteTag("[end]\n");
        RecordBytes<GruntRec> rb2;
        rb2.m_rec = rec;
        rec->m_payloadLen = static_cast<i32>((payload - rb2.m_chars - 0x10));
        m_snapshotDone = 1;
    }
    return SendBatch() + SendAll();
}

RVA(0x000bfb20, 0x1)
void NoopSync(CGruntzCommand*) {}

RVA(0x000bfb40, 0xe2)
i32 CNetSession::SendAll() {
    i32 count = 0;
    CNetCmdSlot* outer = m_slots;
    for (i32 oi = 0; oi < 4; oi++, outer++) {
        if (outer && outer->m_state == 3 && outer->m_isRemote != 0) {
            i32 lo, hi;
            outer->GetRange(&lo, &hi);
            CNetCmdSlot* inner = m_slots;
            i32 in = 4;
            do {
                if (inner && inner->m_state == 3 && inner->m_isRemote == 0) {
                    for (i32 v = lo; v <= hi; v++) {
                        GruntRec* r = outer->FindCmd(v);
                        if (r) {

                            u8 flag = 1;
                            if (v == hi) {
                                flag = 3;
                            }
                            if (SendGruntRecord(v, r, flag, oi, inner->m_desc->m_slotKey)) {
                                count++;
                            }
                        }
                    }
                }
                inner++;
            } while (--in);
        }
    }
    return count;
}

// @early-stop
RVA(0x000bfc70, 0x9c)
i32 CNetSession::SendGruntRecord(i32 seq, GruntRec* rec, u8 flag, i32 slot, i32 dpTo) {
    if (!rec) {
        return 0;
    }
    if (seq < 0) {
        return 1;
    }
    g_netGruntRecMsg.m_seq = seq;
    g_netGruntRecMsg.m_flags = flag;
    g_netGruntRecMsg.m_slot = static_cast<unsigned char>(slot);
    g_netGruntRecMsg.m_checksum = rec->m_checksum;
    g_netGruntRecMsg.m_count = rec->m_count;
    memcpy(g_netGruntRecMsg.m_payload, rec->m_payload, rec->m_payloadLen);

    return m_netMgr->SetData(
               m_localDesc->m_id,
               dpTo,
               0,
               &g_netGruntRecMsg,
               rec->m_payloadLen + offsetof(NetGruntRecMsg, m_payload)
           )
           == 0;
}

RVA(0x000bfd40, 0x116)
i32 CNetSession::SendBatch() {
    i32 count = 0;
    CNetCmdSlot* s = m_slots;
    i32 n = 4;
    do {
        if (s && s->m_state == 3 && s->m_isRemote == 0) {
            i32 t = m_seq + 2;
            if (m_snapshotDone == 0 && (m_tick + 1) % m_period == 0) {
                if (SendOne(s, t)) {
                    count++;
                }
            }
            i32 v = m_seq + 1;
            if (s->m_maxSeq < v && s->NetCmdIdFind(s->m_rangeB, v) == 0) {
                if (SendOne(s, v)) {
                    count++;
                }
            }
            v = m_seq;
            if (s->m_maxSeq < v && s->NetCmdIdFind(s->m_rangeB, v) == 0) {
                if (SendOne(s, v)) {
                    count++;
                }
            }
            v = m_seq - 1;
            if (s->m_maxSeq < v && s->NetCmdIdFind(s->m_rangeB, v) == 0) {
                if (SendOne(s, v)) {
                    count++;
                }
            }
            v = m_seq - 2;
            if (s->m_maxSeq < v && s->NetCmdIdFind(s->m_rangeB, v) == 0) {
                if (SendOne(s, v)) {
                    count++;
                }
            }
        }
        s++;
    } while (--n);
    return count;
}

// @early-stop
RVA(0x000bfeb0, 0xfa)
i32 CNetSession::SendOne(CNetCmdSlot* slot, i32 val) {
    if (!slot) {
        return 0;
    }
    if (val < 0) {
        return 1;
    }
    unsigned char flags = 0;
    i32 baseSeq = slot->m_baseSeq;
    if (slot->NetCmdIdFind(slot->m_rangeA, baseSeq + 2)) {
        flags = 0x10;
    }
    if (slot->NetCmdIdFind(slot->m_rangeA, baseSeq + 3)) {
        flags |= 0x20;
    }
    g_netCmdSendMsg.m_flags = flags;
    g_netCmdSendMsg.m_val = val;
    i32 idx = val % 0x80;
    GruntRec* entry = &m_records[idx];
    g_netCmdSendMsg.m_baseSeq = slot->m_baseSeq;
    g_netCmdSendMsg.m_checksum = entry->m_checksum;
    g_netCmdSendMsg.m_count = entry->m_count;
    memcpy(g_netCmdSendMsg.m_payload, entry->m_payload, entry->m_payloadLen);

    return m_netMgr->SetData(
               m_localDesc->m_id,
               slot->m_desc->m_slotKey,
               0,
               &g_netCmdSendMsg,
               entry->m_payloadLen + offsetof(NetCmdSendMsg, m_payload)
           )
           == 0;
}

RVA(0x000bfff0, 0x5d)
CNetCmdSlot* CNetSession::CreateSlot(i32 index, i32 owner) {
    if (index < 0 || index >= 4) {
        return 0;
    }
    CNetCmdSlot* slot = &m_slots[index];
    if (slot == 0) {
        return 0;
    }
    (static_cast<CNetCmdSlot*>(slot))->ResetAll();
    return slot->Init(m_session, &m_mgr->m_options[index], owner) ? slot : 0;
}

RVA(0x000c0070, 0x15)
void CNetSession::ResetCmdBuffers() {
    for (i32 i = 0; i < 4; i++) {
        m_slots[i].m_latency = 0;
    }
}

RVA(0x000c00a0, 0x31)
CNetCmdSlot* CNetSession::FindCmdSlot(i32 playerId) {
    for (i32 i = 0; i < 4; i++) {
        if (m_slots[i].m_desc->m_slotKey == playerId) {
            return &m_slots[i];
        }
    }
    return 0;
}

RVA(0x000c00f0, 0xaf)
void CNetSession::Reconcile() {
    i32 withFlag = 0;
    i32 withoutFlag = 0;
    CNetCmdSlot* base = m_slots;
    {
        CNetCmdSlot* s = base;
        i32 n = 4;
        do {
            if (s) {
                i32 type = s->m_state;
                if (type == 3 && s->m_isRemote != 0) {
                    withFlag++;
                }
                if (type == 3 && s->m_isRemote == 0) {
                    withoutFlag++;
                }
            }
            s++;
        } while (--n);
    }
    if (withoutFlag == 0) {
        CNetCmdSlot* s = base;
        i32 n = 4;
        do {
            if (s && s->m_state == 3) {
                s->FullReset();
                GruntzPlayer* p = s->m_desc;
                s->m_state = 1;
                p->m_doneFlag = 1;
            }
            s++;
        } while (--n);
    } else if (withFlag != 0) {
        CNetCmdSlot* s = base;
        i32 n = 4;
        do {
            if (s && s->m_state == 3 && s->m_isRemote != 0 && m_seq > s->m_latchedSeq + 2) {
                s->FullReset();
                GruntzPlayer* p = s->m_desc;
                s->m_state = 1;
                p->m_doneFlag = 1;
            }
            s++;
        } while (--n);
    }
}

RVA(0x000c01d0, 0x8c)
i32 CNetSession::Advance() {
    i32 nextTick = m_tick + 1;
    i32 nextSeq = m_seq + 1;
    if (nextTick % m_period != 0) {
        m_tick = nextTick;
        return 1;
    }
    Reconcile();
    if (!Verify(nextSeq)) {
        return 0;
    }
    CNetCmdSlot* s = m_slots;
    i32 n = 4;
    do {
        if (s && s->m_state == 3 && s->m_isRemote == 0) {
            s->RemoveCmd(m_seq - 4);
        }
        s++;
    } while (--n);
    m_tick = nextTick;
    m_seq = nextSeq;
    m_snapshotDone = 0;
    return 1;
}

RVA(0x000c0290, 0x63)
i32 CNetSession::Verify(i32 n) {
    for (i32 i = 0; i < 4; i++) {
        CNetCmdSlot* s = &m_slots[i];
        if (s != 0) {
            if (s->m_state == 3 && s->m_isRemote == 0) {
                if (s->m_baseSeq < n) {
                    return 0;
                }
            } else if (s->m_state == 3 && s->m_isRemote != 0) {
                if (s->Ready() == 0) {
                    return 0;
                }
                if (s->m_latchedSeq != s->m_baseSeq) {
                    return 0;
                }
            }
        }
    }
    return 1;
}

RVA(0x000c0320, 0x37)
i32 CNetSession::AllSlotsReachedSeq(i32 seq) {
    for (i32 i = 0; i < 4; i++) {
        CNetCmdSlot* slot = &m_slots[i];
        if (slot != 0 && slot->m_state == 3 && slot->m_isRemote == 0 && slot->m_maxSeq < seq) {
            return 0;
        }
    }
    return 1;
}

RVA(0x000c0370, 0x28)
void CNetSession::AdvanceAllSlots(i32 id) {
    CNetCmdSlot* slot = m_slots;
    for (i32 i = 4; i != 0; i--) {
        if (slot->m_state == 3) {
            slot->AdvanceSeq(id);
        }
        slot++;
    }
}

RVA(0x000c03b0, 0x28)
void CNetSession::RaiseAllSlotsMax(i32 v) {
    CNetCmdSlot* slot = m_slots;
    for (i32 i = 4; i != 0; i--) {
        if (slot->m_state == 3) {
            slot->RaiseMax(v);
        }
        slot++;
    }
}

RVA(0x000c03f0, 0x29)
void CNetSession::ArmSlot(void* node, u8 parity) {
    m_idMap[(m_tick + parity) % 128] = static_cast<CGruntzCommand*>(node);
}

RVA(0x000c0430, 0x1f)
CGruntzCommand* CNetSession::GetSlotPtr(i32 v) {
    return m_idMap[(v & 0xff) % 128];
}

RVA(0x000c0460, 0x2e)
CNetCmdSlot* CNetSession::FindSlot(u32 key) {

    for (i32 i = 0; i < 4; i++) {
        CNetCmdSlot* p = &m_slots[i];
        if (p && p->m_state == 3 && p->m_isRemote == 0 && static_cast<u32>(p->m_latency) > key) {
            return p;
        }
    }
    return 0;
}

RVA(0x000c04a0, 0x37)
i32 CNetSession::CheckLatency(i32 cap) {
    for (i32 i = 0; i < 4; i++) {
        CNetCmdSlot* slot = &m_slots[i];
        if (slot != 0 && slot->m_state == 3 && slot->m_isRemote == 0
            && static_cast<u32>(slot->m_latency) > static_cast<u32>(cap)) {
            return 0;
        }
    }
    return 1;
}

RVA(0x000c04f0, 0x7c)
i32 CNetSession::Verify() {
    i32 seq = m_seq - 2;
    GruntRec* e = &m_records[seq % 128];
    if (e != 0) {
        for (i32 i = 0; i < 4; i++) {
            CNetCmdSlot* slot = &m_slots[i];
            if (slot != 0 && slot->m_state == 3 && slot->m_isRemote == 0) {
                GruntRec* c = slot->FindCmd(seq);
                if (c != 0 && c->m_checksum != e->m_checksum) {
                    return 0;
                }
            }
        }
    }
    return 1;
}

// @early-stop
RVA(0x000c0b10, 0x72)
i32 CNetCmdSlot::Init(CMulti* owner, GruntzPlayer* desc, i32 state) {
    if (desc == 0) {
        return 0;
    }
    if (owner == 0) {
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

    for (i32 i = 0; i < 4; i++) {
        m_ackFlags[i] = 0;
    }
    ResetTriple(m_rangeA);
    ResetTriple(m_rangeB);
    return 1;
}

RVA(0x000c0bb0, 0x47)
void CNetCmdSlot::ResetAll() {
    m_state = 0;
    m_isRemote = 0;
    m_latchedSeq = 0;
    m_desc = 0;
    m_latency = 0;
    m_baseSeq = 0;
    m_maxSeq = 0;
    m_owner = 0;
    ClearCmds();

    for (i32 i = 0; i < 4; i++) {
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

    for (i32 i = 0; i < 4; i++) {
        m_ackFlags[i] = 0;
    }
    ResetTriple(m_rangeA);
    ResetTriple(m_rangeB);
}

// @early-stop
RVA(0x000c0c70, 0x20f)
i32 CNetCmdSlot::ProcessCmd(i32 playerId, void* rec, i32 size) {
    if (rec == 0) {
        return 0;
    }
    u8 opcode = *static_cast<u8*>(rec);
    i32 odd = opcode & 1;
    char* p = static_cast<char*>(rec) + 1;
    if (m_state != 3) {
        return 1;
    }
    if (opcode & 0x80) {
        return m_owner->DispatchRecvMsg(m_desc->m_slotKey, static_cast<char*>(rec), size);
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
        if (slot == 0) {
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

    GruntRec* pkt = static_cast<GruntRec*>(AllocateGruntRecord(0));
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
        obj->m_submitted = 1;

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
    if (cmd != 0 && FindCmd(cmd->m_seq) == 0) {
        m_cmds.AddTail(cmd);
    }
}

RVA(0x000c11b0, 0x55)
void CNetCmdSlot::RemoveCmd(i32 seq) {
    POSITION pos = m_cmds.GetHeadPosition();
    while (pos != 0) {
        GruntRec* cmd = static_cast<GruntRec*>(m_cmds.GetNext(pos));
        if (seq == cmd->m_seq) {
            if (pos != 0) {

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
    if (pMin == 0) {
        return;
    }
    if (pMax == 0) {
        return;
    }
    *pMax = 0x80000001;
    *pMin = 0x7fffffff;
    POSITION pos = m_cmds.GetHeadPosition();
    if (pos == 0) {
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
    } while (pos != 0);
}

RVA(0x000c12b0, 0x1f)
GruntRec* CNetCmdSlot::FindCmd(i32 seq) {
    POSITION pos = m_cmds.GetHeadPosition();
    while (pos != 0) {
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
        if (cmd != 0) {
            RecycleCmd(cmd);
        }
    }
}

// @early-stop
RVA(0x000c1320, 0x4a)
i32 CNetCmdSlot::Ready() {
    CMulti* mgr = m_owner;
    if (mgr == 0) {
        return 0;
    }
    CNetSession* sess = mgr->m_session;
    for (i32 i = 0; i < 4; i++) {
        CNetCmdSlot* slot = &sess->m_slots[i];
        if (slot != 0 && slot->m_state == 3 && slot->m_isRemote == 0 && m_ackFlags[i] == 0) {
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
