// CDoNothingNormalDtor.cpp - CDoNothingNormal's /GX leaf destructor (C:\Proj\Gruntz).
//
// The ctor stub still lives in src/Stub/CDoNothingNormal.cpp (stub-world base);
// this TU hosts the leaf dtor against the matched <Gruntz/UserLogic.h> teardown.
#include <Gruntz/CDoNothingNormalDtor.h>

// CDoNothingNormal::~CDoNothingNormal @0x0000f8a0 - folds the bare CUserLogic
// teardown: store the CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link
// (the embedded ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The
// destructible link forces the /GX EH frame. Byte-identical to ~CDoNothing
// @0x0000f770.
RVA(0x0000f8a0, 0x44)
CDoNothingNormal::~CDoNothingNormal() {}
