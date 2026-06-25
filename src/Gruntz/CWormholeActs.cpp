// CWormholeActs.cpp - CWormhole's activation-name registration (C:\Proj\Gruntz).
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
#include <Gruntz/UserLogic.h>

#include <rva.h>

class CWormhole : public CUserLogic {
public:
    static void RegisterActs(); // 0x03f3f0
    i32 AdvanceAnim();          // 0x03f5f0 (the per-frame handler PMF)
};

// The handler entry the per-class registry yields: its first dword receives the
// per-frame handler PMF (AdvanceAnim, a 4-byte code ptr on this single-inheritance
// class).
typedef i32 (CWormhole::*WormholeHandler)();
struct CWormholeActEntry {
    WormholeHandler m_fn;
};

// The class's activation-coordinate registry singleton (@0x6445c0). Same
// [2000,2010] fixed-range shape as CCreationPointActReg, built by the shared
// registry ctor (0x408710). ResolveEntry folds the VActLookup archetype inline;
// the slow Insert is __thiscall on m_coll2.
struct CWormholeActReg {
    void* m_vptr;       // +0x00
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
DATA(0x002445c0)
extern CWormholeActReg g_wormholeActReg; // 0x6445c0

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
