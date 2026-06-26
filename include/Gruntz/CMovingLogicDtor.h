// CMovingLogicDtor.h - matched-world view of CMovingLogic for its leaf destructor
// (C:\Proj\Gruntz).
//
// CMovingLogic : CUserLogic (RTTI .?AVCMovingLogic@@) - the moving-object logic
// base (parent of CProjectile). Owner recovered by caller-trace: the
// scalar-deleting-destructor @0x00013c40 (CMovingLogic vftable slot 0) tail-calls
// this plain dtor @0x00013bd0. Although CMovingLogic carries its own most-derived
// vftable (0x5e87ac), the dtor's most-derived vptr store is dead-eliminated at
// /O2, leaving only the CUserLogic vftable store (0x5e705c) then the CUserBase
// vftable store (0x5e70b4) + the +0x18 link teardown (~EngStr @0x16d2a0) - so it
// is byte-identical to the established leaf-dtor archetype.
//
// NOTE: src/Stub/CMovingLogic.cpp still carries the un-matched ctor stub
// (0x13940) against the stub-world base; this matched-world view exists ONLY to
// host the leaf dtor against the real CUserLogic teardown.
#ifndef GRUNTZ_CMOVINGLOGICDTOR_H
#define GRUNTZ_CMOVINGLOGICDTOR_H

#include <rva.h>
#include <Gruntz/UserLogic.h> // CUserLogic base (CMovingLogic : CUserLogic)

class CMovingLogic : public CUserLogic {
public:
    ~CMovingLogic(); // 0x00013bd0 (folds the CUserLogic teardown)
};

#endif // GRUNTZ_CMOVINGLOGICDTOR_H
