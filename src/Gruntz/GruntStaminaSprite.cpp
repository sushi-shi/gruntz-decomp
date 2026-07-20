#include <Gruntz/GruntStaminaSprite.h>
#include <Gruntz/LogicTypeId.h>
#include <Bute/ButeTree.h> // g_buteTree.Find (0x16d190) - the "A" animset seed

// CGruntStaminaSprite::CGruntStaminaSprite @0x0007fae0 - the /GX HUD sprite ctor.
// Chains the CGruntHealthSprite base ctor (0x7eb00, via thunk 0x3224; declared-only
// here so the base `call` reloc-masks). cl stamps the leaf vftable then the body
// caches the bound object's frame sprite (m_38->ApplyLookupSprite), swaps in the "A"
// bute animset node (m_prevAnimSetNode/m_objAux->m_1c), forces the object's pose id
// (m_sortKey = 0xdbba0 + m_flags |= 0x20000) and seeds the icon anchor - the
// +0x5c/+0x60 slots the CGruntHealthSprite base names m_health/m_60, reused as the
// stamina icon's screen offset.
//
// @confidence: high
// @source: rtti-vptr
// @early-stop
// eh-ctor-vptr-restamp-position wall (94.76%): real polymorphic base, body+offsets
// byte-identical (byte-verified vs 0x0007fae0); the single residual is the leaf vptr
// re-stamp (mov [esi],&??_7) which MSVC5's /GX EH-state machine sinks INTO the first
// throwing call's state 0 instead of retail's eager post-base-ctor stamp. NOT
// source-steerable (docs/patterns/eh-ctor-vptr-restamp-position.md). Deferred to the
// final sweep.
RVA(0x0007fae0, 0xa0)
CGruntStaminaSprite::CGruntStaminaSprite(CGameObject* obj) : CGruntHealthSprite(obj) {
    m_38->ApplyLookupSprite("GAME_GRUNTSTAMINASPRITE", 1);
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    CWwdGameObjectA* o = m_object;
    if (o->m_sortKey != 0xdbba0) {
        o->m_sortKey = 0xdbba0;
        o->m_flags |= 0x20000;
    }
    m_health = 0x64; // +0x5c  stamina icon screen-offset X (reuses the base slot)
    m_60 = -0x20;    // +0x60  stamina icon screen-offset Y (drawn above the grunt)
}

// CGruntStaminaSprite::~CGruntStaminaSprite @0x00012070 - the leaf adds no
// destructible members beyond CUserLogic, so its dtor folds the bare CUserLogic
// teardown: store the CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link
// (the embedded ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The
// destructible link forces the /GX EH frame. Byte-identical in shape to
// ~CGruntHealthSprite @0x00011fb0; the empty body is enough for cl.
// IMPLICIT dtor (retail is COMPILER-GENERATED - eh-dtor-vptr-restamp CAUSE B):
// a user-declared `~CGruntStaminaSprite() {}` emits the leaf-vptr restamp, and the CWapX
// base EH state blocks the dead-store elision that used to hide it. The ??_G
// in the vtable-emitting TU forces the implicit ??1 COMDAT; pinned by name.
// @rva-symbol: ??1CGruntStaminaSprite@@UAE@XZ 0x00012070 0x44

RVA(0x0007fbb0, 0xd)
i32 CGruntStaminaSprite::Vslot16(CGrunt* grunt) {
    return grunt->m_stamina;
}
