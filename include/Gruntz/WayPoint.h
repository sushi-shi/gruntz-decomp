// WayPoint.h - the waypoint marker (C:\Proj\Gruntz), a CUserLogic tile-logic leaf
// (RTTI .?AVCUserLogic@@). Only the /GX leaf dtor is reconstructed here; the ctor
// (0xae3f0) remains the @stub backlog in src/Stub/WayPoint.cpp. Offsets + code
// bytes are load-bearing.
#ifndef GRUNTZ_CWAYPOINT_H
#define GRUNTZ_CWAYPOINT_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h> // LogicTypeId (GetTypeTag return type)
#include <Gruntz/UserLogic.h>

class CWayPoint : public CUserLogic, public CWapX {
public:
public:
    // The class's own CUserLogic slot overrides, reconstructed as regular methods
    // (the fat base models these slots with placeholder signatures; see the .cpp).
    // 0x00010220 vtable slot 2: per-class logic-type id, inline (one
    // deduped COMDAT copy in retail; see docs on header-inline members).
    RVA(0x00010220, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_WAYPOINT;
    }
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    CWayPoint(CGameObject* obj);               // 0xae3f0
    // NO user-declared dtor: retail's is COMPILER-GENERATED (implicit
    // elides the leaf-vptr restamp; @rva-symbol pin in the home TU).
};
SIZE(0x54);
VTBL(CWayPoint, 0x001e74b4);

#endif // GRUNTZ_CWAYPOINT_H
