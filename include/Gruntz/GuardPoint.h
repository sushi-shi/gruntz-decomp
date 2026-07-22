// GuardPoint.h - the guard-point marker (C:\Proj\Gruntz), a CUserLogic tile-logic
// leaf (RTTI .?AVCUserLogic@@). Only the /GX leaf dtor is reconstructed here; the
// ctor (0xae5f0) remains the @stub backlog in src/Stub/GuardPoint.cpp. Offsets +
// code bytes are load-bearing.
#ifndef GRUNTZ_CGUARDPOINT_H
#define GRUNTZ_CGUARDPOINT_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h> // LogicTypeId (GetTypeTag return type)
#include <Gruntz/UserLogic.h>

class CGuardPoint : public CUserLogic, public CWapX {
public:
public:
    // The class's own CUserLogic overrides, reconstructed as regular methods (the
    // fat <Gruntz/UserLogic.h> base models slots 1/2 with placeholder signatures, so
    // they cannot be spelled OVERRIDE here - see the base-signature note in the .cpp;
    // the leaf vtable is not a diffed symbol, so a plain method matches byte-for-byte).
    // 0x00010350 vtable slot 2: per-class logic-type id, inline (one
    // deduped COMDAT copy in retail; see docs on header-inline members).
    RVA(0x00010350, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_GUARDPOINT;
    }
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    CGuardPoint(CGameObject* obj);             // 0xae5f0
    // NO user-declared dtor: retail's is COMPILER-GENERATED (implicit
    // elides the leaf-vptr restamp; @rva-symbol pin in the home TU).
};
SIZE(0x54);

#endif // GRUNTZ_CGUARDPOINT_H
