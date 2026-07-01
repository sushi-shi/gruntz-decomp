#include <rva.h>
#include <Ints.h>

#include <Gruntz/SbiDtorChain.h>
// SBI_GruntMachineEh.cpp - the /GX EH-framed CSBI_GruntMachine destructor
// (C:\Proj\Gruntz). The split off the frameless base TU is matching-neutral (each
// function is RVA-keyed). Chain: CSBI_GruntMachine : CStatusBarItem (shared).

struct CSBI_GruntMachine : CStatusBarItem {
    virtual ~CSBI_GruntMachine();
    void Reset(); // 0xe8c70  most-derived member teardown (reloc-masked)
};

RVA(0x00104ce0, 0x55)
CSBI_GruntMachine::~CSBI_GruntMachine() {
    Reset();
}
