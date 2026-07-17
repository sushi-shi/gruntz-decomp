// RainCloud.cpp - the rain-cloud path-hazard game-object (C:\Proj\Gruntz), a
// CPathHazard leaf (RTTI-proven; vtable_hierarchy --tree). Split out of the former
// GameObjectCtors.cpp catch-all so the class maps to one TU / one retail .obj. The
// ctor is the emission anchor (cl emits ??_7CRainCloud@@6B@ + the implicit
// post-base vptr stamp here).
//
// CRainCloud : CPathHazard - the base ctor (0xb35a0, via thunk 0x2fc2) is DECLARED
// only (out-of-line in PathHazard.cpp), so the leaf's base `call` reloc-masks by
// address. The base folds the whole CUserLogic init + constructs the throwing
// CUserBaseLink, so the leaf emits the /GX EH frame.
#include <Gruntz/RainCloud.h> // CRainCloud : CPathHazard (canonical; pulls PathHazard.h -> GameRegistry.h)
#include <Gruntz/LightFxMgr.h> // reg->m_logicPump (+0x78): the shade-table pump the fill arg reads
#include <rva.h>

// The game registry / settings singleton (*0x24556c) is modeled by PathHazard.h as
// g_gameReg (CGameRegistry*): the rain cloud reads its draw-fill argument out of
// the light-FX pump's shade-table slot at m_logicPump->+0x28 (== m_tables[5]).

// @confidence: high
// @source: rtti-vptr
RVA(0x000b49b0, 0xa8)
CRainCloud::CRainCloud(CGameObject* obj) : CPathHazard(obj) {
    CGameObject* o = m_object;
    i32 n = (i32)g_gameReg->m_logicPump->m_tables[5]; // reg->+0x78->+0x28
    o->m_drawActive = 1;
    o->m_drawFillCmd = 0x7;
    o->m_drawFillArg = n;
    m_value = m_38->m_1a0.m_14;
    m_38->ApplyLookupGeometry("LEVEL_RAINCLOUD", 0);
    m_object->m_areaL = 1;
    m_object->m_areaR = 1;
    m_object->m_areaT = 1;
    m_object->m_areaB = 1;
}

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
// @rva-symbol: ??1CRainCloud@@UAE@XZ 0x00013340 0x44
