#include <Gruntz/GruntToyTimeSprite.h>
#include <Bute/ButeTree.h> // g_buteTree.Find (0x16d190) - the "A" animset seed

RVA_COMPGEN(0x00012130, 0x44, ??1CGruntToyTimeSprite@@UAE@XZ)

// CGruntToyTimeSprite::CGruntToyTimeSprite @0x0007fbd0 - the /GX HUD sprite ctor.
// Chains the CGruntHealthSprite base ctor (0x7eb00, via thunk 0x3224; declared-only
// here so the base `call` reloc-masks). Same shape as CGruntStaminaSprite's ctor
// (different sprite key + anchor constants); see GruntStaminaSprite.cpp.
//
// @confidence: high
// @source: rtti-vptr
// @early-stop
// eh-ctor-vptr-restamp-position wall (94.76%): real polymorphic base, body+offsets
// byte-identical; the single residual is the leaf vptr re-stamp (mov [esi],&??_7) that
// MSVC5's /GX EH-state machine sinks into the first throwing call's state 0 instead of
// retail's eager post-base-ctor stamp. NOT source-steerable
// (docs/patterns/eh-ctor-vptr-restamp-position.md). Deferred to the final sweep.
RVA(0x0007fbd0, 0xa0)
CGruntToyTimeSprite::CGruntToyTimeSprite(CGameObject* obj) : CGruntHealthSprite(obj) {
    m_38->ApplyLookupSprite("GAME_GRUNTTOYTIMESPRITE", 1);
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    CWwdGameObjectA* o = m_object;
    if (o->m_sortKey != 0xdbba0) {
        o->m_sortKey = 0xdbba0;
        o->m_flags |= 0x20000;
    }
    m_health = 0; // +0x5c  toy-time icon screen-offset X (reuses the base slot)
    m_60 = -0x20; // +0x60  toy-time icon screen-offset Y (drawn above the grunt)
}

RVA(0x0007fca0, 0xd)
i32 CGruntToyTimeSprite::Vslot16(CGrunt* grunt) {
    return grunt->m_toyTime;
}

// ~CGruntToyTimeSprite @0x012130 - the /GX leaf dtor. Folds the bare CUserLogic
// teardown (store CUserLogic vptr 0x5e705c, inline-destruct the +0x18 link via
// ~EngStr @0x16d2a0, store CUserBase vptr 0x5e70b4); the intermediate leaf/health
// vptr stamps dead-store-eliminate. Byte-identical to the sibling leaf dtors.
// IMPLICIT dtor (retail is COMPILER-GENERATED - eh-dtor-vptr-restamp CAUSE B):
// a user-declared `~CGruntToyTimeSprite() {}` emits the leaf-vptr restamp, and the CWapX
// base EH state blocks the dead-store elision that used to hide it. The ??_G
// in the vtable-emitting TU forces the implicit ??1 COMDAT; pinned by name.
#include <rva.h>
