// SpotLightCtor.cpp - the 1-arg CSpotLight constructor (0xb1200), re-homed from
// src/Stub/SpotLight.cpp (C:\Proj\Gruntz).
//
// CSpotLight : CUserLogic (vftable 0x5e75bc). The ctor folds the shared inline
// CUserLogic(CGameObject*) base init (link + RegisterLogicTypesOnce + the three
// AddLogic* + the m_04..m_3c stores), stamps its own vptr, re-resolves the bute
// "A" node into the bound object's aux, snaps the object's screen position onto the
// 0x20 grid and seeds the rotation offsets (m_70/m_78/m_60/m_68/m_80/m_88), then
// reads the SpotLightTime / settings-table tuning and seeds the per-tick state.
//
// The standalone ctor lives in its OWN TU so the existing flat CSpotLight carcass +
// the matched Update (src/Gruntz/SpotLight.cpp) are untouched. Field names are
// placeholders; the OFFSETS + code bytes are the load-bearing facts. The throwing
// CUserBaseLink in the CUserLogic base forces the /GX EH frame -> eh.
#include <Mfc.h>
#include <Gruntz/UserLogic.h>    // CUserLogic / CGameObject base init + g_buteMgr
#include <Bute/ButeMgr.h>        // CButeTree / CButeMgr
#include <Gruntz/GameRegistry.h> // canonical *0x24556c singleton (color table via m_78)
#include <rva.h>

// The bute store the "A" activation node is resolved through (g_buteTree @0x6bf620,
// Find @0x16d190); declared extern so the call reloc-masks (the Stub TU owns it).
DATA(0x002bf620)
extern CButeTree g_buteTree;

// The two .rdata scale doubles the per-tick rate folds in (0x5ea3f0 numerator /
// 0x5ea3f8 multiplier). Reloc-masked; the literal value is irrelevant to the match.
extern const double g_spotRateNum; // 0x5ea3f0
extern const double g_spotRateMul; // 0x5ea3f8

// The per-frame light-color table is the spotlight facet of the canonical
// registry's reused +0x78 slot ((CSpotMgrTable*)g_gameReg->m_78; see
// CGameRegistry.h): the ctor indexes it by m_object->m_11c (+0x14 base); the
// alpha-blend gate is the registry's m_134 discriminator. Authentic downcast.
struct CSpotMgrTable {
    char m_pad00[0x14];
    i32 m_arr[1]; // +0x14
};
DATA(0x0024556c)
extern CGameRegistry* g_gameReg;

// ---------------------------------------------------------------------------
// CSpotLight : CUserLogic - the light eyecandy logic. Own fields begin past the
// CUserLogic base (+0x40): the rotation/offset doubles + the per-tick state ints.
class CSpotLight : public CUserLogic {
public:
    CSpotLight(CGameObject* obj); // 0xb1200
    virtual ~CSpotLight() OVERRIDE;

    char m_pad40[0x58 - 0x40];
    double m_58; // +0x58  per-tick rate
    double m_60; // +0x60
    double m_68; // +0x68
    double m_70; // +0x70
    double m_78; // +0x78
    double m_80; // +0x80
    double m_88; // +0x88
    double m_90; // +0x90  pi or 0
    i32 m_98;    // +0x98
    i32 m_9c;    // +0x9c
    i32 m_a0;    // +0xa0
    i32 m_a4;    // +0xa4
};

// Out-of-line vtable anchor (gives CSpotLight a real vftable so the ctor's vptr
// store falls out). Body not matched.
CSpotLight::~CSpotLight() {}

RVA(0x000b1200, 0x2cb)
// @early-stop
// x87 fp-stack-schedule wall (docs/patterns/x87-fp-stack-schedule.md, topic:wall):
// the /GX EH frame, the folded CUserLogic(obj) base init, the bute "A" re-resolve,
// the 0x20-grid snap + m_124/m_120 adjust, the SpotLightTime/settings tuning and
// every integer field store are byte-faithful; the residual is the rotation block
// that derives m_60/m_68/m_80/m_88 from m_70/m_78 - its fld/fsub/fxch stack
// ordering is not steerable from C (same wall as CSpotLight::Update @0xb1ee0).
// Logic complete; deferred to the final sweep.
CSpotLight::CSpotLight(CGameObject* obj) : CUserLogic(obj) {
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    m_38->m_flags |= 2;

    i32 ax = (m_object->m_screenX & ~0x1f) + 0x10;
    i32 cx = (m_object->m_screenY & ~0x1f) + 0x10;
    m_70 = (double)ax;
    m_78 = (double)cx;
    i32 nx;
    if (m_object->m_124 == 0) {
        nx = ax - 0x20;
    } else {
        nx = ax - m_object->m_124 * 32;
    }
    m_object->m_screenX = nx;
    m_object->m_screenY = cx;
    m_60 = (double)nx;
    m_68 = m_78;
    if (m_object->m_latchedAnimId != 0xcf850) {
        m_object->m_latchedAnimId = 0xcf850;
        m_object->m_flags |= 0x20000;
    }
    m_80 = m_70 - m_60;
    m_88 = m_78 - m_68;

    u32 v;
    if (m_object->m_120 == 0) {
        v = g_buteMgr.GetDwordDef("Hazardz", "SpotLightTime", 0xbb8);
    } else {
        v = m_object->m_120;
    }
    m_58 = g_spotRateNum / (double)(u32)v;
    if (m_object->m_12c == 1) {
        m_58 = m_58 * g_spotRateMul;
    }
    if (m_object->m_118 == 1) {
        m_90 = 3.1415927;
    } else {
        m_90 = 0;
    }
    i32 looked = ((CSpotMgrTable*)g_gameReg->m_logicPump)->m_arr[m_object->m_11c];
    m_object->m_drawActive = 1;
    m_object->m_drawFillCmd = 7;
    m_object->m_drawFillArg = looked;
    m_98 = 0;
    m_object->m_areaL = 0;
    m_object->m_areaR = 0;
    m_object->m_areaT = 0;
    m_object->m_areaB = 0;
    m_9c = -1;
    m_a0 = -1;
    m_a4 = 0;
    if (g_gameReg->m_134 == 1) {
        m_a4 = 1;
    }
}

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
SIZE_UNKNOWN(CSpotMgrTable);
