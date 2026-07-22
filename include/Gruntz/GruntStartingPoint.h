// GruntStartingPoint.h - the grunt starting-point marker (C:\Proj\Gruntz), a
// CUserLogic tile-logic leaf (RTTI .?AVCUserLogic@@). Only the /GX leaf dtor is
// reconstructed here; the ctor (0x3df30) remains the @stub backlog in
// src/Stub/GruntStartingPoint.cpp. Offsets + code bytes are load-bearing.
#ifndef GRUNTZ_CGRUNTSTARTINGPOINT_H
#define GRUNTZ_CGRUNTSTARTINGPOINT_H

#include <rva.h>
#include <Gruntz/UserLogic.h>

class CGruntStartingPoint : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    RVA(0x000105b0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_GRUNTSTARTINGPOINT;
    } // slot 2
public:
    CGruntStartingPoint(CGameObject* obj);   // 0x3df30
    // NO user-declared dtor: retail's is COMPILER-GENERATED (implicit
    // elides the leaf-vptr restamp; RVA_COMPGEN pin in the home TU).
    virtual void FireActivation(i32 id)
        OVERRIDE; // 0x3e1a0 (vtable slot 4: per-coord PMF dispatch, R4 registry)
};
SIZE(0x54);

typedef i32 (CUserLogic::*StartActHandler)();
struct StartActEntry {
    StartActHandler m_fn;
};
SIZE_UNKNOWN(); // only the first dword (the handler) is modeled

extern u32 g_zvecErrSentinel; // 0x002bf464


// TU-local thunk/table names this TU registers (moved from the .cpp; the
// addresses are ILT thunk VAs, reloc-masked at every use).
extern "C" void ActReg4Handler(); // 0x4040a2

#endif // GRUNTZ_CGRUNTSTARTINGPOINT_H
