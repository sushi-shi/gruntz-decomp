// CLevelTimeDtor.cpp - CLevelTime's /GX leaf destructor (C:\Proj\Gruntz).
//
// The ctor stub still lives in src/Stub/CLevelTime.cpp (stub-world base); this TU
// hosts the leaf dtor against the matched <Gruntz/UserLogic.h> teardown.
#include <Gruntz/CLevelTimeDtor.h>

// CLevelTime::~CLevelTime @0x00011a50 - folds the bare CUserLogic teardown: store
// the CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link (the embedded
// ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The destructible
// link forces the /GX EH frame.
RVA(0x00011a50, 0x44)
CLevelTime::~CLevelTime() {}
