// CLevelTimeDtor.h - matched-world view of CLevelTime for its leaf destructor
// (C:\Proj\Gruntz).
//
// CLevelTime : CUserLogic (RTTI .?AVCLevelTime@@) - the level-timer tile-logic
// object. Owner recovered by caller-trace: the scalar-deleting-destructor
// @0x00011a20 (CLevelTime vftable slot 0) tail-calls this plain dtor @0x00011a50.
// The dtor stamps the CUserLogic vftable 0x5e705c then the CUserBase vftable
// 0x5e70b4, tearing down the +0x18 link via the embedded ~EngStr @0x16d2a0 -
// byte-identical to the established leaf-dtor archetype.
//
// NOTE: src/Stub/CLevelTime.cpp still carries the un-matched ctor stub (0x9b8b0)
// against the stub-world base; this matched-world view exists ONLY to host the
// leaf dtor against the real CUserLogic teardown.
#ifndef GRUNTZ_CLEVELTIMEDTOR_H
#define GRUNTZ_CLEVELTIMEDTOR_H

#include <rva.h>
#include <Gruntz/UserLogic.h> // CUserLogic base (CLevelTime : CUserLogic)

class CLevelTime : public CUserLogic {
public:
    ~CLevelTime(); // 0x00011a50 (folds the CUserLogic teardown)
};

#endif // GRUNTZ_CLEVELTIMEDTOR_H
