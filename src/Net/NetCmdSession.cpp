// NetCmdSession.cpp - CNetSession (the DirectPlay session sub-object at
// CNetMgr+0x520) command-slot helpers, plus the CNetCmdSlot seeding init.
// CNetSession keeps four inline 0x64-byte command slots at +0x20, an array of
// 0x238-byte command buffers at +0x0, and an array of 0x410-byte resync entries
// at +0x3b0. These are self-contained (no DIR32 relocs); the only externals are
// CNetCmdSlot helpers (ClearCmds/ResetTriple/FindCmd/ResetAll), reached as
// reloc-masked rel32 calls.
#include <Net/NetMgr.h>
#include <rva.h>
#include <string.h> // memset (inlined rep stos over the resync scratch block)

// CNetCmdSlot helper reached only here (0xc0bb0, __thiscall, external).

// ---------------------------------------------------------------------------
// CNetSession::ResetAll (0xbbf80, __thiscall) - full session reset: zero the
// scalar header (m_1c latched to 1), reset every one of the four inline command
// slots (the same field-wipe + ClearCmds + ResetTriple sequence as
// CNetCmdSlot::ResetAll, inlined here), clear the 0x200-byte resync scratch
// block, then zero all 0x80 resync entries.
// ---------------------------------------------------------------------------
// @early-stop
// loop induction-variable / regalloc wall (71.5%): every operation is byte-faithful
// (header zero, the inlined per-slot wipe + ClearCmds + both ResetTriple calls, the
// rep stos over m_1b0, the 0x80-entry zero loop). Retail strength-reduces the slot
// loop into TWO induction vars (edi=slot passed as `this`, esi=slot+8 for the field
// stores) and SPILLS the down-counter to a stack slot (the leading `push ecx`),
// also basing the entry loop at entry+8; cl uses one IV (esi=slot) and keeps the
// counter in edi. A pure induction-var-selection coin-flip (same family as
// CNetSyncCheck::AllSlotsReady ~79%); not source-steerable. Final sweep.
RVA(0x000bbf80, 0xb7)
void CNetSession::ResetAll() {
    m_0 = 0;
    m_4 = 0;
    m_8 = 0;
    m_c = 0;
    m_10 = 0;
    m_14 = 0;
    m_18 = 0;
    m_1c = 1;

    i32 i;
    CNetCmdSlot* slot = m_slots;
    for (i = 4; i != 0; i--) {
        slot->m_state = 0;
        slot->m_resetGuard = 0;
        slot->m_latchedSeq = 0;
        slot->m_cmdHead = 0;
        slot->m_latency = 0;
        slot->m_baseSeq = 0;
        slot->m_maxSeq = 0;
        slot->m_owner = 0;
        slot->ClearCmds();
        slot->m_ackFlags[0] = 0;
        slot->m_ackFlags[1] = 0;
        slot->m_ackFlags[2] = 0;
        slot->m_ackFlags[3] = 0;
        slot->ResetTriple(slot->m_rangeA);
        slot->ResetTriple(slot->m_rangeB);
        slot++;
    }

    memset(m_1b0, 0, sizeof(m_1b0));

    CNetResyncEntry* e = m_entries;
    for (i = 0x80; i != 0; i--) {
        e->m_0 = 0;
        e->m_8 = 0;
        e->m_c = 0;
        e->m_4 = 0;
        e++;
    }
}

// ---------------------------------------------------------------------------
// CNetSession::CreateSlot (0xbfff0, __thiscall) - reset slot[index] and seed it
// from the session's command-buffer array; returns the slot on success.
// ---------------------------------------------------------------------------
RVA(0x000bfff0, 0x5d)
CNetCmdSlot* CNetSession::CreateSlot(i32 index, i32 owner) {
    if (index < 0 || index >= 4) {
        return 0;
    }
    CNetCmdSlot* slot = &m_slots[index];
    if (slot == 0) {
        return 0;
    }
    ((CNetCmdSlot*)slot)->ResetAll();
    return slot->Init(m_4, &m_0[index].m_sel.m_slotHead, owner) ? slot : 0;
}

// ---------------------------------------------------------------------------
// CNetSession::FindCmdSlot (0xc00a0, __thiscall) - the slot whose owning player
// (m_c[6]) matches, or null.
// ---------------------------------------------------------------------------
RVA(0x000c00a0, 0x31)
CNetCmdSlot* CNetSession::FindCmdSlot(i32 playerId) {
    for (i32 i = 0; i < 4; i++) {
        if (m_slots[i].m_cmdHead[6] == playerId) {
            return &m_slots[i];
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------
// CNetSession::CheckLatency (0xc04a0, __thiscall) - 0 if any active (m_state==3),
// unreset (m_resetGuard==0) slot's latency (m_latency) exceeds the cap; 1 otherwise.
// ---------------------------------------------------------------------------
RVA(0x000c04a0, 0x37)
i32 CNetSession::CheckLatency(i32 cap) {
    for (i32 i = 0; i < 4; i++) {
        CNetCmdSlot* slot = &m_slots[i];
        if (slot != 0 && slot->m_state == 3 && slot->m_resetGuard == 0
            && (u32)slot->m_latency > (u32)cap) {
            return 0;
        }
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CNetSession::Verify (0xc04f0, __thiscall) - resync consistency: each active
// slot's command (seq = m_18-2) must agree (+0x4) with the resync entry indexed
// by (m_18-2)%128.
// ---------------------------------------------------------------------------
RVA(0x000c04f0, 0x7c)
i32 CNetSession::Verify() {
    i32 seq = m_18 - 2;
    CNetResyncEntry* e = &m_entries[seq % 128];
    if (e != 0) {
        for (i32 i = 0; i < 4; i++) {
            CNetCmdSlot* slot = &m_slots[i];
            if (slot != 0 && slot->m_state == 3 && slot->m_resetGuard == 0) {
                CNetCmd* c = (CNetCmd*)slot->FindCmd(seq);
                if (c != 0 && c->m_4 != e->m_4) {
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
// the args in ecx/eax. A zero-register + arg-register coin-flip driven by cl
// moving `this`->esi before the first arg load (const-materialize-into-reg-vs-
// immediate.md / zero-register-pinning.md family); not source-steerable. Deferred.
RVA(0x000c0b10, 0x72)
i32 CNetCmdSlot::Init(i32 a1, i32* a2, i32 a3) {
    if (a2 == 0) {
        return 0;
    }
    if (a1 == 0) {
        return 0;
    }
    m_owner = (CNetMgr*)a1; // the session passes its owning CNetMgr in as an i32 handle
    m_state = a3;
    m_resetGuard = 0;
    m_latchedSeq = 0;
    m_cmdHead = a2;
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
