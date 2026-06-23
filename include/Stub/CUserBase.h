#ifndef GRUNTZ_STUB_CUSERBASE_H
#define GRUNTZ_STUB_CUSERBASE_H
#include <rva.h>
// CUserBase - WAP32 game-object base (RTTI: CUserBase : CWapX). 3 virtuals, no own
// data -> sizeof 4 (vptr); CWapX is folded into the vptr here (as in UserLogic.h).
// Stub-world spine base for the un-graduated CUserLogic family.
class CUserBase {
public:
    virtual ~CUserBase();
    virtual int UserBaseVfunc1();
    virtual int UserBaseVfunc2();
};
#endif // GRUNTZ_STUB_CUSERBASE_H
