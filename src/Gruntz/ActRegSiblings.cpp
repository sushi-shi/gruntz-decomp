// ActRegSiblings.cpp - two CUserLogic-leaf RegisterActs siblings (C:\Proj\Gruntz).
//
// Both bind a per-frame activation handler to the shared "A" key via the global
// name registry, then store the handler PMF into the class's own activation-
// coordinate registry singleton - the SAME archetype as CAniCycle::RegisterActs /
// CBehindCandyAni::RegisterActs (docs ActNameRegistry.h). The two per-class
// registries (g_netBe90 @0x64be90 built by 0xc76d0's sibling, g_64bf00 @0x64bf00
// built by Unmatched_c76d0) reuse the existing tags so their loads reloc-mask.
// Only offsets + code bytes are load-bearing; class identities are best-guess.
#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ObjectDropper.h>
#include <Gruntz/ActReg.h>            // the shared activation-registrar archetype (CSiblingActReg)
#include <Gruntz/CheckpointTrigger.h> // real CCheckpointTrigger : CUserLogic (was Stub/)

// The per-class activation-coordinate registry (CSiblingActReg) is the shared
// <Gruntz/ActReg.h> CActReg-derived alias: same range/cache shape as the shared name
// registry; ResolveEntry folds the VActLookup archetype the slow Insert dispatches
// __thiscall on coll2.

// Class A: registry @0x64be90 (reuse the g_netBe90 tag so the DATA-pinned loads
// reloc-mask), handler @0xc62e0 (the "Update" per-frame entry).
struct CNetSingletonBe90 : CSiblingActReg {};
extern CNetSingletonBe90 g_netBe90; // 0x64be90 (DATA-pinned by NetMgrMisc.cpp)
struct CSiblingActorAEntry {
    i32 (CObjectDropper::*m_fn)();
};

// Class B: registry @0x64bf00 (reuse the g_64bf00 tag), handler @0xc7ab0.
struct CNetSingletonBf00;
DATA(0x0024bf00)
extern CNetSingletonBf00 g_64bf00; // 0x64bf00
struct CNetSingletonBf00 : CSiblingActReg {};
struct CSiblingActorB {
    i32 Advance();          // 0xc7ab0
    void Activate(i32 actId); // 0xc7750 (fire the registered act handler)
};
struct CSiblingActorBEntry {
    i32 (CSiblingActorB::*m_fn)();
};

// CObjectDropper::Activate (0xc5f80): the runtime side of the registry - resolve the
// handler for act `actId` and, if bound, fire it on `this`. ResolveEntry has side
// effects (m_scratch reset + GrowTo-on-miss), so it is re-run for the actual call
// rather than cached; the null-slot path just returns (eax = the entry ptr).
RVA(0x000c5f80, 0x102)
void CObjectDropper::FireAct(i32 actId) {
    if (((CSiblingActorAEntry*)g_netBe90.ResolveEntry(actId))->m_fn != 0) {
        (this->*(((CSiblingActorAEntry*)g_netBe90.ResolveEntry(actId))->m_fn))();
    }
}

// CObjectDropper::RegisterActs: register "A" in the shared name registry
// (first caller only), then bind Update into the class registry slot.
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
                    ((CString*)list)->CString::~CString();
                }
                list++;
            } while (--cnt);
        }
        ((CString*)slot)->operator=(s_actKeyA);
        g_nextActId++;
    }
    ((CSiblingActorAEntry*)g_netBe90.ResolveEntry(id))->m_fn = &CObjectDropper::Update;
}

// CSiblingActorB::Activate (0xc7750): runtime dispatch for registry @0x64bf00 - same
// double-ResolveEntry + PMF-fire archetype as CObjectDropper::Activate.
RVA(0x000c7750, 0x102)
void CSiblingActorB::Activate(i32 actId) {
    if (((CSiblingActorBEntry*)g_64bf00.ResolveEntry(actId))->m_fn != 0) {
        (this->*(((CSiblingActorBEntry*)g_64bf00.ResolveEntry(actId))->m_fn))();
    }
}

// CSiblingActorB::RegisterActs: same archetype, registry @0x64bf00.
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
                    ((CString*)list)->CString::~CString();
                }
                list++;
            } while (--cnt);
        }
        ((CString*)slot)->operator=(s_actKeyA);
        g_nextActId++;
    }
    ((CSiblingActorBEntry*)g_64bf00.ResolveEntry(id))->m_fn = &CSiblingActorB::Advance;
}

// Class C: CCheckpointTrigger, registry @0x64e7c0 (g_checkpointActReg, DATA-pinned
// in CCheckpointTrigger.cpp), handler @0x10ede0 (Trigger). CCheckpointActReg is the
// shared <Gruntz/ActReg.h> CActReg-derived alias.
extern CCheckpointActReg g_checkpointActReg; // 0x64e7c0 (DATA-pinned in CCheckpointTrigger.cpp)
struct CCheckpointActEntry {
    i32 (CCheckpointTrigger::*m_fn)();
};

// CCheckpointTrigger::RegisterActs: same archetype, registry @0x64e7c0.
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
                    ((CString*)list)->CString::~CString();
                }
                list++;
            } while (--cnt);
        }
        ((CString*)slot)->operator=(s_actKeyA);
        g_nextActId++;
    }
    ((CCheckpointActEntry*)g_checkpointActReg.ResolveEntry(id))->m_fn =
        &CCheckpointTrigger::Trigger;
}

SIZE_UNKNOWN(CCheckpointActEntry);
SIZE_UNKNOWN(CNetSingletonBe90);
SIZE_UNKNOWN(CSiblingActReg);
SIZE_UNKNOWN(CSiblingActorAEntry);
SIZE_UNKNOWN(CSiblingActorB);
SIZE_UNKNOWN(CSiblingActorBEntry);
SIZE_UNKNOWN(CNetSingletonBf00);
