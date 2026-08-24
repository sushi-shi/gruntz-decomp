#include <rva.h>

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
i32 CNetSession::Init(CGruntzMgr* mgr, CMulti* owner, CNetMgr* netMgr) {
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
    m_session = owner;
    m_netMgr = netMgr;
    Reset();
    m_period = owner->m_commandDelay;
    return 1;
}

// @early-stop
RVA(0x000bf000, 0xd5)
void CNetSession::ResetSync() {
    m_mgr = NULL;
    m_session = NULL;
    m_netMgr = NULL;
    m_localDesc = NULL;
    m_tick = 0;
    m_snapshotDone = 0;
    m_seq = 0;
    m_period = 1;
    for (i32 i = 0; i < 4; i++) {
        m_slots[i].m_isRemote = 0;
        m_slots[i].m_latchedSeq = 0;
        m_slots[i].m_state = NETSLOT_EMPTY;
        m_slots[i].m_desc = NULL;
        m_slots[i].m_latency = 0;
        m_slots[i].m_baseSeq = 0;
        m_slots[i].m_maxSeq = 0;
        m_slots[i].m_owner = NULL;
        m_slots[i].ClearCmds();
        m_slots[i].ClearAckFlags();
        m_slots[i].ResetTriple(m_slots[i].m_rangeA);
        m_slots[i].ResetTriple(m_slots[i].m_rangeB);
    }
    for (i32 j = 0; j < 0x80; j++) {
        m_idMap[j] = NULL;
    }
    for (i32 k = 0; k < 0x80; k++) {
        m_records[k].m_seq = 0;
        m_records[k].m_count = 0;
        m_records[k].m_payloadLen = 0;
        m_records[k].m_checksum = 0;
    }
    CPtrList& freeList = CPtrListPool<GruntRec>::s_freeList;
    while (freeList.GetCount() != 0) {
        GruntRec* p = static_cast<GruntRec*>(freeList.RemoveTail());
        if (p) {
            ::operator delete(p);
        }
    }
}

RVA(0x000bf120, 0x11)
void CNetCmdSlot::ClearAckFlags() {
    for (i32 i = 0; i < NET_SLOT_COUNT; i++) {
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
        m_idMap[i] = NULL;
    }
    for (i = 0; i < 0x80; i++) {
        m_records[i].m_seq = 0;
        m_records[i].m_count = 0;
        m_records[i].m_payloadLen = 0;
        m_records[i].m_checksum = 0;
    }
}

// @early-stop
// Keep case bodies in retail's jump-table target order: wp 2 through 0x16,
// followed by the default 0x17 arm. The remaining frame delta is two loop-IV spills.
RVA(0x000bf1d0, 0x2a4)
void CNetSession::BuildGruntzCrcInfo() {
    char szLine[0x100];
    szLine[0] = ""[0];
    memset(szLine + 1, 0, sizeof(szLine) - 1);

    CString info("crc info for all gruntz:\n------------------------\n");

    for (i32 player = 0; player < 4; player++) {
        for (i32 g = 0; g < 0xf; g++) {

            CGrunt* grunt = m_session->Mgr()->m_cmdGrid->m_units[player * 0xf + g];
            if (grunt == NULL) {
                continue;
            }
            i32 rnd = rand();
            PickupType type = grunt->m_entranceReason;
            i32 wp;
            switch (type) {
                case PICKUP_BOMB:
                    wp = 2;
                    break;
                case PICKUP_WELDER:
                    wp = 3;
                    break;
                case PICKUP_SWORD:
                    wp = 4;
                    break;
                case PICKUP_GUNHAT:
                    wp = 5;
                    break;
                case PICKUP_CLUB:
                    wp = 6;
                    break;
                case PICKUP_ROCK:
                    wp = 7;
                    break;
                case PICKUP_SHOVEL:
                    wp = 8;
                    break;
                case PICKUP_BOOMERANG:
                    wp = 9;
                    break;
                case PICKUP_SPRING:
                    wp = 0xa;
                    break;
                case PICKUP_GAUNTLETZ:
                    wp = 0xb;
                    break;
                case PICKUP_WINGZ:
                    wp = 0xc;
                    break;
                case PICKUP_SPY:
                    wp = 0xd;
                    break;
                case PICKUP_BRICK:
                    wp = 0xe;
                    break;
                case PICKUP_GRAVITYBOOTZ:
                    wp = 0xf;
                    break;
                case PICKUP_SHIELD:
                    wp = 0x10;
                    break;
                case PICKUP_GOOBER:
                    wp = 0x11;
                    break;
                case PICKUP_TOOB:
                    wp = 0x12;
                    break;
                case PICKUP_GLOVEZ:
                    wp = 0x13;
                    break;
                case PICKUP_TIMEBOMB:
                    wp = 0x14;
                    break;
                case PICKUP_NERFGUN:
                    wp = 0x15;
                    break;
                case PICKUP_WAND:
                    wp = 0x16;
                    break;
                default:
                    wp = 0x17;
                    break;
            }
            PickupType tool = type;
            if (type > PICKUP_EQUIPPABLE_LAST) {
                tool = grunt->m_toolId;
            }
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
                grunt->m_vehiclePickupType,
                grunt->m_daFlag,
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
    m_session->ReportVersionMsg(const_cast<char*>(static_cast<const char*>(info)), 0);
}

RVA(0x000bf530, 0x3b)
GruntRec* AllocateGruntRecord(int bClear) {
    CPtrList& freeList = CPtrListPool<GruntRec>::s_freeList;
    if (freeList.GetCount()) {
        GruntRec* p = static_cast<GruntRec*>(freeList.RemoveTail());
        if (bClear) {
            memset(p, 0, sizeof(GruntRec));
        }
        return p;
    }
    return new GruntRec;
}

RVA(0x000bf580, 0x10)
void RecycleCmd(GruntRec* cmd) {
    CPtrListPool<GruntRec>::s_freeList.AddTail(cmd);
}

RVA(0x000bf5a0, 0x110)
i32 CNetSession::Poll(i32 delta) {
    for (i32 i = 0; i < 4; i++) {
        if (m_slots[i].m_state == NETSLOT_ACTIVE) {
            m_slots[i].m_latency += delta;
        }
    }

    i32 st = 0;
    i32 avail;
    CNetSessionNode* localDesc = m_localDesc;
    CNetMgr* netMgr = m_netMgr;
    if (localDesc == NULL) {
        avail = 0;
    } else {
        i32 got;

        IDirectPlay4Z* ep = netMgr->m_directPlay;
        i32 r = ep->GetMessageCount(localDesc->m_id, &got);
        avail = (r == 0) ? got : 0;
    }

    i32 a = 0;
    i32 len = 0x800;
    i32 received = 0;
    while (st == 0 && avail > 0 && m_session->m_pollAbort == 0) {
        len = 0x800;
        IDirectPlay4Z* ep = m_netMgr->m_directPlay;
        i32 chan = m_localDesc->m_id;
        st = ep->Receive(&a, &chan, 1, g_lobbyRecvBuf, &len);
        if (st != 0) {
            CNetMgr::ReportError(const_cast<char*>("c:\\proj\\incs\\netmgr.h"), 0x141, st, NULL);
        }
        if (st == 0) {
            avail--;
            received++;
            if (a != m_localDesc->m_id) {

                CNetWireMsg wire;
                wire.m_bytes = g_lobbyRecvBuf;
                Dispatch(a, wire.m_ctrl, len);
            }
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
    CNetWireMsg wire;
    wire.m_ctrl = b;
    return obj->ProcessCmd(a, wire.m_bytes, c);
}

RVA(0x000bf7c0, 0x1b0)
i32 CNetSession::DispatchMsg(CNetCtrlMsg* m, i32 ctrlArg) {
    if (!m) {
        return 0;
    }
    switch (m->m_code) {
        case DPSYS_CREATEPLAYERORGROUP: {
            CNetWireMsg wire;
            wire.m_ctrl = m;
            m_session->LoadMenuSelectSprite(wire.m_menuSelect);
            return 1;
        }
        case DPSYS_DESTROYPLAYERORGROUP:
            if (m->m_subCode == 1) {
                i32 playerId = m->m_playerId;
                m_session->OnPlayerLeft(playerId);
                m_session->ResetPlayerCommands(playerId);
                return 1;
            }
            return 1;
        case DPSYS_SESSIONLOST:
            return m_session->HandleControlMsg(m, ctrlArg);
        case DPSYS_HOST:
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
        i32 t = seq * m_period;
        seq = seq + 1;
        for (; t < seq * m_period; t++) {
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
    i32 r = SendBatch();
    r += SendAll();
    return r;
}

RVA(0x000bfb20, 0x1)
void NoopSync(CGruntzCommand*) {}

RVA(0x000bfb40, 0xe2)
i32 CNetSession::SendAll() {
    i32 count = 0;
    CNetCmdSlot* outer = m_slots;
    for (i32 oi = 0; oi < 4; oi++, outer++) {
        if (outer && outer->m_state == NETSLOT_ACTIVE && outer->m_isRemote != 0) {
            i32 lo, hi;
            outer->GetRange(&lo, &hi);
            CNetCmdSlot* inner = m_slots;
            i32 in = 4;
            do {
                if (inner && inner->m_state == NETSLOT_ACTIVE && inner->m_isRemote == 0) {
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

    i32 r = m_netMgr->SetData(
        m_localDesc->m_id,
        dpTo,
        0,
        &g_netGruntRecMsg,
        rec->m_payloadLen + offsetof(NetGruntRecMsg, m_payload)
    );
    return r == 0;
}

RVA(0x000bfd40, 0x116)
i32 CNetSession::SendBatch() {
    i32 count = 0;
    CNetCmdSlot* s = m_slots;
    i32 n = 4;
    do {
        if (s && s->m_state == NETSLOT_ACTIVE && s->m_isRemote == 0) {
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

    i32 status = m_netMgr->SetData(
        m_localDesc->m_id,
        slot->m_desc->m_slotKey,
        0,
        &g_netCmdSendMsg,
        entry->m_payloadLen + offsetof(NetCmdSendMsg, m_payload)
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
    (static_cast<CNetCmdSlot*>(slot))->ResetAll();
    return slot->Init(m_session, &m_mgr->m_options[index], state) ? slot : NULL;
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
    return NULL;
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
                NetSlotState type = s->m_state;
                if (type == NETSLOT_ACTIVE && s->m_isRemote != 0) {
                    withFlag++;
                }
                if (type == NETSLOT_ACTIVE && s->m_isRemote == 0) {
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
            if (s && s->m_state == NETSLOT_ACTIVE) {
                s->FullReset();
                GruntzPlayer* p = s->m_desc;
                s->m_state = NETSLOT_DONE;
                p->m_doneFlag = 1;
            }
            s++;
        } while (--n);
    } else if (withFlag != 0) {
        CNetCmdSlot* s = base;
        i32 n = 4;
        do {
            if (s && s->m_state == NETSLOT_ACTIVE && s->m_isRemote != 0
                && m_seq > s->m_latchedSeq + 2) {
                s->FullReset();
                GruntzPlayer* p = s->m_desc;
                s->m_state = NETSLOT_DONE;
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
        if (s && s->m_state == NETSLOT_ACTIVE && s->m_isRemote == 0) {
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
        if (s != NULL) {
            if (s->m_state == NETSLOT_ACTIVE && s->m_isRemote == 0) {
                if (s->m_baseSeq < n) {
                    return 0;
                }
            } else if (s->m_state == NETSLOT_ACTIVE && s->m_isRemote != 0) {
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

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000c0320, 0x37)
i32 CNetSession::AllSlotsReachedSeq(i32 seq) {
    for (i32 i = 0; i < 4; i++) {
        CNetCmdSlot* slot = &m_slots[i];
        if (slot != NULL && slot->m_state == NETSLOT_ACTIVE && slot->m_isRemote == 0
            && slot->m_maxSeq < seq) {
            return 0;
        }
    }
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000c0370, 0x28)
void CNetSession::AdvanceAllSlots(i32 id) {
    CNetCmdSlot* slot = m_slots;
    for (i32 i = 4; i != 0; i--) {
        if (slot->m_state == NETSLOT_ACTIVE) {
            slot->AdvanceSeq(id);
        }
        slot++;
    }
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000c03b0, 0x28)
void CNetSession::RaiseAllSlotsMax(i32 v) {
    CNetCmdSlot* slot = m_slots;
    for (i32 i = 4; i != 0; i--) {
        if (slot->m_state == NETSLOT_ACTIVE) {
            slot->RaiseMax(v);
        }
        slot++;
    }
}

RVA(0x000c03f0, 0x29)
void CNetSession::ArmSlot(CGruntzCommand* node, u8 parity) {
    m_idMap[(m_tick + parity) % 128] = node;
}

RVA(0x000c0430, 0x1f)
CGruntzCommand* CNetSession::GetSlotPtr(i32 v) {
    return m_idMap[(v & 0xff) % 128];
}

RVA(0x000c0460, 0x2e)
CNetCmdSlot* CNetSession::FindSlot(u32 key) {

    for (i32 i = 0; i < 4; i++) {
        CNetCmdSlot* p = &m_slots[i];
        if (p && p->m_state == NETSLOT_ACTIVE && p->m_isRemote == 0
            && static_cast<u32>(p->m_latency) > key) {
            return p;
        }
    }
    return NULL;
}

RVA(0x000c04a0, 0x37)
i32 CNetSession::CheckLatency(i32 cap) {
    for (i32 i = 0; i < 4; i++) {
        CNetCmdSlot* slot = &m_slots[i];
        if (slot != NULL && slot->m_state == NETSLOT_ACTIVE && slot->m_isRemote == 0
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
    if (e != NULL) {
        for (i32 i = 0; i < 4; i++) {
            CNetCmdSlot* slot = &m_slots[i];
            if (slot != NULL && slot->m_state == NETSLOT_ACTIVE && slot->m_isRemote == 0) {
                GruntRec* c = slot->FindCmd(seq);
                if (c != NULL && c->m_checksum != e->m_checksum) {
                    return 0;
                }
            }
        }
    }
    return 1;
}

// @early-stop
RVA(0x000c0590, 0x21c)
i32 CNetSession::Checksum() {
    i32 sum = 0;
    i32 idx = 0;
    do {
        i32 count = 15;
        do {
            CGrunt* grunt = static_cast<CGrunt*>(m_session->m_mgr->m_cmdGrid->m_units[idx]);
            if (grunt != NULL) {
                sum += IDX(grunt->m_entranceCell.direction) + grunt->m_stamina + grunt->m_toyTime
                       + grunt->m_health + grunt->m_object->m_screenY + grunt->m_object->m_sortKey
                       + grunt->m_object->m_screenX + grunt->LastTilePx().m_x
                       + grunt->LastTilePx().m_y;

                PickupType carried = grunt->m_entranceReason;
                PickupType effective = carried;
                if (carried > PICKUP_EQUIPPABLE_LAST) {
                    effective = grunt->m_toolId;
                }
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
                       + grunt->m_neighborValid + grunt->m_poweredUp + g_frameTime + IDX(next);
                sum += rand();
            }
            idx++;
        } while (--count);
    } while (idx < 0x3c);
    return sum;
}
