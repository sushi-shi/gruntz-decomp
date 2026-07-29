// GruntStartingPoint.h - the grunt starting-point marker (C:\Proj\Gruntz), a
// CUserLogic tile-logic leaf (RTTI .?AVCUserLogic@@). Only the /GX leaf dtor is
// reconstructed here; the ctor (0x3df30) remains the @stub backlog in
// src/Stub/GruntStartingPoint.cpp. Offsets + code bytes are load-bearing.
#ifndef GRUNTZ_CGRUNTSTARTINGPOINT_H
#define GRUNTZ_CGRUNTSTARTINGPOINT_H

#include <rva.h>
#include <Gruntz/UserLogic.h>
#include <Gruntz/ActReg.h> // CActReg (for the extern below)

class CGruntStartingPoint : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeMove(CFileMemBase*, i32, i32, CGameObject*) OVERRIDE; // slot 1
    RVA(0x000105b0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_GRUNTSTARTINGPOINT;
    } // slot 2
public:
    CGruntStartingPoint(CGameObject* obj); // 0x3df30
    // NO user-declared dtor: retail's is COMPILER-GENERATED (implicit
    // elides the leaf-vptr restamp; RVA_COMPGEN pin in the home TU).
    virtual void FireActivation(i32 id)
        OVERRIDE; // 0x3e1a0 (vtable slot 4: per-coord PMF dispatch, R4 registry)
    // The act-"A" (idle) slot ActReg4RegisterType (0x3e300) binds via ILT 0x4040a2:
    // retail 0x3e500 is the bare `xor eax,eax; ret` - "nothing to do".
    i32 Idle(); // 0x3e500
};
SIZE(0x54);

SIZE_UNKNOWN(); // only the first dword (the handler) is modeled

// --- the TU's extern surface (moved out of the .cpp; addresses/thunk
// VAs are reloc-masked at use) ---

#endif // GRUNTZ_CGRUNTSTARTINGPOINT_H
