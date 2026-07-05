// NetSession2.cpp - the per-game command-session methods Reset (0xbf150), Init
// (0xbef80) and Verify (0xc0290) of the canonical CNetSession (the 0x20bb0 object;
// four 0x64-byte command slots at +0x20, a 0x200-byte scratch region at +0x1b0, and
// 0x80 resync entries (0x410 each) at +0x3b0). Reset re-inits them; Init wires the
// three owner pointers then Resets; Verify validates the slots against a windowed
// sequence. All cross-class callees are external (reloc-masked); offsets bind.
//
// FOLDED onto the canonical CNetSession / CNetCmdSlot (<Net/NetMgr.h>) - the former
// TU-local CNetSession2 / CNetCmdSlot2 shadow is gone. The fold is byte-neutral in
// operation, but the delinker packs these three target functions into one .text
// section (Init@0, Reset@0x54, Verify@0xac) and only the offset-0 symbol (Init)
// re-pairs with the base's separate COMDAT sections under the CNetSession name -
// Reset + Verify(i32) unpair to a lower %. That is an ACCEPTED delinker/objdiff
// symbol-packing artifact (user directive: don't gate on % during view destruction),
// not a code change: the compiled bytes are identical to the shadow's.
#include <Ints.h>
#include <Net/NetMgr.h> // canonical CNetSession / CNetCmdSlot / CNetResyncEntry / CNetCmdBuf
#include <rva.h>
#include <string.h>

// @early-stop
// rep-stos setup scheduling wall (was 94.1% as the shadow) PLUS the delinker unpair:
// logic + every other byte exact. The residual is a 3-instruction permutation of the
// memset(m_1b0,0,0x200) prologue - retail emits `lea edi,[ebp+0x1b0]` before
// `mov ecx,0x80 / xor eax,eax`, cl emits it after (identical multiset, one schedule
// swap, source-invariant under /O2). Plus Reset now unpairs (see file header).
RVA(0x000bf150, 0x58)
void CNetSession::Reset() {
    m_10 = 0;
    m_14 = 0;
    m_18 = 0;
    i32 i;
    for (i = 0; i < 4; i++) {
        m_slots[i].FullReset();
    }
    memset(m_1b0, 0, 0x200);
    for (i = 0; i < 0x80; i++) {
        m_entries[i].m_0 = 0;
        m_entries[i].m_8 = 0;
        m_entries[i].m_c = 0;
        m_entries[i].m_4 = 0;
    }
}

RVA(0x000bef80, 0x51)
i32 CNetSession::Init(void* a1, CNetMgr* a2, void* a3) {
    if (a1 == 0) {
        return 0;
    }
    if (a2 == 0) {
        return 0;
    }
    if (a3 == 0) {
        return 0;
    }
    m_0 = (CNetCmdBuf*)a1;
    m_4 = (i32)a2; // the owning CNetMgr is kept as an i32 handle (CreateSlot re-passes it)
    m_8 = a3;
    Reset();
    m_1c = a2->m_cmdDelay;
    return 1;
}

// @early-stop
// slot-pointer anchor + member re-read wall (~89.5% as the shadow) PLUS the delinker
// unpair: logic byte-faithful (the per-slot state==3 / resetGuard gate, the baseSeq
// window test, the Ready()+latchedSeq branch). Retail anchors the slot cursor at +0x24
// (m_resetGuard) and re-reads it with `cmp $0,(esi)` each test; cl CSEs the read and
// anchors at +0x34 (m_baseSeq) - a non-steerable addressing-mode tie-break. Plus
// Verify(i32) now unpairs (see file header).
RVA(0x000c0290, 0x63)
i32 CNetSession::Verify(i32 n) {
    for (i32 i = 0; i < 4; i++) {
        CNetCmdSlot* s = &m_slots[i];
        if (s != 0) {
            if (s->m_state == 3 && s->m_resetGuard == 0) {
                if (s->m_baseSeq < n) {
                    return 0;
                }
            } else if (s->m_state == 3 && s->m_resetGuard != 0) {
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
