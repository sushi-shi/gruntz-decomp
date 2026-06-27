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

// The class's activation-coordinate registry singleton (@0x64e7c0), built by the
// shared registry ctor (0x408710, __thiscall ret 8) over the fixed [2000,2010]
// range. Only Construct is needed for the init thunk; the registry's full layout
// (CLeafActReg) is modeled in m2_ActRegSiblings.cpp for RegisterActs.
struct CCheckpointActReg {
    char m_pad[0x24];
    void Construct(i32 lo, i32 hi); // 0x408710
};
DATA(0x0024e7c0)
extern CCheckpointActReg g_checkpointActReg; // 0x64e7c0

// CCheckpointTrigger::InitActReg @0x10ea00 - construct the class's activation-
// coordinate registry singleton over [2000, 2010]. Free init thunk; reloc-masked.
RVA(0x0010ea00, 0x15)
void CCheckpointTrigger::InitActReg() {
    g_checkpointActReg.Construct(2000, 2010);
}
