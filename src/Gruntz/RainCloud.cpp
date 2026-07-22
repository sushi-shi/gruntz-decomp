#include <Gruntz/RainCloud.h> // CRainCloud : CPathHazard (canonical; pulls PathHazard.h -> GameRegistry.h)
#include <Gruntz/LightFxMgr.h> // reg->m_logicPump (+0x78): the shade-table pump the fill arg reads
#include <Gruntz/GruntzMgr.h> // complete CGruntzMgr
#include <rva.h>

VTBL(CRainCloud, 0x001e7324); // vtable_names -> code (RTTI game class)
// ~CRainCloud @0x013340 - the CPathHazard-derived rain-cloud leaf's dtor: no
// destructible members of its own, so it folds the bare CUserLogic teardown (store
// the CUserLogic vptr, inline-destruct the +0x18 link's ~EngStr, store the CUserBase
// vptr; the throwing link forces the /GX EH frame). IDENTITY (vtable-owner probe):
// ??_7CRainCloud @0x1e7324 (RTTI-named, <Gruntz/RainCloud.h>) slot 0 -> ILT thunk ->
// the sdd 0x13310 -> THIS body (it was once misbound as ~CPathHazard).
//
// IMPLICIT (retail is COMPILER-GENERATED - eh-dtor-vptr-restamp CAUSE B): a
// user-declared `~CRainCloud() {}` emits the leaf-vptr restamp, and the CWapX base
// EH state blocks the dead-store elision that used to hide it. THIS obj emits
// ??_7CRainCloud -> ??_G -> the implicit ??1 COMDAT (the ctor above is what needs
// the vtable), so the pin resolves here - PathHazard.cpp never emits it.
RVA_COMPGEN(0x00013340, 0x44, ??1CRainCloud@@UAE@XZ)

RVA(0x000b49b0, 0xa8)
CRainCloud::CRainCloud(CGameObject* obj) : CPathHazard(obj) {
    CWwdGameObjectA* o = m_object;
    i32 n = reinterpret_cast<i32>(g_gameReg->m_logicPump->m_tables[5]); // reg->+0x78->+0x28
    o->m_drawActive = 1;
    o->m_drawFillCmd = 0x7;
    o->m_drawFillArg = n;
    m_value = m_38->m_1a0.m_14;
    m_38->ApplyLookupGeometry("LEVEL_RAINCLOUD", 0);
    m_object->m_area.left = 1;
    m_object->m_area.right = 1;
    m_object->m_area.top = 1;
    m_object->m_area.bottom = 1;
}

