// CCheckpointTrigger.cpp - the checkpoint-trigger tile-logic object
// (C:\Proj\Gruntz), a CUserLogic leaf. Only the /GX leaf dtor is reconstructed.
#include <Gruntz/CCheckpointTrigger.h>

// ~CCheckpointTrigger @0x011480 - the leaf adds no destructible members beyond
// CUserLogic, so its dtor folds the bare CUserLogic teardown: store the
// CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link (the embedded
// ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The destructible
// link forces the /GX EH frame; the empty body is enough for cl.
RVA(0x00011480, 0x44)
CCheckpointTrigger::~CCheckpointTrigger() {}
