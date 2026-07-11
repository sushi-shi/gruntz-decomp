// LevelTimeDtor.h - matched-world view of CLevelTime for its leaf destructor
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
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    RVA(0x00011990, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_LEVELTIME;
    } // slot 2
    TILE_LOGIC_TAIL
public:
    CLevelTime(CGameObject* obj);   // 0x9b8b0
    virtual ~CLevelTime() OVERRIDE; // 0x00011a50 (folds the CUserLogic teardown)

    // +0x40..+0x53: the level-timer state tail. NOT initialized by the ctor
    // (0x9b8b0 writes nothing past +0x3c - lazily-used fields); their existence +
    // the 0x54 size are pinned by the StateDispatch new-site (0x9b770: push 0x54
    // before operator new, then the bare `call ??0CLevelTime` thunk 0x404d).
    char m_pad40[0x54 - 0x40];
};
VTBL(CLevelTime, 0x1e801c);
SIZE(CLevelTime, 0x54); // `new CLevelTime` @0x9b77f pushes 0x54

#endif // GRUNTZ_CLEVELTIMEDTOR_H
