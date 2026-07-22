// Explosion.h - the explosion eyecandy (C:\Proj\Gruntz), a CUserLogic tile-logic
// leaf (RTTI .?AVCUserLogic@@). Only the /GX leaf dtor is reconstructed here; the
// ctor (0x470e0) remains the @stub backlog in src/Stub/Explosion.cpp. Offsets +
// code bytes are the load-bearing facts; field names are placeholders.
#ifndef GRUNTZ_CEXPLOSION_H
#define GRUNTZ_CEXPLOSION_H

#include <rva.h>
#include <Gruntz/ActReg.h> // CLogicActTable (the slot-4 activation-dispatch table)
#include <Gruntz/UserLogic.h>

class CExplosion : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    RVA(0x00012e00, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_EXPLOSION;
    } // slot 2
public:
    CExplosion(CGameObject* obj); // 0x470e0
    // FireActivation (0x47350): slot-4 (UserLogicVfunc2) override - resolve `id` in
    // the class dispatch table g_logicActReg_6447f8; if the resolved entry holds a
    // handler, re-resolve and dispatch it __thiscall on `this`. Same archetype as
    // CTeleporter::FireActivation.
    virtual void FireActivation(i32 id) OVERRIDE;
    // NO user-declared dtor: retail's is COMPILER-GENERATED (implicit
    // elides the leaf-vptr restamp; @rva-symbol pin in the home TU).
};
SIZE(0x54);
VTBL(CExplosion, 0x1e766c);

extern CLogicActTable g_logicActReg_6447f8;

typedef i32 (CUserLogic::*ExplosionActHandler)();
struct CExplosionActEntry {
    ExplosionActHandler m_fn;
};
SIZE_UNKNOWN(); // only the first dword (the handler) is modeled

#endif // GRUNTZ_CEXPLOSION_H
