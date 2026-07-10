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
// g_pathGameReg (CGameRegistry*): the rain cloud reads its draw-fill argument out of
// the light-FX pump's shade-table slot at m_logicPump->+0x28 (== m_tables[5]).

// @confidence: high
// @source: rtti-vptr
RVA(0x000b49b0, 0xa8)
CRainCloud::CRainCloud(CGameObject* obj) : CPathHazard(obj) {
    CGameObject* o = m_object;
    i32 n = (i32)g_pathGameReg->m_logicPump->m_tables[5]; // reg->+0x78->+0x28
    o->m_drawActive = 1;
    o->m_drawFillCmd = 0x7;
    o->m_drawFillArg = n;
    m_savedGeoId = m_38->m_geoId;
    m_38->ApplyLookupGeometry("LEVEL_RAINCLOUD", 0);
    m_object->m_areaL = 1;
    m_object->m_areaR = 1;
    m_object->m_areaT = 1;
    m_object->m_areaB = 1;
}
