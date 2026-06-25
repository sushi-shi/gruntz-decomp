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

extern "C" int rand(void); // CRT _rand (reloc-masked)

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
// CWarlord::CWarlord(int)  (0x042d40)  - the warlord ctor
// ===========================================================================
// Builds the CUserLogic base, names the warlord's threat string, then a 4-way
// switch over the owner-index selects the king/napolean/patton/viking config and
// seeds the per-state idle/battlecry/death/moving animation table off the
// "Gruntz" bute group. ~0x73e bytes.
//
// @early-stop
// DEFERRED to the final sweep (big function, >512B, STOP-EARLY). The full body is
// a deep switch + the engine string-table + bute-config inline expansion; per
// breadth-first doctrine it is homed here (so the RVA + its LoadAttributes caller
// pair) as a complete-intent placeholder rather than half-reconstructed.
RVA(0x00042d40, 0x73e)
CWarlord::CWarlord(i32) {}

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

// ===========================================================================
// CWarlord::LoadAttributes  (0x044c00)  - the warlord's per-tick threat update
// ===========================================================================
// Re-arm the geometry sub-player off the global geo source (bail if not ready);
// in multiplayer, measure the nearest-enemy distance vs the "Warlordz/PanicRadius"
// config (default 64) and raise the fort alert when inside the radius; otherwise,
// past the 64-bit cooldown window, randomly resolve an idle / battlecry animation.
// Returns int 0 on every path. Plain /O2 leaf (no destructible local, no /GX use).
RVA(0x00044c00, 0xc6)
i32 CWarlord::LoadAttributes() {
    if (((CWarlordAnimPlayer*)m_38)->m_1a0.SetGeoSourceR(g_defaultGeo) != 1) {
        return 0;
    }

    WwdGameReg* reg = g_gameReg;
    if (reg->m_134 != 1) {
        CWarlordOwner* o = (CWarlordOwner*)m_10;
        i32 dist = reg->m_68->NearestEnemyDist(o->m_124, o->m_5c, o->m_60);
        if (dist < g_buteMgr.GetIntDef("Warlordz", "PanicRadius", 0x40)) {
            NotifyFortUnderAttack();
            return 0;
        }
    }

    if ((i64)(u32)g_645588 - *(i64*)&m_88 >= *(i64*)&m_90) {
        if (rand() % 10 < 5) {
            ResolveIdleAnimation();
            return 0;
        }
        ResolveBattlecryAnimation();
    }
    return 0;
}

// ===========================================================================
// CWarlord::LoadAttributes2  (0x044d10)  - the single-player-aware variant
// ===========================================================================
// Same geo-sub re-arm gate; multiplayer raises the battle alert when the nearest
// enemy is NOT inside the panic radius; single-player resolves the moving anim
// while the level objective is open, else posts a fort battle event past the
// cooldown window and re-arms a 0x7530 stamp. Returns int 0 on every path.
//
// @early-stop
// regalloc wall (topic:regalloc, docs/patterns/zero-register-pinning.md +
// pin-local-for-callee-saved-reg.md): structure/offsets/instruction-selection are
// byte-exact, but retail keeps g_gameReg in edx (live in BOTH the multiplayer and
// single-player branches, freeing ecx for the thiscall `this`) while cl parks it
// in ecx, mirror-swapping g_645588 into the other scratch reg. A pure scratch
// ecx<->edx coin-flip - no source lever flips it (tried inline vs named helper,
// m_2c-chain split; all no-change at the same ~91% plateau).
RVA(0x00044d10, 0x106)
i32 CWarlord::LoadAttributes2() {
    if (((CWarlordAnimPlayer*)m_38)->m_1a0.SetGeoSourceR(g_defaultGeo) != 1) {
        return 0;
    }

    WwdGameReg* reg = g_gameReg;
    if (reg->m_134 != 1) {
        CWarlordOwner* o = (CWarlordOwner*)m_10;
        i32 dist = reg->m_68->NearestEnemyDist(o->m_124, o->m_5c, o->m_60);
        if (dist >= g_buteMgr.GetIntDef("Warlordz", "PanicRadius", 0x40)) {
            RaiseBattleAlert();
            return 0;
        }
    } else {
        if (reg->m_2c->m_3f4->m_4c == 0) {
            ResolveMovingAnimation();
            return 0;
        }
        if ((i64)(u32)g_645588 - *(i64*)&m_88 >= *(i64*)&m_90) {
            reg->m_60->PostBattleEvent(((CWarlordOwner*)m_10)->m_188, 0x436, -1, -1, -1);
            m_90 = 0x7530;
            m_94 = 0;
            m_88 = g_645588;
            m_8c = 0;
        }
    }
    return 0;
}

// ===========================================================================
// CWarlord::BuildFortSplashParticles  (0x044f80)
// ===========================================================================
// Re-arm the geo sub-player, and when ready-to-move, spawn the fort splash
// particle at the warlord's clamped screen position (registry effect dispatch),
// arm the panic timer on the registry sub-object, then flag the anim player.
// @early-stop
// DEFERRED to the final sweep: 0x127-byte function over a dozen WwdGameReg offsets
// (viewport bounds, the +0x30 effect-spawn dispatch, the +0x68 panic sub-object
// + its +0x290 timer, the +0x150 owner-array slot). Homed so the RVA is labeled.
RVA(0x00044f80, 0x127)
void CWarlord::BuildFortSplashParticles() {}

// ===========================================================================
// CWarlord::NotifyFortUnderAttack  (0x045270)
// ===========================================================================
// Raise the fort-under-attack alert when an enemy breaches the panic radius.
// @early-stop
// DEFERRED to the final sweep (big function, 0x2a8 > 512B, STOP-EARLY). Homed so
// the RVA + its LoadAttributes caller pair; full body left for a leaf-first redo.
RVA(0x00045270, 0x2a8)
void CWarlord::NotifyFortUnderAttack() {}
