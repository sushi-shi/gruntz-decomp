// NetCmdSession.cpp - CNetSession (the DirectPlay session sub-object at
// CNetMgr+0x520) command-slot helpers, plus the CNetCmdSlot seeding init.
// CNetSession keeps four inline 0x64-byte command slots at +0x20, an array of
// 0x238-byte command buffers at +0x0, and an array of 0x410-byte resync entries
// at +0x3b0. These are self-contained (no DIR32 relocs); the only externals are
// CNetCmdSlot helpers (ClearCmds/ResetTriple/FindCmd/Reset0bb0), reached as
// reloc-masked rel32 calls.
#include <Net/NetMgr.h>
#include <rva.h>

// A queued command, as the slot list holds it: +0x0 sequence, +0x4 a payload
// word Verify compares against the resync entry.
struct CNetCmd {
    i32 m_seq; // +0x0
    i32 m_4;   // +0x4
};

// CNetCmdSlot helper reached only here (0xc0bb0, __thiscall, external).
struct CNetCmdSlotReset {
    void Reset0bb0();
};

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
    ((CNetCmdSlotReset*)slot)->Reset0bb0();
    return slot->Init(m_4, &m_0[index].m_150, owner) ? slot : 0;
}

// ---------------------------------------------------------------------------
// CNetSession::FindCmdSlot (0xc00a0, __thiscall) - the slot whose owning player
// (m_c[6]) matches, or null.
// ---------------------------------------------------------------------------
RVA(0x000c00a0, 0x31)
CNetCmdSlot* CNetSession::FindCmdSlot(i32 playerId) {
    for (i32 i = 0; i < 4; i++) {
        if (m_slots[i].m_c[6] == playerId) {
            return &m_slots[i];
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------
// CNetSession::CheckLatency (0xc04a0, __thiscall) - 0 if any active (m_0==3),
// unreset (m_4==0) slot's latency (m_10) exceeds the cap; 1 otherwise.
// ---------------------------------------------------------------------------
RVA(0x000c04a0, 0x37)
i32 CNetSession::CheckLatency(i32 cap) {
    for (i32 i = 0; i < 4; i++) {
        CNetCmdSlot* slot = &m_slots[i];
        if (slot != 0 && slot->m_0 == 3 && slot->m_4 == 0 && (u32)slot->m_10 > (u32)cap) {
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
            if (slot != 0 && slot->m_0 == 3 && slot->m_4 == 0) {
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
    m_1c = a1;
    m_0 = a3;
    m_4 = 0;
    m_8 = 0;
    m_c = a2;
    m_10 = 0;
    m_14 = 0;
    m_18 = 0;
    ClearCmds();
    m_3c = 0;
    m_40 = 0;
    m_44 = 0;
    m_48 = 0;
    ResetTriple(m_4c);
    ResetTriple(m_58);
    return 1;
}
