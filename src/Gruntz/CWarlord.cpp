// CWarlord.cpp - the AI fort-warlord game object (RTTI .?AVCWarlord@@), a
// CUserLogic-derived leaf. Home TU for three __thiscall methods, in ascending
// retail-RVA order:
//   0x0107f0  ~CWarlord       the dtor: destruct the +0x54 CString, then the
//                             CUserLogic base (link ~EngStr + base vptr stores).
//                             The destructible members force the /GX EH frame.
//   0x044640  ResolveState    slot-4 override: a bounds-checked lookup into a
//                             file-static CArray of per-state handler vtables,
//                             growing it on miss, then dispatching `(*elem)(this)`.
//   0x044bb0  RearmMoving      re-arm the geometry sub-player off the global geo
//                             source, then resolve the moving animation when the
//                             sub's state words say so; returns 0.
//
// CUserBase / CUserLogic / EngStr / CGameObject come from <Gruntz/UserLogic.h>;
// MFC CString from <Mfc.h>. Engine callees/globals are reloc-masked (no body).
#include <Gruntz/CWarlord.h>

#include <rva.h>

// ===========================================================================
// CWarlord::~CWarlord  (0x0107f0)
// ===========================================================================
// CWarlord adds one destructible member past the CUserLogic base - the +0x54
// CString. The empty body lets MSVC emit the canonical most-derived teardown:
//   1. ~CString(m_54)                      (retail EH state 1)
//   2. store the CUserLogic vptr (0x5e705c); inline-destruct the +0x18 link's
//      ~EngStr                             (retail EH state 2)
//   3. store the CUserBase vptr (0x5e70b4)
// The destructible members force the /GX frame. The empty body is enough; the
// teardown BODY (lea+call x2 + the two base vptr stores) is byte-identical.
//
// @early-stop
// Two intersecting /GX EH-machine walls, no source lever (body is byte-exact):
//   (a) eh-dtor-vptr-restamp-presence.md - because the FIRST destruction is the
//       leaf's OWN +0x54 CString (vs the single-destructible leaves whose first
//       destruction is the base +0x18 link), cl re-stamps ??_7CWarlord at entry;
//       retail elided that store entirely.
//   (b) eh-state-numbering-base.md - retail numbers the two trylevels 1/2 over a
//       deeper frame (extra `push ecx`, state slot [esp+0x10], add esp,0x10),
//       ours uses 0/1 over [esp+0xc] / add esp,0xc. A cl EH-state-machine choice
//       for a 2-destructible-member dtor; neither member order nor the vptr model
//       (polymorphic vs manual) flips it. ~74%, deferred to the final sweep.
RVA(0x000107f0, 0x55)
CWarlord::~CWarlord() {}

// ===========================================================================
// CWarlord::ResolveState  (0x044640)  - slot-4 override (the animation dispatcher)
// ===========================================================================
// Bounds-check the state key against a file-static growable handler array
// (g_644610: lo @+0x18 / hi @+0x1c / base @+0x20 / elem-size @+0x28); on a miss,
// grow it via the engine CArray-grow helper (FUN_0056da80) down the AfxThrow-style
// out-of-memory path (FUN_0056d990 + the variadic FUN_0056d850). Then, if the
// resolved slot holds a handler, dispatch `(*slot)(this)`; returns the slot value.
//
// @early-stop
// DEFERRED to the final sweep / a leaf-first redo. This is a 0x102-byte function
// whose whole body is a deeply-INLINED engine collection access (an MFC-style
// CArray<>::ElementAt + grow + the AfxThrow out-of-memory funnel over the file-
// static table). Reproducing the exact template instantiation + regalloc is a
// large, high-wall task (the orchestrator flagged it as a likely regalloc wall);
// per breadth-first doctrine it is parked as a complete-intent placeholder rather
// than half-reconstructed (a partial would under-count AND diverge its regalloc).
RVA(0x00044640, 0x102)
i32 CWarlord::ResolveState(i32 key) {
    (void)key;
    return 0;
}

// ===========================================================================
// CWarlord::RearmMoving  (0x044bb0)
// ===========================================================================
// Re-arm the geometry sub-player at m_38->m_1a0 against the global default geo
// source (result discarded). Then, if the sub's state words say it is ready to
// move (m_28 != 0 && m_20 == 0), resolve the moving animation. Returns 0.
RVA(0x00044bb0, 0x38)
i32 CWarlord::RearmMoving() {
    ((CWarlordAnimPlayer*)m_38)->m_1a0.SetGeoSourceR(g_defaultGeo);
    CWarlordAnimSub* sub = &((CWarlordAnimPlayer*)m_38)->m_1a0;
    if (sub->m_28 != 0 && sub->m_20 == 0) {
        ResolveMovingAnimation();
    }
    return 0;
}
