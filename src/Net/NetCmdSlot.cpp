#include <Net/CmdPool.h>
#include <Net/NetMgr.h>   // canonical CNetSession / CNetCmdSlot / CPtrList / CObject
#include <Gruntz/Multi.h> // CMulti - the real owner of the LoadMenuSelectSprite/OnPlayerLeft/... game-mgr methods (netmgr-vs-cmulti split); Init a2 is a CMulti
#include <Gruntz/GruntzMgr.h> // CGruntzMgr - CMulti::m_4's real type (its +0x6c m_cmdSubMgr is the CGruntzCmdMgr command manager)
#include <Gruntz/GruntzCmdMgr.h>  // CGruntzCmdMgr::EnqueueCommand (the +0x6c command manager)
#include <Gruntz/GruntzCommand.h> // canonical CGruntzCommand/Single/Multi (slot-7 Parse)
#include <Ints.h>
#include <Rez/RezMgr.h>
#include <dplay.h> // real DirectPlay: CNetMgr::m_endpoint (+0x18) is IDirectPlay4
#include <rva.h>
#include <string.h>         // memcpy / memset / strcat (see #pragma intrinsic below)
#include <Net/NetCmdSlot.h> // own exported globals (ex Globals.h)

#pragma intrinsic(memcpy)
#pragma intrinsic(strcat)

char g_lobbyRecvBuf[0x800]; // 0x249858
DATA(0x0024a058)
unsigned char gB_flag; // 0x24a058
DATA(0x0024a059)
i32 gB_val; // 0x24a059
DATA(0x0024a05d)
i32 gB_m14; // 0x24a05d
DATA(0x0024a061)
i32 gB_e04; // 0x24a061
DATA(0x0024a065)
unsigned char gB_e08; // 0x24a065
DATA(0x0024a066)
unsigned char gB_data; // 0x24a066
DATA(0x0024a8a8)
unsigned char gA_flag; // 0x24a8a8
DATA(0x0024a8a9)
unsigned char gA_slot; // 0x24a8a9
DATA(0x0024a8aa)
i32 gA_seq; // 0x24a8aa
DATA(0x0024a8b2)
i32 gA_e04; // 0x24a8b2
DATA(0x0024a8b6)
unsigned char gA_e08; // 0x24a8b6
DATA(0x0024a8b7)
unsigned char gA_data; // 0x24a8b7

template<> DATA(0x0024aca8)
CPtrList CPtrListPool<CNetCmdPacket>::s_freeList(0xa);

DATA(0x0024b6a0)
char g_idScratch[0x10]; // 0x24b6a0

DATA(0x0024b6b0)
char g_idListBuf[0x40]; // 0x24b6b0

void NoopSync(CGruntzCommand* p); // 0xbfb20 (empty)

RVA_COMPGEN(0x000beef0, 0xa, _$E782064)
RVA_COMPGEN(0x000bef10, 0xd, _$E782096)
RVA_COMPGEN(0x000bef30, 0xe, _$E782128)
RVA_COMPGEN(0x000bef50, 0x1f, _$E782160)

RVA(0x000bef80, 0x51)
i32 CNetSession::Init(void* a1, CMulti* a2, void* a3) {
    if (a1 == 0) {
        return 0;
    }
    if (a2 == 0) {
        return 0;
    }
    if (a3 == 0) {
        return 0;
    }
    m_0 = static_cast<CNetCmdBuf*>(a1);
    m_session = a2; // the owning CMulti (kept as the +0x4 handle CreateSlot re-passes)
    m_netMgr = static_cast<CNetMgr*>(a3);
    Reset();
    m_period = a2->m_5a4;
    return 1;
}

// @early-stop
// regalloc/scheduling wall (~85%): instruction sequence byte-faithful, but retail
// centers the per-slot store base at slot+8 (keeping edi=slot for the thiscall
// `this`) and spills the loop counter to [esp+0x10]; this cl uses one pointer
// (esi=slot) + counter in edi.  Same store order/values; callees+pool reloc-masked.
// 0xbf000  Reset: recycle each channel slot, clear the id-map + record table,
// then drain the recycled-node free pool.
RVA(0x000bf000, 0xd5)
void CNetSession::ResetSync() {
    m_0 = 0;
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
    CPtrList& freeList = CPtrListPool<CNetCmdPacket>::s_freeList;
    while (freeList.GetCount() != 0) {
        void* p = freeList.RemoveTail();
        if (p) {
            RezFree(p);
        }
    }
}

// ClearRange (0xbf120, absorbed from NetMgrMisc.cpp): zero the slot's +0x3c..+0x48
// dword range. (== CLobbyChannel::InitSub3c, declared-only above.)
// @early-stop
// regalloc coin-flip (84%): the inline 16-byte memset (base pointer + 4 dword
// stores of a zero register) is byte-faithful in operation, but retail materializes
// the base in eax via `lea eax,[ecx+0x3c]` and reuses the dead `this` (ecx) as the
// zero, while cl advances ecx in place (`add ecx,0x3c`) and zeroes via eax - a pure
// pointer/zero register swap (zero-register-pinning.md family); not source-steerable
// (a plain 4-member `=0` instead folds to direct `[ecx+N]` stores, 71%). Deferred.
RVA(0x000bf120, 0x11)
void CNetCmdSlot::ClearAckFlags() {
    memset(m_ackFlags, 0, 16);
}

// @early-stop
// rep-stos setup scheduling wall (was 94.1%) PLUS the delinker unpair: logic + every
// other byte exact. The residual is a 3-instruction permutation of the
// memset(m_1b0,0,0x200) prologue - retail emits `lea edi,[ebp+0x1b0]` before
// `mov ecx,0x80 / xor eax,eax`, cl emits it after (identical multiset, one schedule
// swap, source-invariant under /O2). Plus Reset now unpairs (delinker section-packing).
RVA(0x000bf150, 0x58)
void CNetSession::Reset() {
    m_tick = 0;
    m_snapshotDone = 0;
    m_seq = 0;
    i32 i;
    for (i = 0; i < 4; i++) {
        m_slots[i].FullReset();
    }
    memset(m_idMap, 0, 0x200);
    for (i = 0; i < 0x80; i++) {
        m_records[i].m_seq = 0;
        m_records[i].m_count = 0;
        m_records[i].m_payloadLen = 0;
        m_records[i].m_checksum = 0;
    }
}

RVA(0x000bf580, 0x10)
void RecycleCmd(void* cmd) {
    CPtrListPool<CNetCmdPacket>::s_freeList.AddTail(cmd);
}

// @early-stop
// regalloc cascade (~63%): logic byte-faithful; retail pins `this` in edi, hoists
// the slot `== 3` test constant into esi, and parks `len` in the reused incoming
// arg slot (3 distinct stack locals).  This cl pins `this` in esi and coalesces a
// stack local into a register; the consequent register renames cascade the body.
// 0xbf5a0  Poll: advance active slots by `delta`, then drain the endpoint's
// incoming packet queue, dispatching foreign packets.
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
        IDirectPlay4* ep = reinterpret_cast<IDirectPlay4*>(m_netMgr->m_directPlay);
        i32 r = ep->GetMessageCount(m_localDesc->m_id, reinterpret_cast<LPDWORD>(&got));
        avail = (r == 0) ? got : 0;
    }

    i32 a = 0;
    i32 received = 0;
    while (avail > 0 && m_session->m_pollAbort == 0) {
        i32 len = 0x800;
        i32 chan = m_localDesc->m_id;
        IDirectPlay4* ep = reinterpret_cast<IDirectPlay4*>(m_netMgr->m_directPlay);
        i32 st = ep->Receive(
            reinterpret_cast<LPDPID>(&a),
            reinterpret_cast<LPDPID>(&chan),
            1,
            g_lobbyRecvBuf,
            reinterpret_cast<LPDWORD>(&len)
        );
        if (st != 0) {
            CNetMgr::ReportError(const_cast<char*>("c:\\proj\\incs\\netmgr.h"), 0x141, st, 0);
            if (st != 0) {
                break;
            }
        }
        received++;
        avail--;
        if (a != m_localDesc->m_id) {
            Dispatch(a, reinterpret_cast<LobbyMsg*>(g_lobbyRecvBuf), len);
        }
    }
    return received;
}

// @early-stop
// regalloc tie (~93%): logic byte-exact, retail keeps obj in eax / reads the flag
// byte into cl; cl's MSVC spills obj to ecx then reads flag into al.
RVA(0x000bf700, 0x82)
i32 CNetSession::Dispatch(i32 a, LobbyMsg* b, i32 c) {
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
    CNetCmdSlot* target = obj;
    unsigned char* p = reinterpret_cast<unsigned char*>(b);
    if (!(p[0] & 0x80) && (p[0] & 1)) {
        target = &m_slots[p[1]];
        if (!target) {
            return 0;
        }
    }
    return target->ProcessCmd(a, b, c);
}

// @early-stop
// jump-table-placement wall (docs/patterns/switch-jumptable-separate-comdat.md):
// code bytes byte-identical (proven llvm-objdump -dr base vs target); MSVC emits
// the 0xff-byte index table + jump table as separate $L symbols while the delinker
// folds them into the fn symbol, so the table region can't pair.
RVA(0x000bf7c0, 0x95)
i32 CNetSession::DispatchMsg(LobbyMsg* m, i32 arg2) {
    if (!m) {
        return 0;
    }
    switch (m->m_type) {
        case 3:
            m_session->LoadMenuSelectSprite(static_cast<void*>(m));
            return 1;
        case 5:
            if (m->m_04 == 1) {
                void* p = reinterpret_cast<void*>(m->m_08);
                m_session->OnPlayerLeft(reinterpret_cast<i32>(p));
                m_session->ResetPlayerCommands(reinterpret_cast<i32>(p));
                return 1;
            }
            return 1;
        case 49:
            return m_session->HandleControlMsg(reinterpret_cast<CNetCtrlMsg*>(m), arg2);
        case 257:
            return m_session->HandleControlMsg(reinterpret_cast<CNetCtrlMsg*>(m), arg2);
        default:
            return 1;
    }
}

// @early-stop
// regalloc tie (~85%): instruction sequence byte-identical except retail holds
// `this` in ebp where this cl holds it in ebx (cascading esi/edi/ebx renames);
// the callee-save register pick is function-specific and non-steerable.
// 0xbf9e0  Tick: at the reconcile boundary, snapshot every channel's grunt
// state into the current record, broadcast, then flush the pending batches.
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
                payload += obj->Pack(payload, reinterpret_cast<char*>(rec) - payload + 0x410);
            }
        }
        m_session->WriteTag("[end]\n");
        rec->m_payloadLen = static_cast<i32>((payload - reinterpret_cast<char*>(rec) - 0x10));
        m_snapshotDone = 1;
    }
    return SendBatch() + SendAll();
}

// @early-stop
// regalloc/spill wall (~67%): logic correct, retail spills `this` (dead slot) +
// caches &m_slots[0]; this cl allocates the slot pointers differently.
RVA(0x000bfb20, 0x1)
void NoopSync(CGruntzCommand*) {}

RVA(0x000bfb40, 0xe2)
i32 CNetSession::SendAll() {
    i32 count = 0;
    CNetCmdSlot* outer = m_slots;
    for (i32 oi = 0; oi < 4; oi++) {
        if (outer && outer->m_state == 3 && outer->m_isRemote != 0) {
            i32 lo, hi;
            outer->GetRange(&lo, &hi);
            CNetCmdSlot* inner = m_slots;
            i32 in = 4;
            do {
                if (inner && inner->m_state == 3 && inner->m_isRemote == 0) {
                    for (i32 v = lo; v <= hi; v++) {
                        GruntRec* r = reinterpret_cast<GruntRec*>(outer->FindCmd(v));
                        if (r) {
                            i32 flag = (v == hi) ? 3 : 1;
                            if (m_slots[0]
                                    .SendGruntRecord(v, r, flag, oi, inner->m_desc->m_netId)) {
                                count++;
                            }
                        }
                    }
                }
                inner++;
            } while (--in);
        }
        outer++;
    }
    return count;
}

// @early-stop
// one-register regalloc tie (~90%): byte-identical except retail holds `seq` in
// esi where this cl holds it in ecx (mov esi/ecx,[esp+0x10]); non-steerable.
// 0xbfc70  (CNetCmdSlot method: this == the channel base slot)
RVA(0x000bfc70, 0x9c)
i32 CNetCmdSlot::SendGruntRecord(i32 seq, GruntRec* rec, i32 flag, i32 slot, i32 gruntId) {
    if (!rec) {
        return 0;
    }
    if (seq < 0) {
        return 1;
    }
    gA_seq = seq;
    gA_flag = static_cast<unsigned char>(flag);
    gA_slot = static_cast<unsigned char>(slot);
    gA_e04 = rec->m_checksum;
    gA_e08 = rec->m_count;
    memcpy(&gA_data, rec->m_payload, rec->m_payloadLen);
    return (reinterpret_cast<CNetMgr*>(m_latchedSeq))
               ->SetData(
                   m_desc->m_playerId,
                   gruntId,
                   0,
                   reinterpret_cast<i32>(&gA_flag),
                   rec->m_payloadLen + 0xf
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
// regalloc cascade (~84%): logic byte-exact; ecx/edx + esi/eax allocation for the
// modulo index and arg differ, plus the M_c0fd0 sibling (Boundary_0c0fd0) reloc.
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
    gB_flag = flags;
    gB_val = val;
    i32 idx = val % 0x80;
    GruntRec* entry = &m_records[idx];
    gB_m14 = slot->m_baseSeq;
    gB_e04 = entry->m_checksum;
    gB_e08 = entry->m_count;
    memcpy(&gB_data, entry->m_payload, entry->m_payloadLen);
    return m_netMgr->SetData(
               m_localDesc->m_id,
               slot->m_desc->m_netId,
               0,
               reinterpret_cast<i32>(&gB_flag),
               entry->m_payloadLen + 0xe
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
    return slot->Init(reinterpret_cast<i32>(m_session), &m_0[index].m_sel, owner) ? slot : 0;
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
        if (m_slots[i].m_desc->m_netId == playerId) {
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
                s->FullReset(); // 0xc0c20
                SlotInfo* p = s->m_desc;
                s->m_state = 1;
                p->m_dirty = 1;
            }
            s++;
        } while (--n);
    } else if (withFlag != 0) {
        CNetCmdSlot* s = base;
        i32 n = 4;
        do {
            if (s && s->m_state == 3 && s->m_isRemote != 0 && m_seq > s->m_latchedSeq + 2) {
                s->FullReset(); // 0xc0c20
                SlotInfo* p = s->m_desc;
                s->m_state = 1;
                p->m_dirty = 1;
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
    if (!Verify(nextSeq)) { // 0xc0290 (CNetSession::Verify(i32))
        return 0;
    }
    CNetCmdSlot* s = m_slots;
    i32 n = 4;
    do {
        if (s && s->m_state == 3 && s->m_isRemote == 0) {
            s->RemoveCmd(m_seq - 4); // 0xc11b0
        }
        s++;
    } while (--n);
    m_tick = nextTick;
    m_seq = nextSeq;
    m_snapshotDone = 0;
    return 1;
}

// @early-stop
// slot-pointer anchor + member re-read wall (~89.5%) PLUS the delinker unpair: logic
// byte-faithful (the per-slot state==3 / resetGuard gate, the baseSeq window test, the
// Ready()+latchedSeq branch). Retail anchors the slot cursor at +0x24 (m_isRemote)
// and re-reads it with `cmp $0,(esi)` each test; cl CSEs the read and anchors at +0x34
// (m_baseSeq) - a non-steerable addressing-mode tie-break. (wave2-F /GX flip: 100->89.5.)
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

// ---------------------------------------------------------------------------
// CNetSession::ArmSlot (0xc03f0, __thiscall) - store `node` at the (tick-biased)
// wrapped slot of the +0x1b0 synced-object id-map. The setter biases the parity
// by +m_tick, abs's it, masks to 7 bits, abs's again - MSVC emits a single cdq
// feeding both branchless-abs pairs around the `% 128` (see
// docs/patterns/signed-modulo-pow2-abs-restore.md). Ex fake view "GlyphTable"
// (was 100% in its own base-flags TU).
// @early-stop
// regalloc wall (topic:wall topic:regalloc): eax/edx role swap on the two loads
// ([esp+8]/[ecx+0x10]) under this TU's /GX flags; add is commutative so retail's
// operand roles aren't source-steerable (tried both add orders + a temp), ~98.85%.
// ---------------------------------------------------------------------------
RVA(0x000c03f0, 0x29)
void CNetSession::ArmSlot(void* node, i32 parity) {
    m_idMap[(m_tick + (parity & 0xff)) % 128] = static_cast<CGruntzCommand*>(node);
}

RVA(0x000c0430, 0x1f)
CGruntzCommand* CNetSession::GetSlotPtr(i32 v) {
    return m_idMap[(v & 0xff) % 128];
}

// Scan the four inline command slots for the first active (m_state==3), un-reset
// (m_isRemote==0) slot whose latency exceeds key (unsigned).
// @early-stop
// regalloc wall (topic:wall topic:regalloc): structure byte-identical to retail
// (lea/loop/guards/ja/ret all match); residual is the key<->counter register swap
// (retail key=ecx/counter=edx, cl here key=edx/counter=ecx) driven by the key-load
// vs lea schedule order, not source-steerable, ~88.75%.
RVA(0x000c0460, 0x2e)
CNetCmdSlot* CNetSession::FindSlot(u32 key) {
    CNetCmdSlot* p = &m_slots[0];
    for (i32 i = 0; i < 4; i++, p++) {
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
                CNetCmd* c = static_cast<CNetCmd*>(slot->FindCmd(seq));
                if (c != 0 && c->m_4 != e->m_checksum) {
                    return 0;
                }
            }
        }
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CNetCmdSlot::Init (0xc0b10, __thiscall) - seed a fresh slot (owner/buffer),
// drain its queue and reset both command ranges.
// ---------------------------------------------------------------------------
// @early-stop
// regalloc wall (74%): logic + field-store order are byte-faithful, but retail
// keeps the constant 0 in eax (caller-saved, re-`xor`ed after ClearCmds) and the
// args in edx/ecx/edi, whereas cl pins 0 in edi (callee-saved, no re-zero) and
// the args in ecx/eax. A zero-register + arg-register coin-flip; not source-steerable.
RVA(0x000c0b10, 0x72)
i32 CNetCmdSlot::Init(i32 a1, SlotInfo* a2, i32 a3) {
    if (a2 == 0) {
        return 0;
    }
    if (a1 == 0) {
        return 0;
    }
    m_owner =
        reinterpret_cast<CMulti*>(a1); // the session passes its owning CMulti in as an i32 handle
    m_state = a3;
    m_isRemote = 0;
    m_latchedSeq = 0;
    m_desc = a2;
    m_latency = 0;
    m_baseSeq = 0;
    m_maxSeq = 0;
    ClearCmds();
    m_ackFlags[0] = 0;
    m_ackFlags[1] = 0;
    m_ackFlags[2] = 0;
    m_ackFlags[3] = 0;
    ResetTriple(m_rangeA);
    ResetTriple(m_rangeB);
    return 1;
}

// ---------------------------------------------------------------------------
// CNetCmdSlot::ResetAll (0x0c0bb0, __thiscall) - full wipe.
// ---------------------------------------------------------------------------
// @early-stop
// zero-register-pinning wall (73.7%): logic byte-exact. Retail re-materializes
// the splat constant (`xor eax,eax` twice, around ClearCmds) and saves only esi;
// cl instead pins 0 in callee-saved edi across the call. A regalloc coin-flip
// identical to ResetTriple/AllSlotsReady in this TU; not source-steerable.
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
    m_ackFlags[0] = 0;
    m_ackFlags[1] = 0;
    m_ackFlags[2] = 0;
    m_ackFlags[3] = 0;
    ResetTriple(m_rangeA);
    ResetTriple(m_rangeB);
}

// ---------------------------------------------------------------------------
// CNetCmdSlot::FullReset (0x0c0c20, __thiscall) - partial slot reset: keep m_state /
// m_desc / m_owner, zero the sequence-tracking fields, drain the queue and splat
// both command ranges. Called by CNetSession::Reset, CMulti::AckDropPlayer and
// CNetSession::Reconcile (was the CCluster0c::Init view).
// @early-stop
// regalloc tie-break wall (~71%): logic byte-identical, but retail rematerializes
// the zero constant in eax while cl hoists 0 into a callee-saved edi across the
// ClearCmds call; a cl-build heuristic delta, not a source shape.
RVA(0x000c0c20, 0x3f)
void CNetCmdSlot::FullReset() {
    m_isRemote = 0;
    m_latchedSeq = 0;
    m_latency = 0;
    m_baseSeq = 0;
    m_maxSeq = 0;
    ClearCmds();
    m_ackFlags[0] = 0;
    m_ackFlags[1] = 0;
    m_ackFlags[2] = 0;
    m_ackFlags[3] = 0;
    ResetTriple(m_rangeA);
    ResetTriple(m_rangeB);
}

// ---------------------------------------------------------------------------
// CNetCmdSlot::ProcessCmd (0x0c0c70, __thiscall) - parse one incoming command record.
// ---------------------------------------------------------------------------
// @early-stop
// regalloc + this-residency wall (~75%): the full control flow is byte-faithful -
// the opcode/parity dispatch, the bit-7 relay, the FindCmdSlot ack-flag set, the
// RaiseMax/NetCmdIdAdd/NetCmdIdClear/NetCmdIdFind/AdvanceSeq window updates, the
// Unmatched_bf530 packet alloc + inline payload memcpy + AddCmd, and the per-entry
// Single/MultiCommand parse loop with the vtable-slot-7 dispatch + EnqueueCommand.
// Three unsteerable MSVC5 /O2 choices remain: (a) cl pins `this` in ebp where retail
// keeps it in esi + a [esp+0x18] reload; (b) the redundant `mov ecx,esi` retail
// refreshes before each __stdcall NetCmdId* call; (c) retail walks the header through
// an advancing cursor with grouped byte-counter decrements. No source lever; final sweep.
RVA(0x000c0c70, 0x20f)
i32 CNetCmdSlot::ProcessCmd(i32 playerId, void* rec, i32 size) {
    if (rec == 0) {
        return 0;
    }
    u8 opcode = *static_cast<u8*>(rec);
    i32 odd = opcode & 1;
    char* p = reinterpret_cast<char*>(rec) + 1;
    if (m_state != 3) {
        return 1;
    }
    if (opcode & 0x80) {
        return m_owner->DispatchRecvMsg(m_desc->m_netId, static_cast<char*>(rec), size);
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
    CNetCmdHdr* h = reinterpret_cast<CNetCmdHdr*>(p);
    i32 seq = h->m_sequence;
    i32 base = h->m_windowBase;
    i32 flags = h->m_flags;
    u8 count = h->m_entryCount;
    char* cursor = p + 13;
    rem -= 13;

    if (m_isRemote != 0 && odd) {
        CNetCmdSlot* slot = m_owner->m_session->FindCmdSlot(playerId);
        if (slot == 0) {
            return 0;
        }
        if (opcode & 2) {
            i32 pid = slot->m_desc->m_cmdWord & 0xff;
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

    CNetCmdPacket* pkt = static_cast<CNetCmdPacket*>(Unmatched_bf530(0));
    pkt->m_sequence = seq;
    pkt->m_owner = this;
    pkt->m_flags = static_cast<u8>(flags);
    pkt->m_payloadLength = rem;
    memcpy(pkt->m_payload, cursor, rem);
    AddCmd(reinterpret_cast<CNetCmd*>(pkt));

    for (i32 i = count & 0xff; i > 0; i--) {
        u8 b = *reinterpret_cast<u8*>(cursor);
        CGruntzCommand* obj;
        if (b & 1) {
            obj = CGruntzSingleCommand::Allocate();
        } else if (b & 2) {
            obj = CGruntzMultiCommand::Allocate();
        } else {
            continue;
        }
        i32 consumed = obj->Parse(cursor, rem);
        obj->m_submitted = 1; // "submitted" latch
        // m_owner->m_4 is the game-mgr (CGruntzMgr); its +0x6c command manager is the
        // real CGruntzCmdMgr member m_cmdSubMgr - no cross-cast to a net-facet view.
        m_owner->m_mgr->m_cmdSubMgr->EnqueueCommand(0, obj);
        rem -= consumed;
        cursor += consumed;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CNetCmdSlot::AdvanceSeq (0x0c0f10, __thiscall) - fold an acknowledged id into
// the high-water window.
// ---------------------------------------------------------------------------
// @early-stop
// this-in-ecx residency wall (86.6%): logic + control flow byte-exact. Retail
// refreshes `mov ecx,esi` (this) before each __stdcall NetCmdId* call even though
// the convention ignores ecx (4 redundant moves cl omits); not source-steerable.
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

// ---------------------------------------------------------------------------
// CNetCmdSlot::ResetTriple (0x0c10a0, __thiscall) - splat -1 over three dwords.
// ---------------------------------------------------------------------------
// @early-stop
// regalloc swap (93.33%): logic byte-exact, but retail loads p into ecx (reusing
// the dead `this`) and materializes -1 into eax, while cl picks eax for p and ecx
// for -1. A pure eax/ecx coin-flip on a thiscall member that ignores `this`.
RVA(0x000c10a0, 0x12)
void CNetCmdSlot::ResetTriple(i32* p) {
    p[0] = -1;
    p[1] = -1;
    p[2] = -1;
}

// ---------------------------------------------------------------------------
// NetCmdIdToString (0x0c10d0, __stdcall) - format the three-int player-id set as a
// "%d,"-joined debug string in the shared g_idListBuf accumulator.
// ---------------------------------------------------------------------------
// @early-stop
// frame-slot-selection wall (95.36%): the whole body is byte-faithful. The only
// real residue is where the spilled 3->0 counter lives: retail REUSES the dead arg1
// home slot ([esp+0x14]) with a zero-extra frame, while cl allocates a fresh local
// (`push ecx`/`pop ecx`, counter at [esp+0x10]). Not source-steerable.
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
void CNetCmdSlot::AddCmd(CNetCmd* cmd) {
    if (cmd != 0 && FindCmd(cmd->m_seq) == 0) {
        m_cmds.AddTail(cmd);
    }
}

RVA(0x000c11b0, 0x55)
void CNetCmdSlot::RemoveCmd(i32 seq) {
    CNetCmdNode* node = reinterpret_cast<CNetCmdNode*>(m_cmds.GetHeadPosition());
    while (node != 0) {
        CNetCmdNode* cur = node;
        node = node->m_next;
        CNetCmd* cmd = cur->m_data;
        if (seq == cmd->m_seq) {
            if (node != 0) {
                m_cmds.RemoveAt(reinterpret_cast<POSITION>(node->m_prev));
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
    CNetCmdNode* node = reinterpret_cast<CNetCmdNode*>(m_cmds.GetHeadPosition());
    if (node == 0) {
        *pMax = 0;
        *pMin = 0;
        return;
    }
    do {
        CNetCmdNode* cur = node;
        node = node->m_next;
        CNetCmd* cmd = cur->m_data;
        if (cmd->m_seq > *pMax) {
            *pMax = cmd->m_seq;
        }
        if (cmd->m_seq < *pMin) {
            *pMin = cmd->m_seq;
        }
    } while (node != 0);
}

RVA(0x000c12b0, 0x1f)
CNetCmd* CNetCmdSlot::FindCmd(i32 seq) {
    CNetCmdNode* node = reinterpret_cast<CNetCmdNode*>(m_cmds.GetHeadPosition());
    while (node != 0) {
        CNetCmdNode* cur = node;
        node = node->m_next;
        CNetCmd* cmd = cur->m_data;
        if (seq == cmd->m_seq) {
            return cmd;
        }
    }
    return 0;
}

RVA(0x000c12e0, 0x2c)
void CNetCmdSlot::ClearCmds() {
    while (m_cmds.GetCount() != 0) {
        CNetCmd* cmd = static_cast<CNetCmd*>(m_cmds.RemoveHead());
        if (cmd != 0) {
            RecycleCmd(cmd);
        }
    }
}

// ---------------------------------------------------------------------------
// CNetCmdSlot::Ready (0x0c1320, __thiscall) - dispatched by CNetSession::Verify on each
// slot cursor: false (0) if any active, unreset command slot has not yet been acked
// locally (per THIS slot's m_ackFlags); true (1) otherwise. `this` is the slot: m_owner
// (+0x1c) -> m_session -> m_slots[], gated by m_ackFlags (+0x3c).
// ---------------------------------------------------------------------------
// @early-stop
// induction-var/regalloc wall (~79%): logic byte-exact. Retail keeps `sess` in
// esi and recomputes the slot via `lea ecx,[esi+eax+0x20]` each iteration; cl folds
// sess+offset into one running slot pointer and counts i<4. An IV-selection coin-flip.
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
