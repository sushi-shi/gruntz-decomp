// CMovingLogicDtor.cpp - CMovingLogic's /GX leaf destructor (C:\Proj\Gruntz).
//
// The ctor stub still lives in src/Stub/CMovingLogic.cpp (stub-world base); this
// TU hosts the leaf dtor against the matched <Gruntz/UserLogic.h> teardown.
#include <Gruntz/CMovingLogicDtor.h>

// CMovingLogic::~CMovingLogic @0x00013bd0 - the most-derived vptr store is
// dead-eliminated at /O2, so the dtor folds the bare CUserLogic teardown: store
// the CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link (the embedded
// ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The destructible
// link forces the /GX EH frame.
RVA(0x00013bd0, 0x44)
CMovingLogic::~CMovingLogic() {}

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
#include <rva.h>
SIZE_UNKNOWN(CMovingLogic);
