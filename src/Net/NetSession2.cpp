// NetSession2.cpp - the per-game command-session sub-object (the inline variant
// matcher-4 also models): four 0x64-byte command slots at +0x20, a 0x200-byte
// scratch region at +0x1b0, and 0x80 resync entries (0x410 each) at +0x3b0.
// Reset re-inits them; Init wires the three owner pointers then Resets; Verify
// validates the slots against a windowed sequence. All cross-class callees are
// external (reloc-masked). Field names are placeholders; offsets are load-bearing.
#include <Ints.h>
#include <Net/NetMgr.h> // shared CNetMgr (m_cmdDelay @+0x5a4) + CNetResyncEntry
#include <rva.h>
#include <string.h>

struct CNetCmdSlot2 {
    i32 m_armed;      // +0x00  armed flag (==3 active)
    i32 m_resetGuard; // +0x04  reset guard
    i32 m_latchedSeq; // +0x08  latched seq
    char m_padc[0x14 - 0xc];
    i32 m_baseSeq; // +0x14  base seq
    char m_pad18[0x64 - 0x18];

    void FullReset(); // 0xc0c20 (external, reloc-masked)
    i32 Ready();      // 0xc1320 (external, reloc-masked)
};
SIZE(CNetCmdSlot2, 0x64); // fully-known inline command slot (array stride 0x64)

struct CNetSession2 {
    void* m_0;    // +0x00  owner a1
    CNetMgr* m_4; // +0x04  owner net manager a2
    void* m_8;    // +0x08  owner a3
    char m_padc[0x10 - 0xc];
    i32 m_10;                        // +0x10
    i32 m_14;                        // +0x14
    i32 m_18;                        // +0x18
    i32 m_cachedCmdDelay;            // +0x1c  cached a2->m_cmdDelay
    CNetCmdSlot2 m_slots[4];         // +0x20  (0x64 each -> +0x20..+0x1b0)
    i32 m_1b0[0x80];                 // +0x1b0 (0x200 bytes scratch)
    CNetResyncEntry m_entries[0x80]; // +0x3b0 (0x410 each)

    void Reset();                              // 0xbf150
    i32 Init(void* a1, CNetMgr* a2, void* a3); // 0xbef80
    i32 Verify(i32 n);                         // 0xc0290
};
SIZE(CNetSession2, 0x20bb0); // fully-laid-out: +0x3b0 + 0x80*0x410 resync entries

// @early-stop
// rep-stos setup scheduling wall (94.1%): logic + every other byte exact. The ONLY
// residual is a 3-instruction permutation of the memset(m_1b0,0,0x200) prologue -
// retail emits `lea edi,[ebp+0x1b0]` before `mov ecx,0x80 / xor eax,eax`, cl emits
// it after (identical instruction multiset, one schedule swap, source-invariant
// under /O2). docs/patterns: instruction-scheduling coin-flip. Final sweep.
RVA(0x000bf150, 0x58)
void CNetSession2::Reset() {
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
i32 CNetSession2::Init(void* a1, CNetMgr* a2, void* a3) {
    if (a1 == 0) {
        return 0;
    }
    if (a2 == 0) {
        return 0;
    }
    if (a3 == 0) {
        return 0;
    }
    m_0 = a1;
    m_4 = a2;
    m_8 = a3;
    Reset();
    m_cachedCmdDelay = a2->m_cmdDelay;
    return 1;
}

// @early-stop
// slot-pointer anchor + member re-read wall (~89.5%): logic byte-faithful (the
// per-slot armed==3 / resetGuard gate, the baseSeq window test, the Ready()+
// latchedSeq branch). Retail anchors the slot cursor at +0x24 (m_resetGuard) and
// re-reads resetGuard with `cmp $0,(esi)` each test; cl CSEs the read into a reg
// and anchors at +0x34 (m_baseSeq). Splitting the else-if into two ifs regressed
// it (87.2%); the anchor is a non-steerable addressing-mode tie-break. Final sweep.
RVA(0x000c0290, 0x63)
i32 CNetSession2::Verify(i32 n) {
    for (i32 i = 0; i < 4; i++) {
        CNetCmdSlot2* s = &m_slots[i];
        if (s != 0) {
            if (s->m_armed == 3 && s->m_resetGuard == 0) {
                if (s->m_baseSeq < n) {
                    return 0;
                }
            } else if (s->m_armed == 3 && s->m_resetGuard != 0) {
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
