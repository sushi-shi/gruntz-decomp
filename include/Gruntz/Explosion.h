// Explosion.h - the explosion eyecandy (C:\Proj\Gruntz), a CUserLogic tile-logic
// leaf (RTTI .?AVCUserLogic@@). Only the /GX leaf dtor is reconstructed here; the
// ctor (0x470e0) remains the @stub backlog in src/Stub/Explosion.cpp. Offsets +
// code bytes are the load-bearing facts; field names are placeholders.
#ifndef GRUNTZ_CEXPLOSION_H
#define GRUNTZ_CEXPLOSION_H

#include <rva.h>
#include <Gruntz/ActReg.h> // CLogicActTable (the slot-4 activation-dispatch table)
#include <Gruntz/UserLogic.h>

class CExplosion : public CUserLogic {
public:
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    virtual LogicTypeId GetTypeTag() OVERRIDE;                         // slot 2
    virtual i32 UserLogicVfunc2() OVERRIDE;                            // slot 4
    i32 Serialize(i32 ar, i32 tag, i32 c, i32 d); // 0x012e20 (vtable slot 1: serialize chain)
    TILE_LOGIC_TAIL
public:
    CExplosion(CGameObject* obj); // 0x470e0
    // FireActivation (0x47350): slot-4 (UserLogicVfunc2) override - resolve `id` in
    // the class dispatch table g_logicActReg_6447f8; if the resolved entry holds a
    // handler, re-resolve and dispatch it __thiscall on `this`. Same archetype as
    // CTeleporter::FireActivation.
    void FireActivation(i32 id);
    virtual ~CExplosion() OVERRIDE; // 0x12ec0 (folds the CUserLogic teardown)
    char m_pad40[0x54 - 0x40];
};
VTBL(CExplosion, 0x1e766c);
SIZE(CExplosion, 0x54);

// The class's activation-dispatch table (CLogicActTable @0x6447f8); filled by
// RegisterXLogic_6447f8 (LogicActReg.cpp), read by FireActivation. Owner (DATA
// binding) is LogicActReg.cpp; declared extern here so the loads reloc-mask.
extern CLogicActTable g_logicActReg_6447f8;

// A dispatch-table entry: its first dword is the class activation handler, stored
// by the registrar as a free-fn ptr but dispatched __thiscall on `this` - a 4-byte
// single-inheritance PMF gives the plain `mov ecx,this; call [entry]` code.
typedef i32 (CExplosion::*ExplosionActHandler)();
struct CExplosionActEntry {
    ExplosionActHandler m_fn;
};
SIZE_UNKNOWN(CExplosionActEntry); // only the first dword (the handler) is modeled

#endif // GRUNTZ_CEXPLOSION_H
