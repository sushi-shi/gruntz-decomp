// GuardPoint.h - the guard-point marker (C:\Proj\Gruntz), a CUserLogic tile-logic
// leaf (RTTI .?AVCUserLogic@@). Only the /GX leaf dtor is reconstructed here; the
// ctor (0xae5f0) remains the @stub backlog in src/Stub/GuardPoint.cpp. Offsets +
// code bytes are load-bearing.
#ifndef GRUNTZ_CGUARDPOINT_H
#define GRUNTZ_CGUARDPOINT_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h> // LogicTypeId (GetTypeTag return type)
#include <Gruntz/UserLogic.h>

class CGuardPoint : public CTileLogic {
public:
    // The class's own CUserLogic overrides, reconstructed as regular methods (the
    // fat <Gruntz/UserLogic.h> base models slots 1/2 with placeholder signatures, so
    // they cannot be spelled OVERRIDE here - see the base-signature note in the .cpp;
    // the leaf vtable is not a diffed symbol, so a plain method matches byte-for-byte).
    LogicTypeId GetTypeTag();                  // 0x10350 (vtable slot 2: per-class logic-type id)
    i32 Serialize(i32 a, i32 b, i32 c, i32 d); // 0x10370 (vtable slot 1: serialize chain)
    CGuardPoint(CGameObject* obj);             // 0xae5f0
    virtual ~CGuardPoint() OVERRIDE;           // 0x10410 (folds the CUserLogic teardown)
    char m_pad40[0x54 - 0x40];
};
SIZE(CGuardPoint, 0x54);

#endif // GRUNTZ_CGUARDPOINT_H
