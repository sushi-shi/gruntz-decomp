// WormholeActs.cpp - CWormhole's activation-name registration (C:\Proj\Gruntz).
//
// CWormhole's per-frame activation handler is bound here by RegisterActs (the
// 0x18d shared-name-registry archetype). This lives in its own TU rather than
// Wormhole.cpp because Wormhole.cpp already models the SAME shared registry
// addresses (0x6bf650 / 0x6bf464 / 0x6bf428 / 0x60a454 / 0x61aea8) under its
// zvec-typed names (g_buteNameVec / g_zvecErrSentinel / g_zvecErrToken /
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

// The handler entry the per-class registry yields: its first dword receives the
// per-frame handler PMF (AdvanceAnim, a 4-byte code ptr on this single-inheritance
// class).
typedef i32 (CWormhole::*WormholeHandler)();
struct CWormholeActEntry {
    WormholeHandler m_fn;
};

// The class's activation-coordinate registry singleton (@0x6445c0), built over the
// fixed [2000,2010] range by the shared registry ctor (0x408710). Was a per-file
// duplicate of the <Gruntz/ActReg.h> CActReg archetype (layout + ResolveEntry); now
// derives from it, keeping its own placeholder name so the DATA-pinned global is
// unchanged.
struct CWormholeActReg : public CActReg {};
DATA(0x002445c0)
extern CWormholeActReg g_wormholeActReg; // 0x6445c0

// CWormhole::InitActReg @0x03f210 - construct the class's activation-coordinate
// registry singleton (g_wormholeActReg @0x6445c0) over the fixed range
// [2000, 2010] via the shared registry ctor (0x408710). Free init thunk.
RVA(0x0003f210, 0x15)
void CWormhole::InitActReg() {
    g_wormholeActReg.Construct(2000, 2010);
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
    i32 id = (i32)g_buteTree.Find(s_actKeyA);
    if (id == 0) {
        id = g_nextActId;
        g_buteTree.Insert(s_actKeyA, (void*)id);
        char* slot = ActNameLookup(id);
        i32 n = g_nameRegScratch;
        void** list = g_nameRegCurList;
        while (n-- != 0) {
            if (list != 0) {
                ((CActName*)list)->Free();
            }
            list++;
        }
        ((CActName*)slot)->Assign(s_actKeyA);
        g_nextActId++;
    }
    ((CWormholeActEntry*)g_wormholeActReg.ResolveEntry(id))->m_fn = &CWormhole::AdvanceAnim;
}

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
// (CWormhole's SIZE_UNKNOWN now rides <Gruntz/Wormhole.h>.)
SIZE_UNKNOWN(CWormholeActEntry);
SIZE_UNKNOWN(CWormholeActReg);
