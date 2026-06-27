// m2_ActRegSiblings.cpp - two CUserLogic-leaf RegisterActs siblings (C:\Proj\Gruntz).
//
// Both bind a per-frame activation handler to the shared "A" key via the global
// name registry, then store the handler PMF into the class's own activation-
// coordinate registry singleton - the SAME archetype as CAniCycle::RegisterActs /
// CBehindCandyAni::RegisterActs (docs ActNameRegistry.h). The two per-class
// registries (g_netBe90 @0x64be90 built by 0xc76d0's sibling, g_64bf00 @0x64bf00
// built by Unmatched_c76d0) reuse the existing tags so their loads reloc-mask.
// Only offsets + code bytes are load-bearing; class identities are best-guess.
#include <Gruntz/ActNameRegistry.h>
#include <Stub/CCheckpointTrigger.h>

// The per-class activation-coordinate registry: same range/cache shape as the
// shared name registry (CActColl coll @+0 / coll2 @+4 / [lo,hi] fast range /
// base+stride slot / cur slow-path result / scratch zeroed first). ResolveEntry
// folds the VActLookup archetype the slow Insert dispatches __thiscall on coll2.
struct CSiblingActReg {
    void* m_coll;       // +0x00 (CActColl: Find 0x16da80)
    CActColl2* m_coll2; // +0x04
    i32 m_lo;           // +0x08
    i32 m_hi;           // +0x0c
    char* m_base;       // +0x10
    char* m_cur;        // +0x14
    i32 m_stride;       // +0x18
    char m_pad1c[0x20 - 0x1c];
    i32 m_scratch; // +0x20

    char* ResolveEntry(i32 id) {
        m_scratch = 0;
        if (id >= m_lo && id <= m_hi) {
            return m_base + (id - m_lo) * m_stride;
        }
        if (((CActColl*)this)->Find(id, 0)) {
            return m_base + (id - m_lo) * m_stride;
        }
        void* item = g_actCache;
        g_actAllocResult = (void*)ActAlloc();
        m_coll2->Insert(this, item, 0xc);
        return m_cur;
    }
};

// Class A: registry @0x64be90 (reuse the g_netBe90 tag so the DATA-pinned loads
// reloc-mask), handler @0xc62e0 (the "LoadAttributes" per-frame entry).
struct CNetSingletonBe90 : CSiblingActReg {};
extern CNetSingletonBe90 g_netBe90; // 0x64be90 (DATA-pinned by NetMgrMisc.cpp)
struct CSiblingActorA {
    i32 LoadAttributes(); // 0xc62e0
};
struct CSiblingActorAEntry {
    i32 (CSiblingActorA::*m_fn)();
};

// Class B: registry @0x64bf00 (reuse the g_64bf00 tag), handler @0xc7ab0.
struct Obj64bf00;
DATA(0x0024bf00)
extern Obj64bf00 g_64bf00; // 0x64bf00
struct Obj64bf00 : CSiblingActReg {};
struct CSiblingActorB {
    i32 Advance(); // 0xc7ab0
};
struct CSiblingActorBEntry {
    i32 (CSiblingActorB::*m_fn)();
};

// 0xc60e0 - CSiblingActorA::RegisterActs: register "A" in the shared name registry
// (first caller only), then bind LoadAttributes into the class registry slot.
//
// @early-stop
// register-pinning wall (docs/patterns/zero-register-pinning.md +
// test-old-value-decrement-loop-while-postdec.md, topic:wall topic:regalloc): every
// call/immediate/branch/offset + the `mov [entry],offset` handler store is
// byte-faithful; the residual is the slot-vs-id callee-saved register choice that
// cascades into the free-loop trip-count materialization (`ecx=cnt; eax=cnt-1; lea
// ebp,[eax+1]`). Identical to CAniCycle::RegisterActs. Deferred to the final sweep.
RVA(0x000c60e0, 0x18d)
void CSiblingActorA_RegisterActs() {
    i32 id = (i32)g_buteTree.Find(s_actKeyA);
    if (id == 0) {
        g_buteTree.Insert(s_actKeyA, (void*)g_nextActId);
        id = g_nextActId;
        char* slot = ActNameLookup(id);
        i32 cnt = g_nameRegScratch;
        void** list = g_nameRegCurList;
        if (cnt != 0) {
            do {
                if (list != 0) {
                    ((CActName*)list)->Free();
                }
                list++;
            } while (--cnt);
        }
        ((CActName*)slot)->Assign(s_actKeyA);
        g_nextActId++;
    }
    ((CSiblingActorAEntry*)g_netBe90.ResolveEntry(id))->m_fn = &CSiblingActorA::LoadAttributes;
}

// 0xc78b0 - CSiblingActorB::RegisterActs: same archetype, registry @0x64bf00.
//
// @early-stop
// same register-pinning wall as CSiblingActorA_RegisterActs above (logic + every
// byte byte-faithful; only the regalloc/free-loop-count materialization diverges).
RVA(0x000c78b0, 0x18d)
void CSiblingActorB_RegisterActs() {
    i32 id = (i32)g_buteTree.Find(s_actKeyA);
    if (id == 0) {
        g_buteTree.Insert(s_actKeyA, (void*)g_nextActId);
        id = g_nextActId;
        char* slot = ActNameLookup(id);
        i32 cnt = g_nameRegScratch;
        void** list = g_nameRegCurList;
        if (cnt != 0) {
            do {
                if (list != 0) {
                    ((CActName*)list)->Free();
                }
                list++;
            } while (--cnt);
        }
        ((CActName*)slot)->Assign(s_actKeyA);
        g_nextActId++;
    }
    ((CSiblingActorBEntry*)g_64bf00.ResolveEntry(id))->m_fn = &CSiblingActorB::Advance;
}

// Class C: CCheckpointTrigger, registry @0x64e7c0 (g_checkpointActReg, DATA-pinned
// in CCheckpointTrigger.cpp), handler @0x10ede0 (Trigger). Homed here (not the
// All.cpp-aggregated CCheckpointTrigger.cpp) because the shared registry archetype
// pulls <Mfc.h>, which the engine_label_stubs aggregate cannot also include.
struct CCheckpointActReg : CSiblingActReg {};
extern CCheckpointActReg g_checkpointActReg; // 0x64e7c0 (DATA-pinned in CCheckpointTrigger.cpp)
struct CCheckpointActEntry {
    i32 (CCheckpointTrigger::*m_fn)();
};

// 0x10ebe0 - CCheckpointTrigger::RegisterActs: same archetype, registry @0x64e7c0.
//
// @early-stop
// same register-pinning wall as the two siblings above (logic + every byte faithful;
// only the regalloc/free-loop-count materialization diverges). Deferred.
RVA(0x0010ebe0, 0x18d)
void CCheckpointTrigger::RegisterActs() {
    i32 id = (i32)g_buteTree.Find(s_actKeyA);
    if (id == 0) {
        g_buteTree.Insert(s_actKeyA, (void*)g_nextActId);
        id = g_nextActId;
        char* slot = ActNameLookup(id);
        i32 cnt = g_nameRegScratch;
        void** list = g_nameRegCurList;
        if (cnt != 0) {
            do {
                if (list != 0) {
                    ((CActName*)list)->Free();
                }
                list++;
            } while (--cnt);
        }
        ((CActName*)slot)->Assign(s_actKeyA);
        g_nextActId++;
    }
    ((CCheckpointActEntry*)g_checkpointActReg.ResolveEntry(id))->m_fn = &CCheckpointTrigger::Trigger;
}
