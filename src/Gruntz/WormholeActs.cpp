#include <Gruntz/ActNameRegistry.h> // the shared activation-name registry archetype
#include <Gruntz/ActReg.h>          // the shared CActReg coordinate-registry archetype
#include <Gruntz/Wormhole.h>    // (kept: Wormhole.cpp shares this TU-header's registry note)
#include <Gruntz/ExitTrigger.h> // the owning class (act dispatcher + CExitActEntry)
#include <Gruntz/UserLogic.h>

#include <rva.h>
#include <Wap32/ZVec.h>
#include <Gruntz/WormholeActs.h> // g_exitTriggerActReg decl

// g_exitTriggerActReg (0x002445c0): CActReg - no provable static init (the type has no
// default ctor / is runtime-Init'd), so the datum is named by symbol.
DATA_SYMBOL(0x002445c0, 0x0, ?g_exitTriggerActReg@@3UCActReg@@A)

RVA(0x0003f210, 0x15)
void CExitTrigger::InitActReg() {
    g_exitTriggerActReg.Construct(2000, 2010);
}

// CWormhole::FireAct @0x03f290 [@identity-TODO: retail says this is CExitTrigger's
// vtable slot 4 - ILT 0x42e6 `jmp 0x3f290` + CExitTrigger vtbl 0x1e822c slot 4 == 0x42e6;
// see the note in <Gruntz/Wormhole.h>] - look the activation coordinate up in the
// class registry (g_exitTriggerActReg); if the resolved entry carries a registered
// handler PMF, resolve it again and dispatch it __thiscall on `this`. Same
// archetype as CParticlez::FireActivation (double ResolveEntry + PMF dispatch).
RVA(0x0003f290, 0x102)
void CExitTrigger::FireActivation(i32 coord) {
    CExitActEntry* e = reinterpret_cast<CExitActEntry*>(g_exitTriggerActReg.ResolveEntry(coord));
    if (e->m_fn != 0) {
        CExitActEntry* e2 = reinterpret_cast<CExitActEntry*>(g_exitTriggerActReg.ResolveEntry(coord));
        (this->*(e2->m_fn))();
    }
}

// CWormhole::RegisterActs @0x03f3f0 - bind the per-frame handler (AdvanceAnim
// @0x03f5f0) to the activation key "A" via the shared name registry. The SAME
// archetype as CGruntCreationPoint::RegisterActs.
//
// @early-stop
// register-pinning wall (docs/patterns/zero-register-pinning.md +
// test-old-value-decrement-loop-while-postdec.md, topic:wall topic:regalloc): logic
// byte-faithful (every call/immediate/branch/offset + the `mov [entry],offset
// AdvanceAnim` handler store match retail); residual is the slot-vs-id callee-saved
// register choice cascading into the free-loop count materialization. Deferred.
RVA(0x0003f3f0, 0x18d)
void CExitTrigger::RegisterActs() {
    i32 id = reinterpret_cast<i32>(g_buteTree.Find("A"));
    if (id == 0) {
        id = g_typeCounter;
        g_buteTree.Insert("A", reinterpret_cast<void*>(id));
        char* slot = ActNameLookup(id);
        i32 n = g_typeColl.m_grown;
        void** list = reinterpret_cast<void**>(g_typeColl.m_alloc);
        while (n-- != 0) {
            if (list != 0) {
                (reinterpret_cast<CString*>(list))->CString::~CString();
            }
            list++;
        }
        (reinterpret_cast<CString*>(slot))->operator=("A");
        g_typeCounter++;
    }
    (reinterpret_cast<CExitActEntry*>(g_exitTriggerActReg.ResolveEntry(id)))->m_fn =
        static_cast<i32 (CUserLogic::*)()>(&CExitTrigger::AdvanceAnim);
}
