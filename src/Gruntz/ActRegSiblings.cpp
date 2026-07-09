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
#include <Gruntz/AniAdvanceCursor.h>  // CAniAdvanceCursor (m_38+0x1a0 anim sub-mgr; Advance)
#include <Gruntz/GameRegistry.h>      // g_gameReg->m_world->m_8 (CSpriteFactory)
#include <Gruntz/SpriteFactory.h>     // CSpriteFactory::CreateSprite (0x1597b0)

// The +0x1a0 anim sub-mgr's per-frame delta scalar (g_6bf3bc, u32) and the game-
// registry singleton (0x64556c); address-pinned so the loads reloc-mask.
extern "C" u32 g_6bf3bc;
DATA(0x0024556c)
extern CGameRegistry* g_gameReg;

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
// The anim-owning sprite (m_38) viewed through its +0x1a0 anim sub-mgr: a
// CAniAdvanceCursor typed value member (its m_20/m_28 are the idle/active flags),
// plus the redraw flags at +0x08. Same cast-free anim-view shape as CWarpM154 /
// CDecayMgr in UserLogic.cpp.
SIZE_UNKNOWN(CDropAnimObj);
struct CDropAnimObj {
    char m_pad0[0x8];
    i32 m_flags; // +0x08  redraw/state flags
    char m_pad0c[0x1a0 - 0xc];
    CAniAdvanceCursor m_1a0; // +0x1a0  per-object anim sub-mgr (m_20 idle, m_28 active)
};

// A CUserLogic-leaf actor (identity best-guess): Advance reads the bound object at
// +0x10 and the anim-owning sprite at +0x38 exactly like CObjectDropper's leaf tail.
struct CSiblingActorB {
    char m_pad0[0x10];
    CGameObject* m_object;    // +0x10  bound object (CUserLogic::m_object)
    char m_pad14[0x38 - 0x14];
    CDropAnimObj* m_38;       // +0x38  the anim-owning sprite (TILE_LOGIC tail)
    i32 Advance();            // 0xc7ab0
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

// CSiblingActorB::Advance (0xc7ab0): the per-frame act handler - advance the bound
// object's +0x1a0 anim sub-mgr, and on the drop frame (Advance == 2) spawn a
// "DroppedObject" sprite at the bound object's screen position; then raise the
// object's redraw bit if the anim latched active while idle-clear (same idle tail
// as CDroppedObject::UserLogicVfunc5).
// @early-stop
// tail regalloc coin-flip (98.48%): the whole body is byte-faithful (the Advance
// call, the ==2 gate, the CreateSprite spawn, the idle check). The sole residue is
// the m_38 reload in the idle tail: with the CreateSprite branch in front, `this`
// is dead there and retail REUSES esi (`mov esi,[esi+0x38]`), while cl keeps `this`
// in esi and loads m_38 into eax. The identical tail matched EXACT in
// CDroppedObject::UserLogicVfunc5 (no preceding branch); not source-steerable here
// (local/inline m_object + permute all identical). Parked for the final sweep.
RVA(0x000c7ab0, 0x67)
i32 CSiblingActorB::Advance() {
    if (m_38->m_1a0.Advance_15c360(g_6bf3bc) == 2) {
        CGameObject* o = m_object;
        g_gameReg->m_world->m_8->CreateSprite(
            0, o->m_screenX, o->m_screenY, 0, "DroppedObject", 0x40003
        );
    }
    if (m_38->m_1a0.m_28 != 0 && m_38->m_1a0.m_20 == 0) {
        m_38->m_flags |= 0x10000;
    }
    return 0;
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

// CCheckpointTrigger::FireActivation (0x10ea80), vtable slot 4: look the activation
// coordinate up in the class registry (g_checkpointActReg); if the resolved entry
// carries a registered handler PMF, resolve it again and dispatch it __thiscall on
// `this`. Same double-ResolveEntry + PMF-fire archetype as the two siblings above.
RVA(0x0010ea80, 0x102)
void CCheckpointTrigger::FireActivation(i32 coord) {
    CCheckpointActEntry* e = (CCheckpointActEntry*)g_checkpointActReg.ResolveEntry(coord);
    if (e->m_fn != 0) {
        CCheckpointActEntry* e2 = (CCheckpointActEntry*)g_checkpointActReg.ResolveEntry(coord);
        (this->*(e2->m_fn))();
    }
}

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
