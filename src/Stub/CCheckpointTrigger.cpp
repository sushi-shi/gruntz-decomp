#include <rva.h>
// CCheckpointTrigger.cpp - engine-label stubs for CCheckpointTrigger.

#include <Stub/CCheckpointTrigger.h>

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

// @confidence: med
// @source: rtti-vptr
// @stub
RVA(0x0010ee20, 0x27d)
CCheckpointTrigger::CCheckpointTrigger(i32) {}
