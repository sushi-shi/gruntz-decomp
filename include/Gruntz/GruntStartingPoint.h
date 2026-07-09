// GruntStartingPoint.h - the grunt starting-point marker (C:\Proj\Gruntz), a
// CUserLogic tile-logic leaf (RTTI .?AVCUserLogic@@). Only the /GX leaf dtor is
// reconstructed here; the ctor (0x3df30) remains the @stub backlog in
// src/Stub/GruntStartingPoint.cpp. Offsets + code bytes are load-bearing.
#ifndef GRUNTZ_CGRUNTSTARTINGPOINT_H
#define GRUNTZ_CGRUNTSTARTINGPOINT_H

#include <rva.h>
#include <Gruntz/UserLogic.h>

class CGruntStartingPoint : public CUserLogic {
public:
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    i32 Serialize(i32 ar, i32 tag, i32 c, i32 d); // 0x105d0 (slot-1 two-chain body)
    virtual LogicTypeId GetTypeTag() OVERRIDE;                         // slot 2
    virtual i32 UserLogicVfunc2() OVERRIDE;                            // slot 4
    TILE_LOGIC_TAIL
public:
    CGruntStartingPoint(CGameObject* obj);   // 0x3df30
    virtual ~CGruntStartingPoint() OVERRIDE; // 0x10670 (folds the CUserLogic teardown)
    void FireActivation(i32 coord); // 0x3e1a0 (vtable slot 4: per-coord PMF dispatch, R4 registry)
    char m_pad40[0x54 - 0x40];
};
VTBL(CGruntStartingPoint, 0x1e8284);
SIZE(CGruntStartingPoint, 0x54);

#endif // GRUNTZ_CGRUNTSTARTINGPOINT_H
