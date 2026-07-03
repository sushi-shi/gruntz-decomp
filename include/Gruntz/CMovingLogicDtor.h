// CMovingLogicDtor.h - hosts CMovingLogic's leaf destructor (0x00013bd0) against
// the canonical class (C:\Proj\Gruntz).
//
// CMovingLogic : CUserLogic (RTTI .?AVCMovingLogic@@) - the moving-object logic
// base. The leaf dtor @0x00013bd0 folds the bare CUserLogic teardown: although
// CMovingLogic carries its own most-derived vftable (0x5e87ac), the dtor's
// most-derived vptr store is dead-eliminated at /O2, leaving only the CUserLogic
// vftable store (0x5e705c) then the CUserBase vftable store (0x5e70b4) + the +0x18
// link teardown (~EngStr @0x16d2a0) - byte-identical to the leaf-dtor archetype.
//
// The class is the shared <Gruntz/CMovingLogic.h> canonical (this header only pins
// the include so the dtor TU compiles against the one true definition).
#ifndef GRUNTZ_CMOVINGLOGICDTOR_H
#define GRUNTZ_CMOVINGLOGICDTOR_H

#include <Gruntz/CMovingLogic.h>

#endif // GRUNTZ_CMOVINGLOGICDTOR_H
