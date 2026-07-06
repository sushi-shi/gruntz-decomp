// Explosion.cpp - the explosion eyecandy (C:\Proj\Gruntz), a CUserLogic leaf.
// The /GX leaf dtor and the 1-arg ctor (the shared CUserLogic(obj) prologue + the
// per-class eyecandy tail).
#include <Gruntz/Explosion.h>

// The global bute store (g_buteTree @0x6bf620; Find 0x16d190 __thiscall ret 4);
// pinned in src/Gruntz/UserLogic.cpp, re-declared so the `A` node lookup masks.
DATA(0x002bf620)
extern CButeTree g_buteTree;

// CExplosion::~CExplosion (0x12ec0) - the /GX leaf dtor folds the bare CUserLogic
// teardown: store the CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link
// (the embedded ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The
// destructible link forces the /GX EH frame; the leaf vptr store is dead-eliminated.
RVA(0x00012ec0, 0x44)
CExplosion::~CExplosion() {}

// CExplosion::CExplosion (0x470e0) - the eyecandy ctor: fold the shared
// CUserLogic(obj) init, then name the bound object "GAME_EXPLOSION", bind its "A"
// bute node, flag the sub-object, lock the draw order to 0xf4240 and clear m_38.
//
// @early-stop
// eh-ctor-vptr-restamp-position wall (docs/patterns/eh-ctor-vptr-restamp-position.md,
// topic:wall topic:eh): body byte-identical; residual is the /GX leaf-vptr re-stamp
// position + the EH-state ids, not source-steerable - the established leaf-ctor
// baseline (cf. CMenuSparkle 92.8% / CEyeCandy 92.5% in userlogic).
RVA(0x000470e0, 0x16b)
CExplosion::CExplosion(CGameObject* obj) : CUserLogic(obj) {
    TILE_LOGIC_SEED(obj);
    m_38->ApplyName("GAME_EXPLOSION");
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    m_38->m_flags |= 0x2000002;
    CGameObject* o = m_object;
    if (o->m_latchedAnimId != 0xf4240) {
        o->m_latchedAnimId = 0xf4240;
        o->m_flags |= 0x20000;
    }
    m_object->m_38 = 0;
}
