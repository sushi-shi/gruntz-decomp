// WormholeActs.cpp - CWormhole's activation-name registration (C:\Proj\Gruntz).
//
// CWormhole's per-frame activation handler is bound here by RegisterActs (the
// 0x18d shared-name-registry archetype). This lives in its own TU rather than
// Wormhole.cpp because Wormhole.cpp already models the SAME shared registry
// addresses (0x6bf650 / 0x6bf464 / 0x6bf428 / 0x60a454 / 0x61aea8) under its
// zvec-typed names (g_buteNameVec / g_zvecErrSentinel / g_retAddrBreadcrumb /
// s_wormholeLogicKey / g_logicRegCounter) for the SEPARATE logic-dispatch
// registration (RegisterWormholeLogic @0x401b0). Re-declaring the registry-typed
// view of those addresses in the same TU would collide; a dedicated TU pulls the
// shared <Gruntz/ActNameRegistry.h> cleanly (same as CLightFx.cpp).
//
// CWormhole : CUserLogic. The full class (virtual dtor, SpawnPartners, LoadColors)
// lives in Wormhole.cpp; here only the minimal members the registration touches
// are declared - the non-virtual AdvanceAnim PMF is a 4-byte single-inheritance
// code ptr regardless of the class's other virtuals, so the `mov [entry],offset
// AdvanceAnim` store reloc-masks identically.
#include <Gruntz/ActNameRegistry.h> // the shared activation-name registry archetype
#include <Gruntz/ActReg.h>          // the shared CActReg coordinate-registry archetype
#include <Gruntz/Wormhole.h>        // the shared CWormhole class (object logic + acts)
#include <Gruntz/UserLogic.h>

#include <rva.h>
#include <Wap32/ZVec.h>
#include <Wap32/ZDArrayDerived.h>

// CWormholeActEntry (the handler entry the per-class registry yields; its first dword
// receives the per-frame handler PMF) is the shared <Gruntz/Wormhole.h> shape.

// The class's activation-coordinate registry singleton (@0x6445c0), built over the
// fixed [2000,2010] range by the shared registry ctor (0x408710). The shared
// <Gruntz/ActReg.h> CActReg archetype, named by address (same as SpotLightActReg's
// g_actReg_646188) - the old CWormholeActReg empty subclass was a view; the global's
// name is our choice (DATA-reloc-masked), so CActReg is used directly.
DATA(0x002445c0)
CActReg g_wormholeActReg; // 0x6445c0

// CWormhole::InitActReg @0x03f210 - construct the class's activation-coordinate
// registry singleton (g_wormholeActReg @0x6445c0) over the fixed range
// [2000, 2010] via the shared registry ctor (0x408710). Free init thunk.
RVA(0x0003f210, 0x15)
void CWormhole::InitActReg() {
    ((CZDArrayDerived*)&g_wormholeActReg)->Construct(2000, 2010);
}

// CWormhole::FireAct @0x03f290 [@identity-TODO: retail says this is CExitTrigger's
// vtable slot 4 - ILT 0x42e6 `jmp 0x3f290` + CExitTrigger vtbl 0x1e822c slot 4 == 0x42e6;
// see the note in <Gruntz/Wormhole.h>] - look the activation coordinate up in the
// class registry (g_wormholeActReg); if the resolved entry carries a registered
// handler PMF, resolve it again and dispatch it __thiscall on `this`. Same
// archetype as CParticlez::FireActivation (double ResolveEntry + PMF dispatch).
RVA(0x0003f290, 0x102)
void CWormhole::FireAct(i32 coord) {
    CWormholeActEntry* e = (CWormholeActEntry*)g_wormholeActReg.ResolveEntry(coord);
    if (e->m_fn != 0) {
        CWormholeActEntry* e2 = (CWormholeActEntry*)g_wormholeActReg.ResolveEntry(coord);
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
void CWormhole::RegisterActs() {
    i32 id = (i32)g_buteTree.Find("A");
    if (id == 0) {
        id = g_typeCounter;
        g_buteTree.Insert("A", (void*)id);
        char* slot = ActNameLookup(id);
        i32 n = g_typeColl.m_grown;
        void** list = (void**)g_typeColl.m_alloc;
        while (n-- != 0) {
            if (list != 0) {
                ((CString*)list)->CString::~CString();
            }
            list++;
        }
        ((CString*)slot)->operator=("A");
        g_typeCounter++;
    }
    ((CWormholeActEntry*)g_wormholeActReg.ResolveEntry(id))->m_fn = (i32 (CUserLogic::*)())&CWormhole::AdvanceAnim;
}

// (CWormhole's SIZE_UNKNOWN + CWormholeActEntry now ride <Gruntz/Wormhole.h>;
// g_wormholeActReg is a plain CActReg named by address - no per-TU view left.)
